#ifndef iGameExternalProcess_h
#define iGameExternalProcess_h

#include "iGameFileSystem.h"
#include "iGameProgressObserver.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cwchar>
#include <filesystem>
#include <functional>
#include <string>
#include <system_error>
#include <utility>
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

/**
 * 把「父进程环境」复制一份，并把 TEMP / TMP / TMPDIR 改写成 @p tempDirectory。
 *
 * 为什么需要它（这是本文件里最不直观的一段，值得读）：
 * `Resources\pyCCMLib\ccm_to_vtu_converter.exe` 这类转换器是 **onefile 自解包**程序
 * （Nuitka / PyInstaller 打包）。它们启动时先把自带的 DLL 解包到 `%TEMP%` 下的
 * `onefile_*` 目录，再从那里加载。如果父进程的 TEMP 不可写（受限用户、容器/沙箱、
 * 安全软件拦截、TEMP 被重定向到只读位置……），转换器会在**解包阶段**失败，只打印：
 *
 *     Error, failed to open '<TEMP>\onefile_xxx\yyy.dll' for writing.
 *
 * 然后以非 0 退出码结束 —— 看起来像"转换器坏了"，其实与转换逻辑毫无关系。
 * 把子进程的 TEMP 指向一个我们确认可写的目录（调用方自己的 `temp/`），就能把这类
 * 环境问题整体消除，而不必要求用户去改系统 TEMP 或加杀软白名单。
 *
 * @param out 产出的环境块（每个 "NAME=VALUE" 以 NUL 结尾，整块以额外一个 NUL 结束）
 */
#if defined(_WIN32)
inline bool BuildEnvironmentWithTemp(const std::wstring& tempDirectory,
                                     std::vector<wchar_t>& out) {
    LPWCH block = GetEnvironmentStringsW();
    if (block == nullptr) { return false; }

    std::vector<std::wstring> entries;
    for (LPWCH cursor = block; *cursor != L'\0'; cursor += std::wcslen(cursor) + 1) {
        entries.emplace_back(cursor);
    }
    FreeEnvironmentStringsW(block);

    // 不区分大小写：Windows 环境变量名本身不区分大小写，TEMP / Temp / temp 都算。
    auto hasName = [](const std::wstring& entry, const wchar_t* name) {
        const size_t length = std::wcslen(name);
        return entry.size() >= length + 1 && entry[length] == L'=' &&
               _wcsnicmp(entry.c_str(), name, length) == 0;
    };
    entries.erase(std::remove_if(entries.begin(), entries.end(),
                                 [&](const std::wstring& entry) {
                                     return hasName(entry, L"TEMP") ||
                                            hasName(entry, L"TMP") ||
                                            hasName(entry, L"TMPDIR");
                                 }),
                  entries.end());

    entries.push_back(L"TEMP=" + tempDirectory);
    entries.push_back(L"TMP=" + tempDirectory);
    // ★ 必须按「名字」**不区分大小写**排序。
    //
    // 这一条不是风格问题：Windows 对 lpEnvironment 做校验，未按此规则排序的
    // 环境块会让 CreateProcessW 直接以 ERROR_INVALID_PARAMETER(87) 失败 ——
    // 而失败点看起来与"环境块"毫无关系（实测踩过：override 路径 started=0，
    // 错误码 87）。用 _wcsicmp 比较整条 "NAME=VALUE" 等价于比较 NAME：
    // 因为 '='（0x3D）小于任何名字字符，所以 "PATH=" 与 "PATHS=" 的先后仍由
    // 名字决定，与逐字符比较名字的结果一致。
    std::sort(entries.begin(), entries.end(),
              [](const std::wstring& a, const std::wstring& b) {
                  return _wcsicmp(a.c_str(), b.c_str()) < 0;
              });

    out.clear();
    for (const std::wstring& entry : entries) {
        out.insert(out.end(), entry.begin(), entry.end());
        out.push_back(L'\0');
    }
    out.push_back(L'\0');
    return true;
}
#endif

} // namespace detail

/**
 * 外部转换器（商软格式 -> PVD/VTU 的 python 打包 exe）不会上报自己的内部进度：
 * 调用方只能「启动 → 等待 → 结束」。而这段等待是在 GUI 线程里同步阻塞的，界面完全
 * 没有反馈，用户会以为软件卡死。
 *
 * 本类用「时间驱动」的渐近曲线代替真实进度：等待期间把进度从 @p begin 缓慢推进到
 * @p end（永远到不了 @p end，避免提前显示 100%），同时把「正在转换 …(12s)」写进状态栏
 * 文案，让用户看到软件在动、也已经等了多久。
 *
 * 用法（把 Ticker() 传给 Run 的第 5 个参数即可）：
 *     ExternalProcess::ConverterProgress progress("正在转换 Ansys 结果文件");
 *     ExternalProcess::Run(exe, args, returnCode, tempDir, progress.Ticker());
 *
 * 生命周期内会把 ProgressObserver 的「阻塞 GUI 线程」标记置位，让进度条控件有机会
 * 把进度真正画出来（见 ProgressObserver::SetBlockingGuiThread）。
 */
class ConverterProgress {
public:
    explicit ConverterProgress(std::string label, double begin = 0.05, double end = 0.85)
            : m_Label(std::move(label)), m_Begin(begin), m_End(std::max(begin, end)),
              m_Start(std::chrono::steady_clock::now()) {
        ProgressObserver::Instance()->SetBlockingGuiThread(true);
    }
    ~ConverterProgress() { ProgressObserver::Instance()->SetBlockingGuiThread(false); }
    ConverterProgress(const ConverterProgress&) = delete;
    ConverterProgress& operator=(const ConverterProgress&) = delete;

    // 交给 ExternalProcess::Run 的等待回调：Run 每隔约 100ms 调用一次。
    std::function<void()> Ticker() {
        return [this]() { this->Report(); };
    }

private:
    // 6 秒时间常数：前 6 秒走完约 63% 的区间，之后继续缓慢逼近上限，
    // 转换器再慢也不会「停在某个数字上一动不动」。
    static constexpr double kTimeConstantSeconds = 6.0;

    void Report() {
        const auto now = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(now - m_Start).count();
        const double fraction = m_Begin + (m_End - m_Begin) * (1.0 - std::exp(-elapsed / kTimeConstantSeconds));

        auto* observer = ProgressObserver::Instance();
        if (observer == nullptr) { return; }
        // 先文案、后进度：进度事件才是「泵一次事件并重绘」的触发点，
        // 先写文案能保证同一次重绘里文字和进度条一起更新。
        observer->UpdateText(m_Label + "…(" + std::to_string(static_cast<long long>(elapsed)) + "s)");
        observer->UpdateProgress(fraction);
    }

    std::string m_Label;
    double m_Begin;
    double m_End;
    std::chrono::steady_clock::time_point m_Start;
};

// 启动失败返回 false；成功则 returnCode 为子进程退出码（0 表示成功）。
//
// @param utf8TempDirectory 可选。非空时把子进程的 TEMP/TMP 指向该目录（不存在则创建），
//        见上方 BuildEnvironmentWithTemp 的说明。空串表示完全继承父进程环境。
// @param onWaitTick 可选。非空时不再一次性无限等待，而是每约 100ms 醒一次并调用它，
//        让调用方在等待期间上报进度/文案（转换器本身无法给出真实进度）。
//        为空时行为与原先完全一致（单次 INFINITE 等待）。
inline bool Run(const std::string& utf8Executable,
                const std::vector<std::string>& utf8Arguments,
                int& returnCode,
                const std::string& utf8TempDirectory = std::string(),
                const std::function<void()>& onWaitTick = std::function<void()>()) {
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

        // 需要时给子进程一组 TEMP/TMP 被改写过的环境块。
        std::vector<wchar_t> environmentBlock;
        void* environment = nullptr;
        if (!utf8TempDirectory.empty()) {
            const fs::path tempDirectory = FileSystem::PathFromUtf8(utf8TempDirectory);
            std::error_code createError;
            fs::create_directories(tempDirectory, createError);
            if (!detail::BuildEnvironmentWithTemp(tempDirectory.native(), environmentBlock)) {
                return false;
            }
            environment = environmentBlock.data();
        }

        STARTUPINFOW startupInfo{};
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION processInfo{};
        // ★ 传了自定义环境块就必须带 CREATE_UNICODE_ENVIRONMENT。
        //   否则 CreateProcessW 直接返回 ERROR_INVALID_PARAMETER(87) —— 实测踩过：
        //   环境块本身完全合法（同样的块加上这个标志就能启动）。
        const DWORD creationFlags = (environment != nullptr) ? CREATE_UNICODE_ENVIRONMENT : 0;
        if (!CreateProcessW(executable.c_str(), commandBuffer.data(), nullptr, nullptr,
                            FALSE, creationFlags, environment, nullptr, &startupInfo,
                            &processInfo)) {
            return false;
        }
        // 先上报一次，让进度条在转换器启动的瞬间就出现（而不是熬到第一次 tick）。
        if (onWaitTick) { onWaitTick(); }

        // 有等待回调时改成每 100ms 醒一次：一边等子进程，一边把进度/文案推给界面。
        // 没有回调时（如 CAS 读取器）保持原先的单次 INFINITE 等待，行为完全不变。
        const DWORD waitSlice = onWaitTick ? 100u : INFINITE;
        while (WaitForSingleObject(processInfo.hProcess, waitSlice) == WAIT_TIMEOUT) {
            if (onWaitTick) { onWaitTick(); }
        }

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
        // 非 Windows 上暂不支持改写子进程的临时目录：本文件的四个调用方
        // （CAS/CCM/Ansys/LsDyna 转换器）都是 Windows-only 的 exe，Linux/macOS
        // 上这条路径目前没有任何真实调用。若将来需要，用 posix_spawn + envp
        // 重写这段，而不是给 std::system 拼 "TMPDIR=..."（拼接会让引号规则变脆）。
        (void) utf8TempDirectory;
        // std::system 期间无法醒来上报进度，只能上报启动前的一次（非 Windows 路径当前无真实调用）。
        if (onWaitTick) { onWaitTick(); }
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
