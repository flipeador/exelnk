#include "framework.hpp"

constexpr DWORD EXELNK_FLAGS_RAW = 0x00000001;

#define READ_ADS_STR(_) ReadAds(path, _).value_or(L"")
#define READ_ADS_INT(_1, _2) StrToInt(READ_ADS_STR(_1)).value_or(_2)

#define CHECK_ERROR(e)                           \
    if (!(e))                                    \
        return PrintSystemError(GetLastError());

static auto PrintSystemError(DWORD error)
{
    if (error)
        PRINTE(L"ERROR: {}\t{}", error, SystemErrorToString(error));
    return error;
}

static auto ReadAds(StrView path, StrView name)
{
    return File::ReadText(std::format(L"{}:{}", path, name));
}

static auto WriteAds(StrView path, StrView name, StrView text)
{
    return File::WriteText(std::format(L"{}:{}", path, name), text);
}

INT wmain(INT argc, PWSTR argv[])
{
    auto consoleProcessCount = CheckConsoleWindow();

    std::vector<String> args;
    for (INT i = 1; i < argc; ++i)
        args.push_back(argv[i]);

    auto path = GetModuleFileName(nullptr);

    // Write alternate data stream (ADS).
    if (args.size() >= 3 && args[0] == L":SET:")
    {
        CHECK_ERROR(WriteAds(path, args[1], args[2]));
        return NO_ERROR;
    }

    auto file = READ_ADS_STR(L"file");
    auto wdir = READ_ADS_STR(L"wdir");
    auto flags = (DWORD)READ_ADS_INT(L"flags", 0);

    if (args.size() && args[0] == L":RAW:")
    {
        args.erase(args.begin());
        flags |= EXELNK_FLAGS_RAW;
    }

    if (file.empty())
    {
        String exe = Path(path).filename();
        PRINT(L"INFO: file not set, run:");
        PRINT(L"\t{} :SET: file <path>", exe);
        return NO_ERROR;
    }

    auto pcwd = wdir.empty() ? nullptr : wdir.data();
    auto scmd = (WORD)READ_ADS_INT(L"scmd", SW_NORMAL);

    String cmdl; // command line
    AppendArgument(cmdl, Path(file).filename().wstring());
    AppendArgument(cmdl, READ_ADS_STR(L"args"), TRUE);
    for (const auto& arg : args)
        AppendArgument(cmdl, arg, BITALL(flags, EXELNK_FLAGS_RAW));

    PROCESS_INFORMATION pi { };
    STARTUPINFOW si { .cb = sizeof(STARTUPINFOW), .dwFlags = STARTF_USESHOWWINDOW, .wShowWindow = scmd };
    CHECK_ERROR(CreateProcessW(file.data(), cmdl.data(), nullptr, nullptr, FALSE, 0, nullptr, pcwd, &si, &pi));
    
    DWORD exitCode = NO_ERROR;

    if (consoleProcessCount > 1)
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exitCode);
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return PrintSystemError(exitCode);
}
