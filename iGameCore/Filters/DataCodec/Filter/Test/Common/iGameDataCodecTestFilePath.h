#ifndef iGameDataCodecTestFilePath_h
#define iGameDataCodecTestFilePath_h

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace iGame::datacodec_test {
using namespace ::datacodec;
using namespace ::datacodec::test;

[[nodiscard]] inline std::string PathToUtf8String(const std::filesystem::path& path) {
#if defined(_WIN32)
    const auto wide = path.wstring();
    if (wide.empty()) {
        return {};
    }
    const auto size = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (size <= 0) {
        return {};
    }
    std::string utf8(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        utf8.data(),
        size,
        nullptr,
        nullptr);
    return utf8;
#else
    return path.string();
#endif
}

[[nodiscard]] inline std::filesystem::path PathFromUtf8String(const std::string& value) {
#if defined(_WIN32)
    if (value.empty()) {
        return {};
    }
    const auto size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) {
        return std::filesystem::path(value);
    }
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), wide.data(), size);
    return std::filesystem::path(wide);
#else
    return std::filesystem::path(value);
#endif
}

[[nodiscard]] inline std::string ToLowerAscii(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](const unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

} // namespace iGame::datacodec_test

#endif
