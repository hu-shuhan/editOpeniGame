//#pragma once
//
//#include <vector>
//
//#include "spdlog/sinks/basic_file_sink.h"
//#include "spdlog/sinks/stdout_color_sinks.h"
//#include "spdlog/spdlog.h"
//
//namespace gpbezier {
//
//class GPLog
//{
//   public:
//    inline static void init(spdlog::level::level_enum level = spdlog::level::err)
//    {
//        static bool bInit = false;
//
//        if(bInit) return;
//
//        std::vector<spdlog::sink_ptr> sinks;
//        sinks.emplace_back(
//            std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
//        sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
//            "logs/GPBezier.log", true));
//
//        sinks[0]->set_pattern("%^[%T] %n: %v%$");
//        sinks[1]->set_pattern("[%T] [%l] %n: %v");
//
//        m_logger = std::make_shared<spdlog::logger>(
//            "GPBezier", begin(sinks), end(sinks));
//        spdlog::register_logger(m_logger);
//        m_logger->set_level(level);
//        m_logger->flush_on(level);
//
//        bInit = true;
//    }
//
//    inline static std::shared_ptr<spdlog::logger>& get_logger()
//    {
//        return m_logger;
//    }
//
//
//   private:
//    inline static std::shared_ptr<spdlog::logger> m_logger;
//};
//}  // namespace gpmesh
//
//#define GPBEZIER_TRACE(...) ::gpbezier::GPLog::get_logger()->trace(__VA_ARGS__)
//#define GPBEZIER_INFO(...) ::gpbezier::GPLog::get_logger()->info(__VA_ARGS__)
//#define GPBEZIER_WARN(...)                                                      \
//    ::gpbezier::GPLog::get_logger()->warn("Line {} File {}", __LINE__, __FILE__); \
//    ::gpbezier::GPLog::get_logger()->warn(__VA_ARGS__)
//#define GPBEZIER_ERROR(...)                                                      \
//    ::gpbezier::GPLog::get_logger()->error("Line {} File {}", __LINE__, __FILE__); \
//    ::gpbezier::GPLog::get_logger()->error(__VA_ARGS__)
//#define GPBEZIER_CRITICAL(...)                    \
//    ::gpbezier::GPLog::get_logger()->critical(      \
//        "Line {} File {}", __LINE__, __FILE__); \
//    ::gpbezier::GPLog::get_logger()->critical(__VA_ARGS__)
