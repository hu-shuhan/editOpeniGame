#include "iGameResourcePath.h"

#include "iGameFileSystem.h"

#include <cstdint>
#include <filesystem>
#include <system_error>

#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#elif defined(__linux__)
#  include <unistd.h>
#elif defined(__APPLE__)
#  include <mach-o/dyld.h>
#  include <vector>
#endif

namespace iGame {

namespace {

// Absolute directory containing the running executable.  Falls back to the
// current working directory when it cannot be determined (e.g. Emscripten).
std::filesystem::path ExecutableDirectory() {
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH] = {0};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        const std::filesystem::path parent =
                std::filesystem::path(buffer).parent_path();
        if (!parent.empty()) { return parent; }
    }
#elif defined(__linux__)
    char buffer[4096] = {0};
    const ssize_t length =
            readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length > 0) {
        buffer[length] = '\0';
        const std::filesystem::path parent =
                std::filesystem::path(buffer).parent_path();
        if (!parent.empty()) { return parent; }
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    if (size > 0 && size < (1u << 20)) {
        std::vector<char> buffer(size);
        if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
            const std::filesystem::path parent =
                    std::filesystem::path(buffer.data()).parent_path();
            if (!parent.empty()) { return parent; }
        }
    }
#endif
    std::error_code ec;
    return std::filesystem::current_path(ec);
}

bool FileExists(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec) && !ec;
}

} // namespace

std::string GetExecutableDirectory() {
    return FileSystem::PathToUtf8(ExecutableDirectory());
}

std::string ResolveResourcePath(const std::string& relativePath) {
#if defined(__EMSCRIPTEN__)
    return relativePath;
#else
    const std::filesystem::path rel =
            FileSystem::PathFromUtf8(relativePath).lexically_normal();

    std::error_code ec;
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    const std::filesystem::path exeDir = ExecutableDirectory();

    // 1) Working directory — preserves existing behaviour for deployments that
    //    already run with "Resources" in the CWD (e.g. ctest / the desktop app).
    const std::filesystem::path cwdCandidate = cwd / rel;
    if (FileExists(cwdCandidate)) {
        return FileSystem::PathToUtf8(cwdCandidate);
    }

    // 2) Next to the executable.
    const std::filesystem::path exeCandidate = exeDir / rel;
    if (FileExists(exeCandidate)) {
        return FileSystem::PathToUtf8(exeCandidate);
    }

    // 3) One level above the executable (multi-config Debug/Release layout).
    const std::filesystem::path exeParentCandidate = exeDir.parent_path() / rel;
    if (FileExists(exeParentCandidate)) {
        return FileSystem::PathToUtf8(exeParentCandidate);
    }

    // Not found: return the original path so callers log the familiar value.
    return relativePath;
#endif
}

} // namespace iGame
