#ifndef iGameExternalProcess_h
#define iGameExternalProcess_h

#include "iGameFileSystem.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

IGAME_NAMESPACE_BEGIN

/**
 * 以 UTF-8 的可执行文件路径和参数启动外部进程（如商业软件格式转换器），
 * 等待其结束并返回退出码。
 *
 * Windows 上直接走 CreateProcessW（宽字符命令行），不再经由 system()/cmd.exe：
 *  1. system() 按 ANSI 代码页解释窄字符串，含中文（UTF-8）的命令行会被转码成乱码；
 *  2. cmd.exe 对首尾引号有剥离规则，参数必须加引号才能容纳含空格的路径。
 * 各参数按 CommandLineToArgvW 规则加引号，空格与中文路径均安全。
 */
namespace ExternalProcess {

namespace detail {

// 按 CommandLineToArgvW 的规则为单个参数加引号（始终加引号，保证解析结果一致）。
inline std::wstring QuoteArgument(const std::wstring& argument) {
    std::wstring quoted;
    quoted.reserve(argument.size() + 2);
    quoted.push_back(L'"');
    std::size_t backslashes = 0;
    for (const wchar_t character : argument) {
        if (character == L'\\') {
            ++backslashes;
            continue;
        }
        if (character == L'"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            backslashes = 0;
            quoted.push_back(L'"');
        } else {
            quoted.append(backslashes, L'\\');
            backslashes = 0;
            quoted.push_back(character);
        }
    }
    quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'"');
    return quoted;
}

// POSIX shell 参数的单引号转义（仅供非 Windows 平台编译与兜底使用）。
inline std::string QuoteShellArgument(const std::string& argument) {
    std::string quoted;
    quoted.reserve(argument.size() + 2);
    quoted.push_back('\'');
    for (const char character : argument) {
        if (character == '\'') {
            quoted += "'\\''";
        } else {
            quoted.push_back(character);
        }
    }
    quoted.push_back('\'');
    return quoted;
}

} // namespace detail

// 启动失败返回 false；成功则 returnCode 为子进程退出码（0 表示成功）。
inline bool Run(const std::string& utf8Executable,
                const std::vector<std::string>& utf8Arguments,
                int& returnCode) {
    namespace fs = std::filesystem;
    returnCode = -1;
    try {
        const fs::path executable = fs::absolute(FileSystem::PathFromUtf8(utf8Executable));
#if defined(_WIN32)
        std::wstring commandLine = detail::QuoteArgument(executable.native());
        for (const auto& argument : utf8Arguments) {
            commandLine.push_back(L' ');
            commandLine += detail::QuoteArgument(FileSystem::PathFromUtf8(argument).native());
        }

        // CreateProcessW 会改写该缓冲区，因此不能传只读字符串。
        std::vector<wchar_t> commandBuffer(commandLine.begin(), commandLine.end());
        commandBuffer.push_back(L'\0');

        STARTUPINFOW startupInfo{};
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION processInfo{};
        if (!CreateProcessW(executable.c_str(), commandBuffer.data(), nullptr, nullptr,
                            FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo)) {
            return false;
        }
        WaitForSingleObject(processInfo.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(processInfo.hProcess, &exitCode);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        returnCode = static_cast<int>(exitCode);
        return true;
#else
        std::string commandLine = detail::QuoteShellArgument(FileSystem::PathToUtf8(executable));
        for (const auto& argument : utf8Arguments) {
            commandLine.push_back(' ');
            commandLine += detail::QuoteShellArgument(argument);
        }
        returnCode = std::system(commandLine.c_str());
        return true;
#endif
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

} // namespace ExternalProcess

IGAME_NAMESPACE_END
#endif
