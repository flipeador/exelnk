#include "framework.hpp"

constexpr uint32_t EXELNK_FLAG_RAW = 1 << 0;

#define READ_ADS_STR(_) ReadAds(modulePath, _).value_or(L"")
#define READ_ADS_INT(_1, _2) StrToInt(READ_ADS_STR(_1)).value_or(_2)

#define CHECK_ERROR(e)                           \
    if (!(e))                                    \
        return PrintSystemError(GetLastError());

static auto PrintSystemError(DWORD error)
{
    if (error)
        PRINT_ERROR(L"ERROR: {}\t{}", error, SystemErrorToString(error));
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
    DWORD consoleProcessList; // https://stackoverflow.com/a/64842606/14822191
    auto isFinalProcess = GetConsoleProcessList(&consoleProcessList, 1) < 2;

    //if (isFinalProcess)
    //    ShowWindow(GetConsoleWindow(), SW_HIDE);

    Vector<StrView> args;
    for (INT i = 1; i < argc; ++i)
        args.push_back(argv[i]);

    const auto modulePath = Path(GetModulePath(nullptr));
    const auto moduleName = modulePath.Name();

    if (args.size() >= 1)
    {
        // Write alternate data stream (ADS).
        if (args[0] == L":SET:")
        {
            if (args.size() >= 3)
            {
                const auto result = WriteAds(modulePath, args[1], args[2]);
                const auto error = result ? NO_ERROR : GetLastError();
                PRINT(L"[{}] {}", error, SystemErrorToString(error));
            }
            else
            {
                PRINT(L"Usage:\n\t{} :SET: <name> <value>", moduleName);
            }
            return NO_ERROR;
        }
        // Resolve path.
        if (args[0] == L":FIND:")
        {
            if (args.size() >= 2)
            {
                Path path(args[1]);
                const auto error = path.Resolve();
                PRINT(L"[{}] {}\n\"{}\"", error, SystemErrorToString(error), (StrView)path);
            }
            else
            {
                PRINT(L"Usage:\n\t{} :FIND: <path>", moduleName);
            }
            return NO_ERROR;
        }
    }

    auto file = Path(READ_ADS_STR(L"file"));
    auto wdir = Path(READ_ADS_STR(L"wdir"));
    auto scmd = (WORD)READ_ADS_INT(L"scmd", SW_NORMAL);
    auto flags = (uint32_t)READ_ADS_INT(L"flags", 0);
    
    if (args.size() && args[0] == L":RAW:")
    {
        args.erase(args.begin());
        flags |= EXELNK_FLAG_RAW;
    }

    if (!file.Type())
    {
        PRINT(L"INFO: file not set or invalid, run:");
        PRINT(L"\t{} :SET: file <path>", moduleName);
        return NO_ERROR;
    }

    // Resolve path wildcards with `FindFirstFileExW`.
    // This is done recursively for each path segment.
    file.Resolve();
    wdir.Resolve();

    // Build command line.
    String cmdl;
    AppendArgument(cmdl, file.Name());
    AppendArgument(cmdl, READ_ADS_STR(L"args"), TRUE);
    for (const auto& arg : args)
        AppendArgument(cmdl, arg, BITALL(flags, EXELNK_FLAG_RAW));

    DWORD creationFlags = 0;

    if (isFinalProcess)
        creationFlags |= CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP;

    PROCESS_INFORMATION pi { };
    STARTUPINFOW si { .cb = sizeof(STARTUPINFOW), .dwFlags = STARTF_USESHOWWINDOW, .wShowWindow = scmd };
    CHECK_ERROR(CreateProcessW(file, cmdl.data(), nullptr, nullptr, FALSE, creationFlags, nullptr, wdir, &si, &pi));

    DWORD exitCode = NO_ERROR;

    if (!isFinalProcess)
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exitCode);
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return exitCode;
}
