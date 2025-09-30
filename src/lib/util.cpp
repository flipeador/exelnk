#include "../framework.hpp"

size_t ClampIndex(int64_t i, size_t size)
{
    const auto n = std::min(size, (size_t)std::abs(i));
    return i < 0 ? size - n : n;
}

Optional<int64_t> StrToInt(StrView str, INT base)
{
    wchar_t* end;
    auto i = std::wcstoll(str.data(), &end, base);
    if (str.data() == end) return std::nullopt;
    return i;
}

bool StrEqual(StrView s1, StrView s2, bool icase)
{
    if (!icase) return s1 == s2;
    return std::ranges::equal(s1, s2,
        [](wchar_t c1, wchar_t c2) -> bool {
            return towupper(c1) == towupper(c2);
        }
    );
}

String SystemErrorToString(DWORD error)
{
    PWSTR buffer = nullptr;
    auto size = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_ALLOCATE_BUFFER,
        nullptr, error, 0, (PWSTR)&buffer, 0, nullptr);
    while (size && iswspace(buffer[size - 1])) --size;
    String message(buffer, size);
    LocalFree(buffer);
    return message;
}

String GetModulePath(HMODULE hModule)
{
    String str;
    str.resize_and_overwrite(PATH_MAX - 1,
        [&](wchar_t* ptr, size_t count) -> size_t {
            return GetModuleFileNameW(hModule, ptr, (DWORD)count);
        }
    );
    return str;
}

String GetCurrentDirectory()
{
    String path;
    path.resize_and_overwrite(PATH_MAX - 1,
        [&](wchar_t* ptr, size_t count) -> size_t {
            return GetCurrentDirectoryW((DWORD)count, ptr);
        }
    );
    return path;
}

DWORD EnumerateFiles(StrView path, const Function<DWORD(WIN32_FIND_DATA*)>& fn)
{
    WIN32_FIND_DATA data;
    auto hFindFile = FindFirstFileExW(path.data(), FindExInfoBasic, &data, FindExSearchNameMatch, nullptr, 0);
    if (hFindFile == INVALID_HANDLE_VALUE)
        return GetLastError();
    DWORD error = NO_ERROR;
    for (;;)
    {
        StrView name(data.cFileName);
        if (name != L"." && name != L"..")
        {
            error = fn(&data);
            if (error != NO_ERROR)
                break;
        }
        if (!FindNextFileW(hFindFile, &data))
        {
            error = GetLastError();
            break;
        }
    }
    FindClose(hFindFile);
    return error;
}

String GetEnvironmentVariable(StrView name)
{
    String value;
    value.resize_and_overwrite(
        GetEnvironmentVariableW(name.data(), nullptr, 0),
        [&](wchar_t* ptr, size_t count) -> size_t {
            return GetEnvironmentVariableW(name.data(), ptr, (DWORD)count);
        }
    );
    return value;
}

BOOL SetEnvironmentVariable(StrView name, Optional<StrView> value)
{
    return SetEnvironmentVariableW(name.data(), value ? value->data() : nullptr);
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

    if (raw || arg.find_first_of(L" \t\"") == arg.npos)
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
