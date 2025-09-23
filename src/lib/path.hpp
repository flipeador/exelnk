#pragma once

constexpr uint16_t PATH_MAX = UNICODE_STRING_MAX_CHARS; // 32767

constexpr uint8_t PATH_TYPE_UNC         = 1; // "\\?\UNC" | "\\"
constexpr uint8_t PATH_TYPE_DRIVE       = 2; // "\\?\X:"  | "X:" | "X:." | "." | "\"
constexpr uint8_t PATH_TYPE_DEVICE      = 3; // "\\.\"
constexpr uint8_t PATH_TYPE_ROOT_DEVICE = 4; // "\\?\"

constexpr uint32_t PATH_FLAG_IGNORE_ROOTED         = 1 << 0;
constexpr uint32_t PATH_FLAG_IGNORE_RELATIVE       = 1 << 1;
constexpr uint32_t PATH_FLAG_IGNORE_DRIVE_RELATIVE = 1 << 2;
constexpr uint32_t PATH_FLAG_UPDATE_DRIVE_RELATIVE = 1 << 3;
constexpr uint32_t PATH_FLAG_IGNORE_SEGMENTS       = 1 << 4;

/**
 * A simple class for long paths in Windows.
 * Reference:
 * - https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file
 * - https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
 * - https://googleprojectzero.blogspot.com/2016/02/the-definitive-guide-on-win32-to-nt.html
 */
class Path final
{
public:
    Path(StrView path, uint32_t flags = 0);

    StrView Name() const;
    uint8_t Type() const;
    DWORD Resolve(size_t i = 0);
    StrView ToString(int64_t nseg = INT64_MAX) const;

    operator PCWSTR() const;
    operator StrView() const;

    static bool IsDriveLetter(wchar_t c);
private:
    wchar_t At(size_t index) const;
    bool Is(size_t index, wchar_t c) const;
    bool IsSep(size_t index) const;
    bool ParseDrive(uint32_t& flags);

    StrView* m_pView;
    bool m_endsWithSep = false;  // Whether the path ends with a separator.
    uint8_t m_type = 0;          // path type
    String m_server;             // UNC server address (domain name or IP address)
    String m_root;               // UNC share name | drive leter
    Vector<String> m_segments;   // resource elements
    mutable String m_path;       // full path name with "\\.\" or "\\?\"
};
