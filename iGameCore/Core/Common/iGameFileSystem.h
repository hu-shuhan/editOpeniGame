#ifndef iGameFileSystem_h
#define iGameFileSystem_h

#include <cstdio>
#include <filesystem>
#include <string>

namespace iGame::FileSystem {

// File paths crossing the public iGameCore API are UTF-8 encoded.  Windows
// file APIs require UTF-16, while POSIX systems use the UTF-8 bytes directly.
inline std::filesystem::path PathFromUtf8(const std::string& path) {
#if defined(_WIN32)
    return std::filesystem::u8path(path);
#else
    return std::filesystem::path(path);
#endif
}

inline std::string PathToUtf8(const std::filesystem::path& path) {
#if defined(__cpp_char8_t)
    const auto value = path.u8string();
    return std::string(value.begin(), value.end());
#else
    return path.u8string();
#endif
}

inline FILE* OpenFile(const std::string& path, const char* mode) {
#if defined(_WIN32)
    try {
        std::wstring wideMode;
        while (*mode != '\0') {
            wideMode.push_back(static_cast<wchar_t>(*mode));
            ++mode;
        }
        return _wfopen(PathFromUtf8(path).c_str(), wideMode.c_str());
    } catch (const std::filesystem::filesystem_error&) {
        return nullptr;
    }
#else
    return std::fopen(path.c_str(), mode);
#endif
}

} // namespace iGame::FileSystem

#endif
