# ExeLnk

Use an `exe` file as a shortcut file or app execution alias:
- Serves as a lightweight intermediary to run executables seamlessly.
- Manages program dependencies and dynamic DLL loading correctly.
- Simplifies [environment][env] setup by preventing uncontrolled `PATH` growth.
- Resolves [wildcards](#wildcard-patterns) with [FindFirstFileEx][fff] recursively for each path segment.

## Usage

### Configuration

```bash
exelnk.exe :SET: file  <path>  # set target file
exelnk.exe :SET: args  <cmdl>  # set command line
exelnk.exe :SET: wdir  <path>  # set working directory
exelnk.exe :SET: scmd  <scmd>  # 1=normal | 2=min | 3=max
exelnk.exe :SET: flags <flags> # 0 | 1=:RAW:
```

Use wildcards to link to files with version numbers in the path:

```bash
# Example with an app that contains the version number in the path.
exelnk.exe :SET: file "C:\Program Files\Python\3.*\python.exe"
```

### Execution

Execute the target file:

```bash
exelnk.exe [...args]
```

Use `:RAW:` to disable argument parsing (useful with [CMD][cmd]):

```bash
exelnk.exe :RAW: [...args]
```

Use `:FIND:` to resolve a path (for testing purposes):

```bash
exelnk.exe :FIND: <path>

# Example:
exelnk.exe :FIND: "C:/pro*les/win*der/msmpeng.e?e"
# Possible output:
# [15106] User stopped resource enumeration.
# "\\?\C:\Program Files\Windows Defender\MsMpEng.exe"
```

Use `:DLL:` to call functions from a DLL (similar to [`rundll32`][rdl]):

```bash
exelnk.exe :DLL: <path> <function> [...args]
```

```cpp
int function(int argc, wchar_t* argv[]);
```

## Wildcard Patterns

You can use the following [wildcard characters][wil]:

| Wildcard | Description |
| :---: | --- |
| `?` | Matches a single character. |
| `*` | Matches zero or more characters. |

<details>
<summary><h3>How it works</h4></summary>

Performs a recursive [depth-first search][dfs] to resolve wildcard patterns in the path.

At each path segment, it [enumerates][fff] matching directories or files:
- Substitutes the current path segment with the candidate name, and recurses into the next level if the item is a directory.
- If the target path does not exist at some depth, backtracks and continues with the next candidate from the previous level.
- The search terminates as soon as a full valid path is found, or exhausts all options if none exists.

For example, given the pattern `C:\XYZ_*\File.txt` and the following file structure:

```
C:\
 ├─ XYZ_a\
 ├─ XYZ_b\
 │  └─ File.txt
 └─ XYZ_c\
    └─ File.txt
```

The algorithm behaves as follows:

1. `C:\XYZ_a\` found → `File.txt` not found → backtrack.
2. `C:\XYZ_b\` found → `File.txt` found → stop (full path resolved).

</details>

## Download

<https://github.com/flipeador/exelnk/releases>

## Build

Install [Visual Studio][vs] 2026; add the workloads and components required for developing with C++.

[Download][downl] or [clone][clone] this repository to your local computer, open [`exelnk.slnx`](src/exelnk.slnx) with Visual Studio.

Select the **Release** configuration, right click the `exelnk` project and **Build** it.

<!-- Reference Links -->
[vs]: https://visualstudio.microsoft.com

[dfs]: https://en.wikipedia.org/wiki/Depth-first_search
[env]: https://github.com/flipeador/environment-variables-editor
[fff]: https://learn.microsoft.com/windows/win32/api/fileapi/nf-fileapi-findfirstfileexw
[isl]: https://learn.microsoft.com/windows/win32/api/shobjidl_core/nn-shobjidl_core-ishelllinkw
[wil]: https://web.archive.org/web/20230406111635/https://learn.microsoft.com/en-us/archive/blogs/jeremykuhne/wildcards-in-windows
[cmd]: https://learn.microsoft.com/en-us/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way#:~:text=cmd.exe
[rdl]: https://learn.microsoft.com/windows-server/administration/windows-commands/rundll32

[downl]: https://github.com/flipeador/exelnk/archive/refs/heads/main.zip
[clone]: https://docs.github.com/en/repositories/creating-and-managing-repositories/cloning-a-repository
