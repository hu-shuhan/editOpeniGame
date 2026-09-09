//
// Created by m_ky on 2025/2/22.
//

/**
 * @class   iGameLogger
 * @brief   iGameLogger's brief
 */

#include "iGameLogger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#ifndef __EMSCRIPTEN__
#include <spdlog/sinks/basic_file_sink.h>
#endif

namespace iGame // iGame namespace begin
{
std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
std::shared_ptr<spdlog::logger> Log::s_RenderingLogger;
std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

void Log::Init() {
    //        spdlog::set_pattern("%^[%T] %n: %v%$");
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S][%n][%l$]: %v%$");
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("%^[%T][%n][%l]: %v%$");
#ifdef __EMSCRIPTEN__
    spdlog::sinks_init_list core_sinksInitList{console_sink};
#else
    auto core_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/iGame-core-log.txt");
    spdlog::sinks_init_list core_sinksInitList{console_sink, core_file_sink};
#endif
    s_CoreLogger = std::make_shared<spdlog::logger>("iGameVis_Core", core_sinksInitList);
    s_CoreLogger->set_level(spdlog::level::trace);

#ifdef __EMSCRIPTEN__
    spdlog::sinks_init_list rendering_sinksInitList{console_sink};
#else
    auto rendering_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/iGame-Rendering-log.txt");
    spdlog::sinks_init_list rendering_sinksInitList{console_sink, rendering_file_sink};
#endif
    s_RenderingLogger = std::make_shared<spdlog::logger>("iGameVis_Rendering", rendering_sinksInitList);
    s_RenderingLogger->set_level(spdlog::level::trace);

#ifdef __EMSCRIPTEN__
    spdlog::sinks_init_list client_sinksInitList{console_sink};
#else
    auto client_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/iGame-client-log.txt");
    spdlog::sinks_init_list client_sinksInitList{console_sink, client_file_sink};
#endif
    s_ClientLogger = std::make_shared<spdlog::logger>("iGameVis_Client", client_sinksInitList);
    s_ClientLogger->set_level(spdlog::level::trace);
}

std::shared_ptr<spdlog::logger>& Log::GetCoreLogger() {
    if (!s_CoreLogger) Log::Init();
    return s_CoreLogger;
}

std::shared_ptr<spdlog::logger>& Log::GetClientLogger() {
    if (!s_ClientLogger) Log::Init();
    return s_ClientLogger;
}
std::shared_ptr<spdlog::logger>& Log::GetRenderingLogger() {
    if (!s_RenderingLogger) Log::Init();
    return s_RenderingLogger;
}
} //iGame namespace end
