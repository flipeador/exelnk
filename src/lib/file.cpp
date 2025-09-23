#include "../framework.hpp"

#define CHECK_DWORD_OVERFLOW(n)       \
    if ((n) > UINT_MAX)               \
        throw std::overflow_error("");

File::File(StrView path, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes)
{
    m_hFile = CreateFileW(path.data(), desiredAccess, shareMode, nullptr, creationDisposition, flagsAndAttributes, nullptr);
}

File::~File()
{
    Close();
}

void File::Close()
{
    if (IsOpen())
    {
        CloseHandle(m_hFile);
        m_hFile = nullptr;
    }
}

bool File::IsOpen() const
{
    return m_hFile && m_hFile != INVALID_HANDLE_VALUE;
}

Optional<size_t> File::Size() const
{
    LARGE_INTEGER size;
    if (GetFileSizeEx(m_hFile, &size))
        return size.QuadPart;
    return std::nullopt;
}

Optional<size_t> File::Read(void* ptr, size_t bytes) const
{
    CHECK_DWORD_OVERFLOW(bytes);
    DWORD bytesRead;
    if (ReadFile(m_hFile, ptr, (DWORD)bytes, &bytesRead, nullptr))
        return bytesRead;
    return std::nullopt;
}

Optional<size_t> File::Write(const void* ptr, size_t bytes) const
{
    CHECK_DWORD_OVERFLOW(bytes);
    DWORD bytesWritten;
    if (WriteFile(m_hFile, ptr, (DWORD)bytes, &bytesWritten, nullptr))
        return bytesWritten;
    return std::nullopt;
}

File::operator bool() const
{
    return IsOpen();
}

Optional<String> File::ReadText(StrView path)
{
    String str;
    Optional<size_t> r;
    if (File file(path); file)
        str.resize_and_overwrite(
            file.Size().value_or(0) / sizeof(wchar_t),
            [&](wchar_t* ptr, size_t count) -> size_t {
                r = file.Read(ptr, count * sizeof(wchar_t));
                return r.value_or(0) / sizeof(wchar_t);
            }
        );
    if (r) return str; return std::nullopt;
}

Optional<size_t> File::WriteText(StrView path, StrView text)
{
    if (File file(path, GENERIC_WRITE, 0, CREATE_ALWAYS); file)
        return file.Write(text.data(), text.size() * sizeof(wchar_t));
    return std::nullopt;
}
