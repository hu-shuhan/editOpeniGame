/**
 * @class   iGameLogger
 * @brief   iGameLogger's brief
 */

#pragma once

//#include "iGameMacro.h"

#include <memory>
#include <spdlog/spdlog.h>

namespace iGame // iGame namespace begin
{
class Log {
public:
    static void Init();
    //    inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() {return s_CoreLogger;}
    //    inline static std::shared_ptr<spdlog::logger>& GetRenderingLogger() {return s_RenderingLogger;}
    //    inline static std::shared_ptr<spdlog::logger>& GetClientLogger() {return s_ClientLogger;}

    static std::shared_ptr<spdlog::logger>& GetCoreLogger();
    static std::shared_ptr<spdlog::logger>& GetRenderingLogger();
    static std::shared_ptr<spdlog::logger>& GetClientLogger();
    //    static std::shared_ptr<spdlog::logger>& GetCoreLogger();
    //    static std::shared_ptr<spdlog::logger>& GetClientLogger();
private:
    static std::shared_ptr<spdlog::logger> s_CoreLogger;
    static std::shared_ptr<spdlog::logger> s_RenderingLogger;
    static std::shared_ptr<spdlog::logger> s_ClientLogger;
};

} // iGame namespace end

// Core log macros
#define IGAME_CORE_ERROR(...) ::iGame::Log::GetCoreLogger()->error(__VA_ARGS__)
#define IGAME_CORE_WARN(...)  ::iGame::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define IGAME_CORE_INFO(...)  ::iGame::Log::GetCoreLogger()->info(__VA_ARGS__)
#define IGAME_CORE_DEBUG(...)  ::iGame::Log::GetCoreLogger()->debug(__VA_ARGS__)
#define IGAME_CORE_TRACE(...) ::iGame::Log::GetCoreLogger()->trace(__VA_ARGS__)
//#define IGAME_CORE_FATAL(...) ::iGame::Log::GetCoreLogger()->fatal(__VA_ARGS__)

// Rendering log macros
#define IGAME_RENDERING_ERROR(...) ::iGame::Log::GetRenderingLogger()->error(__VA_ARGS__)
#define IGAME_RENDERING_WARN(...)  ::iGame::Log::GetRenderingLogger()->warn(__VA_ARGS__)
#define IGAME_RENDERING_INFO(...)  ::iGame::Log::GetRenderingLogger()->info(__VA_ARGS__)
#define IGAME_RENDERING_DEBUG(...)  ::iGame::Log::GetRenderingLogger()->debug(__VA_ARGS__)
#define IGAME_RENDERING_TRACE(...) ::iGame::Log::GetRenderingLogger()->trace(__VA_ARGS__)
//#define IGAME_RENDERING_FATAL(...) ::iGame::Log::GetRenderingLogger()->fatal(__VA_ARGS__)

// Client log macros
#define IGAME_ERROR(...) ::iGame::Log::GetClientLogger()->error(__VA_ARGS__)
#define IGAME_WARN(...)  ::iGame::Log::GetClientLogger()->warn(__VA_ARGS__)
#define IGAME_INFO(...)  ::iGame::Log::GetClientLogger()->info(__VA_ARGS__)
#define IGAME_DEBUG(...)  ::iGame::Log::GetClientLogger()->debug(__VA_ARGS__)
#define IGAME_TRACE(...) ::iGame::Log::GetClientLogger()->trace(__VA_ARGS__)
//#define IGAME_FATAL(...) ::iGame::Log::GetClientLogger()->fatal(__VA_ARGS__)