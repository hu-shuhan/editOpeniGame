#pragma once

#include "iGameObject.h"
#include <chrono>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <source_location>

IGAME_NAMESPACE_BEGIN

namespace Logger
{

#define LOGGER_FOREACH_LOG_LEVEL(f)                                            \
    f(Trace) f(Debug) f(Info) f(Warn) f(Error) f(Fatal)

enum class LogLevel : std::uint8_t {
#define _FUNCTION(name) name,
    LOGGER_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
};

namespace details
{

#if defined(__linux__) || defined(__APPLE__)
inline constexpr char k_level_ansi_colors[(std::uint8_t) LogLevel::fatal +
                                          1][8] = {
        "\E[37m", "\E[35m", "\E[32m", "\E[34m", "\E[33m", "\E[31m", "\E[31;1m",
};
inline constexpr char k_reset_ansi_color[4] = "\E[m";
#define _LOGGER_IF_HAS_ANSI_COLORS(x) x
#else
#define _LOGGER_IF_HAS_ANSI_COLORS(x)
inline constexpr char k_level_ansi_colors[(std::uint8_t) LogLevel::Fatal + 1]
                                         [1] = {
                                                 "", "", "", "", "", "",
};
inline constexpr char k_reset_ansi_color[1] = "";
#endif

inline std::string LogLevelName(LogLevel lev) {
    switch (lev) {
#define _FUNCTION(name)                                                        \
    case LogLevel::name:                                                       \
        return #name;
        LOGGER_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
    }
    return "unknown";
}

inline LogLevel LogLevelFromName(std::string lev) {
#define _FUNCTION(name)                                                        \
    if (lev == #name) return LogLevel::name;
    LOGGER_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
    return LogLevel::Info;
}

template<class T>
struct with_source_location {
private:
    T inner;
    std::source_location loc;

public:
    template<class U>
        requires std::constructible_from<T, U>
    consteval with_source_location(
            U&& inner,
            std::source_location loc = std::source_location::current())
        : inner(std::forward<U>(inner)), loc(std::move(loc)) {}
    constexpr T const& format() const { return inner; }
    constexpr std::source_location const& location() const { return loc; }
};

inline LogLevel g_MaxLevel = []() -> LogLevel {
#ifdef _MSC_VER // Use _dupenv_s for MSVC
    char* lev = nullptr;
    size_t len = 0;
    if (_dupenv_s(&lev, &len, "LOGGER_LEVEL") == 0 && lev) {
        LogLevel level = details::LogLevelFromName(lev);
        free(lev); // Free dynamically allocated memory
        return level;
    }
#else // For non-MSVC platforms, use getenv
    if (const char* lev = std::getenv("LOGGER_LEVEL")) {
        return details::LogLevelFromName(lev);
    }
#endif
    return LogLevel::Info; // Default log level
}();


inline std::ofstream g_LogFile{
#ifdef _MSC_VER
        []() {
            char* path = nullptr;
            size_t len = 0;
            if (_dupenv_s(&path, &len, "LOGGER_FILE") == 0 && path != nullptr) {
                std::ofstream logFile(path, std::ios::app);
                free(path); // Free dynamically allocated memory
                return logFile;
            }
            return std::ofstream{};
        }()
#else
        (std::getenv("LOGGER_FILE")
                 ? std::ofstream(std::getenv("LOGGER_FILE"), std::ios::app)
                 : std::ofstream{})
#endif
};

inline void OutputLog(LogLevel lev, std::string msg,
                      std::source_location const& loc) {
    std::chrono::zoned_time now{std::chrono::current_zone(),
                                std::chrono::system_clock::now()};
    msg = std::format("{} {}:{} [{}] {}", now, loc.file_name(), loc.line(),
                      details::LogLevelName(lev), msg);
    if (g_LogFile) {
        g_LogFile << msg + '\n';
        g_LogFile.flush();
    }
    if (lev >= g_MaxLevel) {
        std::cout << _LOGGER_IF_HAS_ANSI_COLORS(
                             k_level_ansi_colors[(std::uint8_t) lev] +)
                                     msg _LOGGER_IF_HAS_ANSI_COLORS(
                                             +k_reset_ansi_color) +
                             '\n';
    }
}


} // namespace details

inline void SetLogFile(std::string path) {
    details::g_LogFile = std::ofstream(path, std::ios::app);
}

inline void SetLogLevel(LogLevel lev) { details::g_MaxLevel = lev; }

template<typename... Args>
void GenericLog(LogLevel lev,
                details::with_source_location<std::format_string<Args...>> fmt,
                Args&&... args) {
    auto const& loc = fmt.location();
    auto msg = std::vformat(fmt.format().get(), std::make_format_args(args...));
    details::OutputLog(lev, std::move(msg), loc);
}

#define _FUNCTION(name)                                                        \
    template<typename... Args>                                                 \
    void Log##name(                                                            \
            details::with_source_location<std::format_string<Args...>> fmt,    \
            Args&&... args) {                                                  \
        return GenericLog(LogLevel::name, std::move(fmt),                      \
                          std::forward<Args>(args)...);                        \
    }
LOGGER_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION

#define LOG_P(x) ::Logger::LogDebug(#x "={}", x)

} // namespace Logger

IGAME_NAMESPACE_END