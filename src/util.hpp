#pragma once

#define PRINT(fmt, ...) std::wcout << std::format(fmt, __VA_ARGS__) << std::endl
#define PRINTE(fmt, ...) std::wcerr << std::format(fmt, __VA_ARGS__) << std::endl

#define BITALL(value, bits) (((value) & (bits)) == (bits))

class File final
{
public:
    explicit File(
        StrView fileName,
        DWORD desiredAccess = GENERIC_READ,
        DWORD shareMode = FILE_SHARE_READ,
        DWORD creationDisposition = OPEN_EXISTING,
        DWORD flagsAndAttributes = FILE_ATTRIBUTE_NORMAL);
    ~File();

    std::optional<size_t> Size();
    std::optional<size_t> Read(void* ptr, size_t bytes);
    std::optional<size_t> Write(const void* ptr, size_t bytes);

    operator bool();

    static std::optional<String> ReadText(StrView path);
    static std::optional<size_t> WriteText(StrView path, StrView text);
private:
    HANDLE m_hFile = nullptr;
};

std::optional<int64_t> StrToInt(StrView str, int base = 10);

DWORD CheckConsoleWindow();
String SystemErrorToString(DWORD error);
String GetModuleFileName(HMODULE hModule);
String& AppendArgument(String& str, StrView arg, BOOL raw = FALSE);
