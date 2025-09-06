# ExeLnk

Use an `exe` file as a shortcut file or app execution alias:
- Serves as a lightweight intermediary to run executables seamlessly.
- Manages program dependencies and dynamic DLL loading correctly.
- Simplifies environment setup by preventing uncontrolled `PATH` growth.

```bash
exelnk.exe :SET: file  <path>  # set target file
exelnk.exe :SET: args  <path>  # set command line
exelnk.exe :SET: wdir  <path>  # set working directory
exelnk.exe :SET: scmd  <scmd>  # 1=normal | 2=min | 3=max
exelnk.exe :SET: flags <flags> # 0 | 1=:RAW:
```

```bash
exelnk.exe [...args] # execute the target file
```

Use `:RAW:` to disable argument parsing, useful with [`cmd.exe`][cmd]:

```bash
exelnk.exe :RAW: [...args]
```

## Download

<https://github.com/flipeador/exelnk/releases>

<!-- Reference Links -->
[isl]: https://learn.microsoft.com/windows/win32/api/shobjidl_core/nn-shobjidl_core-ishelllinkw
[cmd]: https://learn.microsoft.com/en-us/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way#:~:text=cmd.exe
