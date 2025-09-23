#include "../framework.hpp"

#define CREATE_PATH_ABSOLUTE(path, flags) \
    Path(                                 \
        path,                             \
        PATH_FLAG_IGNORE_ROOTED         | \
        PATH_FLAG_IGNORE_RELATIVE       | \
        PATH_FLAG_IGNORE_DRIVE_RELATIVE | \
        flags                             \
    )

#define GET_CURRENT_DIRECTORY_PATH(flags)              \
    CREATE_PATH_ABSOLUTE(GetCurrentDirectory(), flags)

static size_t ClampIndex(int64_t i, size_t size)
{
    const auto n = std::min(size, (size_t)std::abs(i));
    return i < 0 ? size - n : n;
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
    if (IsSep(0)) // "\"
    {
        if (IsSep(1)) // "\\"
        {
            // Local Device.
            if (At(2) == L'.' && IsSep(3)) // "\\.\"
            {
                m_type = PATH_TYPE_DEVICE;
                m_path = path;
            }
            // Root Local Device.
            else if (At(2) == L'?' && IsSep(3)) // "\\?\"
            {
                path.remove_prefix(4); // skip "\\?\"
                if (Is(0, L'U') && Is(1, L'N') && Is(2, L'C') && (!At(3) || IsSep(3))) // "\\?\UNC"
                {
                    path.remove_prefix(At(3) ? 4 : 3); // skip "UNC\" or "UNC"
                    goto __path_type_set_unc;
                }
                else if (!ParseDrive(flags)) // "\\?\X:"
                {
                    m_type = PATH_TYPE_ROOT_DEVICE;
                    m_path = std::format(L"\\\\?\\{}", path);
                }
            }
            // UNC Absolute.
            else // "\\"
            {
                path.remove_prefix(2); // skip "\\"
                __path_type_set_unc:
                m_type = PATH_TYPE_UNC;
                m_server = Extract(path);
                m_root = Extract(path);
            }
        }
        // Rooted.
        else if (!BITALL(flags, PATH_FLAG_IGNORE_ROOTED)) // "\"
        {
            path.remove_prefix(1); // skip "\"
            auto cwp = GET_CURRENT_DIRECTORY_PATH(PATH_FLAG_IGNORE_SEGMENTS);
            if ((m_type = cwp.Type()))
            {
                m_server = std::move(cwp.m_server);
                m_root = std::move(cwp.m_root);
            }
        }
    }
    // Drive Absolute/Relative | Relative.
    else if (!ParseDrive(flags) && At(0)) // "X:" | "X:." | "."
    {
        if (!BITALL(flags, PATH_FLAG_IGNORE_RELATIVE))
        {
            auto cwp = GET_CURRENT_DIRECTORY_PATH(0);
            if ((m_type = cwp.Type()))
            {
                m_server = std::move(cwp.m_server);
                m_root = std::move(cwp.m_root);
                m_segments = std::move(cwp.m_segments);
            }
        }
    }

    if (m_type == PATH_TYPE_UNC || m_type == PATH_TYPE_DRIVE)
    {
        if (!BITALL(flags, PATH_FLAG_IGNORE_SEGMENTS))
        {
            StrView part;
            while (Extract(path, part, &m_endsWithSep))
            {
                if (part == L"..")
                {
                    if (!m_segments.empty())
                        m_segments.pop_back();
                }
                else if (!part.empty() && part != L".")
                    m_segments.emplace_back(part);
            }
        }
    }

    if (BITALL(flags, PATH_FLAG_UPDATE_DRIVE_RELATIVE) && Type() == PATH_TYPE_DRIVE)
        SetEnvironmentVariable(std::format(L"={}:", m_root[0]), ToString());
}

StrView Path::Name() const
{
    if (m_segments.empty())
        return L"";
    return m_segments.back();
}

uint8_t Path::Type() const
{
    if (m_type == PATH_TYPE_UNC || m_type == PATH_TYPE_DRIVE)
    {
        if (m_root.empty()) return 0;
        if (m_type == PATH_TYPE_UNC && m_server.empty()) return 0;
        if (m_type == PATH_TYPE_DRIVE && !IsDriveLetter(m_root[0])) return 0;
    }
    const auto size = ToString().size();
    return size && size < PATH_MAX ? m_type : 0;
}

DWORD Path::Resolve(size_t i)
{
    DWORD error = NO_ERROR;
    if (i < m_segments.size())
    {
        error = EnumerateFiles(
            ToString(i + 1),
            [&](WIN32_FIND_DATA* pfd) -> DWORD {
                m_segments[i] = pfd->cFileName;
                DWORD error = Resolve(i + 1);
                if (error == ERROR_FILE_NOT_FOUND || error == ERROR_NO_MORE_FILES)
                    return NO_ERROR; // continue enum if no files have been found
                return error == NO_ERROR ? ERROR_RESOURCE_ENUM_USER_STOP : error;
            }
        );
    }
    return error;
}

StrView Path::ToString(int64_t nseg) const
{
    if (m_type == PATH_TYPE_UNC || m_type == PATH_TYPE_DRIVE)
    {
        if (m_type == PATH_TYPE_DRIVE)
            m_path = L"\\\\?\\" + m_root;
        else
            m_path = std::format(L"\\\\?\\UNC\\{}\\{}", m_server, m_root);

        const auto count = ClampIndex(nseg, m_segments.size());

        if (!count || m_segments.empty())
            m_path.push_back(L'\\');
        else
        {
            for (size_t i = 0; i < count; ++i)
                m_path += L"\\" + m_segments[i];
            if (count == INT64_MAX && m_endsWithSep)
                m_path.push_back(L'\\');
        }
    }

    return m_path;
}

Path::operator PCWSTR() const
{
    return Type() ? ToString().data() : nullptr;
}

Path::operator StrView() const
{
    return Type() ? ToString() : L"";
}

bool Path::IsDriveLetter(wchar_t c)
{
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122); // A-Z || a-z
}

wchar_t Path::At(size_t index) const
{
    return m_pView->data()[index];
}

bool Path::Is(size_t index, wchar_t c) const
{
    return towupper(At(index)) == c;
}

bool Path::IsSep(size_t index) const
{
    const auto c = At(index);
    return c == L'\\' || c == L'/';
}

bool Path::ParseDrive(uint32_t& flags)
{
    if (At(0) && At(1) == L':') // "X:"
    {
        const wchar_t letter = towupper(At(0));

        // Drive Absolute.
        if (!At(2) || IsSep(2)) // "X:" or "X:\"
        {
            m_type = PATH_TYPE_DRIVE;
            m_root = std::format(L"{}:", letter); // "X:"
            m_pView->remove_prefix(At(2) ? 3 : 2); // skip "X:\" or "X:"
        }
        // Drive Relative.
        else if (!BITALL(flags, PATH_FLAG_IGNORE_DRIVE_RELATIVE)) // "X:."
        {
            m_type = PATH_TYPE_DRIVE;
            // If the drive letter matches the current working directory drive letter, use that directory.
            auto cwp = GET_CURRENT_DIRECTORY_PATH(0);
            if (cwp.Type() == PATH_TYPE_DRIVE && cwp.m_root[0] == letter)
            {
                m_root = std::move(cwp.m_root);
                m_segments = std::move(cwp.m_segments);
            }
            else
            {
                // If the environment variable "=X:" exists, use its value.
                const auto name = std::format(L"={}:", letter); // "=X:"
                auto path = CREATE_PATH_ABSOLUTE(GetEnvironmentVariable(name), 0);
                if (path.Type() == PATH_TYPE_DRIVE)
                {
                    m_root = std::move(path.m_root);
                    m_segments = std::move(path.m_segments);
                }
                // If all else fails, use the drive letter and update the environment variable.
                else
                {
                    m_root.push_back(letter); // drive letter
                    m_root.push_back(L':');
                    // Signal to update later, after the segments are processed.
                    flags |= PATH_FLAG_UPDATE_DRIVE_RELATIVE;
                }
            }
            m_pView->remove_prefix(2); // skip "X:"
        }
        return true;
    }
    return false;
}
