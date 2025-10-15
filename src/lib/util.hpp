#pragma once

#define BITALL(value, bits) (((value) & (bits)) == (bits))

#define PRINT(fmt, ...) \
	std::wcout << std::format(fmt, __VA_ARGS__) << std::endl

size_t ClampIndex(int64_t i, size_t size);
Optional<int64_t> StrToInt(StrView str, INT base = 10);
bool StrEqual(StrView s1, StrView s2, bool icase = false);
String SystemErrorToString(DWORD error);
String GetModulePath(HMODULE hModule);
String GetCurrentDirectory();
String GetEnvironmentVariable(StrView name);
BOOL SetEnvironmentVariable(StrView name, Optional<StrView> value);
String& AppendArgument(String& str, StrView arg, BOOL raw = FALSE);
DWORD EnumerateFiles(StrView path, const Function<DWORD(WIN32_FIND_DATA*)>& fn);
DWORD EnumerateStreams(StrView path, const Function<DWORD(WIN32_FIND_STREAM_DATA*)>& fn);
