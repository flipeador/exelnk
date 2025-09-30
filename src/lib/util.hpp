#pragma once

#define PRINT(fmt, ...) std::wcout << std::format(fmt, __VA_ARGS__) << std::endl
#define PRINT_ERROR(fmt, ...) std::wcerr << std::format(fmt, __VA_ARGS__) << std::endl

#define BITALL(value, bits) (((value) & (bits)) == (bits))

size_t ClampIndex(int64_t i, size_t size);
Optional<int64_t> StrToInt(StrView str, INT base = 10);
bool StrEqual(StrView s1, StrView s2, bool icase = false);
String SystemErrorToString(DWORD error);
String GetModulePath(HMODULE hModule);
String GetCurrentDirectory();
DWORD EnumerateFiles(StrView pattern, const Function<DWORD(WIN32_FIND_DATA*)>& fn);
String GetEnvironmentVariable(StrView name);
BOOL SetEnvironmentVariable(StrView name, Optional<StrView> value);
String& AppendArgument(String& str, StrView arg, BOOL raw = FALSE);
