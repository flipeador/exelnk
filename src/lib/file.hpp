#pragma once

class File final
{
public:
    explicit File(
        StrView path,
        DWORD desiredAccess = GENERIC_READ,
        DWORD shareMode = FILE_SHARE_READ,
        DWORD creationDisposition = OPEN_EXISTING,
        DWORD flagsAndAttributes = FILE_ATTRIBUTE_NORMAL);
    ~File();

    void Close();
    bool IsOpen() const;
    Optional<size_t> Size() const;
    Optional<size_t> Read(void* ptr, size_t bytes) const;
    Optional<size_t> Write(const void* ptr, size_t bytes) const;

    operator bool() const;

    static Optional<String> ReadText(StrView path);
    static Optional<size_t> WriteText(StrView path, StrView text);
private:
    HANDLE m_hFile = nullptr;
};
