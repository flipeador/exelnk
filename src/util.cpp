#include "framework.hpp"

File::File(StrView path, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes)
{
    m_hFile = CreateFileW(path.data(), desiredAccess, shareMode, nullptr, creationDisposition, flagsAndAttributes, nullptr);
}

File::~File()
{
    CloseHandle(m_hFile);
}

std::optional<size_t> File::Size() const
{
    LARGE_INTEGER size;
    if (GetFileSizeEx(m_hFile, &size))
        return size.QuadPart;
    return std::nullopt;
}

std::optional<size_t> File::Read(void* ptr, size_t bytes) const
{
    DWORD bytesRead;
    if (ReadFile(m_hFile, ptr, (DWORD)bytes, &bytesRead, nullptr))
        return bytesRead;
    return std::nullopt;
}

std::optional<size_t> File::Write(const void* ptr, size_t bytes) const
{
    DWORD bytesWritten;
    if (WriteFile(m_hFile, ptr, (DWORD)bytes, &bytesWritten, nullptr))
        return bytesWritten;
    return std::nullopt;
}

File::operator bool() const
{
    return m_hFile != INVALID_HANDLE_VALUE;
}

std::optional<String> File::ReadText(StrView path)
{
    String str;
    std::optional<size_t> r;
    if (File file(path); file)
        str.resize_and_overwrite(
            file.Size().value_or(0) / sizeof(wchar_t),
            [&](wchar_t* ptr, size_t count) -> size_t {
                r = file.Read(ptr, count * sizeof(wchar_t));
                return r.value_or(0) / sizeof(wchar_t);
            }
        );
    if (r) return str; return std::nullopt;
}

std::optional<size_t> File::WriteText(StrView path, StrView text)
{
    if (File file(path, GENERIC_WRITE, 0, CREATE_ALWAYS); file)
        return file.Write(text.data(), text.size() * sizeof(wchar_t));
    return std::nullopt;
}

std::optional<int64_t> StrToInt(StrView str, INT base)
{
    wchar_t* end;
    auto i = std::wcstoll(str.data(), &end, base);
    if (str.data() == end) return std::nullopt;
    return i;
}

// https://learn.microsoft.com/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way
String& AppendArgument(String& str, StrView arg, BOOL raw)
{
    if (raw && arg.empty())
        return str;

    if (!str.empty() && !str.ends_with(L' ') && !str.ends_with(L'\t'))
        str.push_back(L' '); // delimiter (space or tab)

    if (!raw && arg.empty())
        return str.append(L"\"\"");

    if (raw || arg.find_first_of(L" \t\"") == String::npos)
        return str.append(arg);

    str.push_back(L'"');
    for (auto it = arg.begin(); ; ++it)
    {
        size_t backslashes = 0;
        while (it != arg.end() && *it == L'\\')
        {
            ++it;
            ++backslashes;
        }
        if (it == arg.end())
        {
            str.append(backslashes * 2, L'\\');
            break;
        }
        if (*it == L'"')
            backslashes = backslashes * 2 + 1;
        str.append(backslashes, L'\\');
        str.push_back(*it);
    }
    str.push_back(L'"');

    return str;
}

String SystemErrorToString(DWORD error)
{
    PWSTR buffer = nullptr;
    auto size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, 0, (PWSTR)&buffer, 0, nullptr);
    String message(buffer, size);
    LocalFree(buffer);
    return message;
}

String GetModulePath(HMODULE hModule)
{
    String str;
    str.resize_and_overwrite(LONG_MAXPATH,
        [&](wchar_t* ptr, size_t count) -> size_t {
            return GetModuleFileNameW(hModule, ptr, (DWORD)count);
        }
    );
    return str;
}
