#pragma once

constexpr uint16_t PATH_MAX = UNICODE_STRING_MAX_CHARS; // 32767

constexpr uint8_t PATH_TYPE_ROOTED         = 1; // "\"
constexpr uint8_t PATH_TYPE_RELATIVE       = 2; // "."
constexpr uint8_t PATH_TYPE_DRIVE_ABSOLUTE = 3; // "X:" | "\\?\X:"
constexpr uint8_t PATH_TYPE_DRIVE_RELATIVE = 4; // "X:."
constexpr uint8_t PATH_TYPE_UNC            = 5; // "\\" | "\\?\UNC"
constexpr uint8_t PATH_TYPE_DEVICE         = 6; // "\\.\"
constexpr uint8_t PATH_TYPE_ROOT_DEVICE    = 7; // "\\?\"

constexpr uint32_t PATH_FLAG_IGNORE_ROOTED         = 1 << 0;
constexpr uint32_t PATH_FLAG_IGNORE_RELATIVE       = 1 << 1;
constexpr uint32_t PATH_FLAG_IGNORE_DRIVE_RELATIVE = 1 << 2;
constexpr uint32_t PATH_FLAG_IGNORE_SEGMENTS       = 1 << 3;

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

    uint8_t Type() const;
    StrView Name() const;
    StrView ToString(int64_t nseg = INT64_MAX, String* stream = nullptr) const;
    DWORD Resolve(size_t i = 0);
    void MakeAbsolute();

    bool IsDevice() const;
    bool IsAbsolute() const;
    bool IsRelative() const;

    bool operator==(const Path& rhs) const;
    Path& operator/=(const Path& rhs);

    operator PCWSTR() const;
    operator StrView() const;

    static bool IsPattern(StrView path);
private:
    wchar_t At(size_t) const;
    bool IsSep(size_t) const;
    void MoveFrom(Path&, bool);

    StrView* m_pView;
    mutable String m_path;

    bool m_endsWithSep = false; // Whether the path ends with a separator.
    uint8_t m_type = 0;         // path type
    String m_server;            // UNC server (domain name or IP address)
    String m_root;              // UNC share name | drive letter
    Vector<String> m_segments;  // components / elements / directories / filename
};
