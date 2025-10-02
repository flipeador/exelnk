#include "../framework.hpp"

#define CREATE_PATH_ABSOLUTE(path, flags) \
    Path(                                 \
        path,                             \
        PATH_FLAG_IGNORE_ROOTED         | \
        PATH_FLAG_IGNORE_RELATIVE       | \
        PATH_FLAG_IGNORE_DRIVE_RELATIVE | \
        flags                             \
    )

#define GET_CURRENT_DIRECTORY_PATH(flags) \
    CREATE_PATH_ABSOLUTE(GetCurrentDirectory(), flags)

#define CURRENT_DIRECTORY_FULL_PATH GET_CURRENT_DIRECTORY_PATH(0)
#define CURRENT_DIRECTORY_ROOT_PATH GET_CURRENT_DIRECTORY_PATH(PATH_FLAG_IGNORE_SEGMENTS)

static auto IsRootLocalDeviceDrive(StrView path)
{
    static std::wregex re(L"^(\\\\|/){2}\\?(\\\\|/)[A-Z]:", std::regex_constants::icase);
    return std::regex_search(path.begin(), path.end(), re);
}

static auto IsRootLocalDeviceUNC(StrView path)
{
    static std::wregex re(L"^(\\\\|/){2}\\?(\\\\|/)UNC(\\\\|/)", std::regex_constants::icase);
    return std::regex_search(path.begin(), path.end(), re);
}

static auto Extract(StrView& path, StrView& name, bool* ews)
{
    if (path.empty()) return false;
    const auto pos = path.find_first_of(L"\\/");
    if (pos == path.npos)
    {
        name = path;
        path.remove_prefix(path.size());
    }
    else
    {
        name = path.substr(0, pos);
        path.remove_prefix(pos + 1);
        if (ews) *ews = path.empty();
    }
    return true;
}

static auto Extract(StrView& path)
{
    StrView name;
    Extract(path, name, nullptr);
    return String(name);
}

Path::Path(StrView path, uint32_t flags)
    : m_pView(&path)
{
    if (IsRootLocalDeviceDrive(path))
    {
        path.remove_prefix(4);
        goto __path_type_drive;
    }
    else if (IsRootLocalDeviceUNC(path))
    {
        path.remove_prefix(8);
        goto __path_type_unc_absolute;
    }

    if (IsSep(0))
    {
        if (IsSep(1))
        {
            // Local Device.
            if (At(2) == L'.' && IsSep(3))
            {
                m_type = PATH_TYPE_DEVICE;
                m_root = path;
            }
            // Root Local Device.
            else if (At(2) == L'?' && IsSep(3))
            {
                m_type = PATH_TYPE_ROOT_DEVICE;
                m_root = path;
            }
            // UNC Absolute.
            else
            {
                path.remove_prefix(2);
                __path_type_unc_absolute:
                m_type = PATH_TYPE_UNC;
                m_server = Extract(path);
                m_root = Extract(path);
            }
        }
        // Rooted.
        else if (!BITALL(flags, PATH_FLAG_IGNORE_ROOTED))
        {
            path.remove_prefix(1);
            m_type = PATH_TYPE_ROOTED;
        }
    }
    else if (At(0))
    {
        if (At(1) == L':')
        {
            __path_type_drive:
            m_root = std::format(L"{:c}:", towupper(At(0)));
            // Drive Absolute.
            if (!At(2) || IsSep(2))
            {
                path.remove_prefix(At(2) ? 3 : 2);
                m_type = PATH_TYPE_DRIVE_ABSOLUTE;
            }
            // Drive Relative.
            else if (!BITALL(flags, PATH_FLAG_IGNORE_DRIVE_RELATIVE))
            {
                path.remove_prefix(2);
                m_type = PATH_TYPE_DRIVE_RELATIVE;
            }
        }
        // Relative.
        else if (!BITALL(flags, PATH_FLAG_IGNORE_RELATIVE))
            m_type = PATH_TYPE_RELATIVE;
    }

    // Parse the path segments.
    if (!IsDevice() && !BITALL(flags, PATH_FLAG_IGNORE_SEGMENTS))
    {
        StrView part;
        while (Extract(path, part, &m_endsWithSep))
        {
            if (part == L"..")
            {
                // Process non-consecutive `..`.
                if (!m_segments.empty() && m_segments.back() != L"..")
                    m_segments.pop_back();
                // Keep `..` if the path is relative.
                // They are processed in `MakeAbsolute`.
                else if (IsRelative())
                    m_segments.emplace_back(part);
            }
            else if (!part.empty() && part != L".")
                m_segments.emplace_back(part);
        }
    }
}

uint8_t Path::Type() const
{
    return m_type;
}

StrView Path::Name() const
{
    if (m_segments.empty())
        return L"";
    return m_segments.back();
}

String Path::ToString(int64_t nseg) const
{
    if (!m_type) return L"";
    if (IsDevice()) return m_root;

    String path;
    const auto count = ClampIndex(nseg, m_segments.size());

    if (IsAbsolute())
    {
        if (m_type == PATH_TYPE_DRIVE_ABSOLUTE)
            path = std::format(L"\\\\?\\{}", m_root);
        else if (m_type == PATH_TYPE_UNC)
            path = std::format(L"\\\\?\\UNC\\{}\\{}", m_server, m_root);
    }
    else if (m_type == PATH_TYPE_DRIVE_RELATIVE)
        path.assign(m_root);
    else if (m_type == PATH_TYPE_ROOTED)
        path.push_back(L'\\');
    else if (m_type == PATH_TYPE_RELATIVE)
        path.push_back(L'.');

    for (size_t i = 0; i < count; ++i)
    {
        if (i || m_type != PATH_TYPE_DRIVE_RELATIVE)
            path.push_back(L'\\');
        path.append(m_segments[i]);
    }

    if (!count || (nseg == INT64_MAX && m_endsWithSep))
        path.push_back(L'\\');

    return path;
}

DWORD Path::Resolve(size_t i)
{
    if (!i) MakeAbsolute();
    DWORD error = NO_ERROR;
    if (i < m_segments.size())
    {
        error = EnumerateFiles(
            ToString(i + 1),
            [&](WIN32_FIND_DATA* pfd) -> DWORD {
                m_segments[i] = pfd->cFileName;
                // Stop enumeration if there are no more segments.
                if (i == m_segments.size() - 1)
                    return ERROR_RESOURCE_ENUM_USER_STOP;
                // Continue enumeration if the current item is not a directory.
                if (!BITALL(pfd->dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
                    return NO_ERROR;
                // Continue the depth-first search at the next segment.
                DWORD error = Resolve(i + 1);
                // Continue enumeration if no matching items have been found.
                if (error == ERROR_FILE_NOT_FOUND || error == ERROR_NO_MORE_FILES)
                    return NO_ERROR;
                // Stop enumeration if an error has occurred or an item has been found.
                return error;
            }
        );
    }
    return error;
}

void Path::MakeAbsolute()
{
    if (m_type == PATH_TYPE_ROOTED)
    {
        auto path = CURRENT_DIRECTORY_ROOT_PATH;
        MoveFrom(path, false);
    }
    else if (m_type == PATH_TYPE_RELATIVE)
    {
        auto path = CURRENT_DIRECTORY_FULL_PATH;
        MoveFrom(path, true);
    }
    else if (m_type == PATH_TYPE_DRIVE_RELATIVE)
    {
        // If the drive letter matches the current working directory drive letter, use that directory.
        auto path = CURRENT_DIRECTORY_FULL_PATH;
        if (path.m_type == PATH_TYPE_DRIVE_ABSOLUTE && path.m_root == m_root)
            MoveFrom(path, true);
        else
        {
            const auto name = std::format(L"={}:", m_root[0]);
            // If the environment variable "=X:" exists, use its value.
            path = CREATE_PATH_ABSOLUTE(GetEnvironmentVariable(name), 0);
            if (path.m_type == PATH_TYPE_DRIVE_ABSOLUTE)
                MoveFrom(path, true);
            // If all else fails, use the drive letter and update the environment variable.
            else
            {
                m_type = PATH_TYPE_DRIVE_ABSOLUTE;
                SetEnvironmentVariable(name, ToString());
            }
        }
    }

    // Process ".." in the path segments.
    size_t size = 0;
    for (size_t i = 0; i < m_segments.size(); ++i)
    {
        if (m_segments[i] == L"..")
        {
            if (size > 0)
                --size;
        }
        else
            m_segments[size++] = std::move(m_segments[i]);
    }
    m_segments.resize(size);
}

bool Path::IsDevice() const
{
    return m_type == PATH_TYPE_DEVICE || m_type == PATH_TYPE_ROOT_DEVICE;
}

bool Path::IsAbsolute() const
{
    return m_type == PATH_TYPE_DRIVE_ABSOLUTE || m_type == PATH_TYPE_UNC;
}

bool Path::IsRelative() const
{
    return m_type == PATH_TYPE_ROOTED || m_type == PATH_TYPE_RELATIVE || m_type == PATH_TYPE_DRIVE_RELATIVE;
}

bool Path::operator==(const Path& rhs) const
{
    return
        m_type == rhs.m_type &&
        m_root == rhs.m_root &&
        StrEqual(m_server, rhs.m_server, true) &&
        std::ranges::equal(m_segments, rhs.m_segments,
            [](const String& s1, const String& s2) -> bool {
                return StrEqual(s1, s2, true);
            }
        );
}

Path& Path::operator/=(const Path& rhs)
{
    m_endsWithSep = rhs.m_endsWithSep;
    m_segments.append_range(rhs.m_segments);
    return *this;
}

Path::operator PCWSTR() const
{
    const auto path = operator StrView();
    return path.empty() ? nullptr : path.data();
}

Path::operator StrView() const
{
    m_path.reserve(PATH_MAX - 1);
    const auto path = ToString();
    if (path.size() < PATH_MAX)
        return m_path.assign(path);
    return L"";
}

wchar_t Path::At(size_t index) const
{
    return m_pView->data()[index];
}

bool Path::IsSep(size_t index) const
{
    const auto c = At(index);
    return c == L'\\' || c == L'/';
}

void Path::MoveFrom(Path& path, bool moveSegments)
{
    if (!path.IsAbsolute())
        throw std::runtime_error("");

    m_type = path.m_type;
    m_server = std::move(path.m_server);
    m_root = std::move(path.m_root);

    if (moveSegments)
    {
        for (auto& segment : m_segments)
            path.m_segments.push_back(std::move(segment));
        m_segments = std::move(path.m_segments);
    }
}
