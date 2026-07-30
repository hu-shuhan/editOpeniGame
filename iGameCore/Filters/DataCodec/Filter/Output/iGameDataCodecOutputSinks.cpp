#include "DataCodec/Filter/Output/iGameDataCodecOutputSinks.h"

#include "DataCodec/Common/DataCodecCallback.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <memory>
#include <string_view>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

IGAME_NAMESPACE_BEGIN

namespace {

spdlog::logger& DataCodecConsoleLogger() {
    static const auto logger = [] {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_pattern("%^[%T] %v%$");
        auto result = std::make_shared<spdlog::logger>(
            "DataCodec",
            spdlog::sinks_init_list{consoleSink});
        result->set_level(spdlog::level::info);
        return result;
    }();
    return *logger;
}

std::string EncodeForActiveConsole(const std::string_view utf8Text) {
#ifdef _WIN32
    const auto outputHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD consoleMode = 0u;
    if (outputHandle == nullptr || outputHandle == INVALID_HANDLE_VALUE ||
        !::GetConsoleMode(outputHandle, &consoleMode)) {
        return std::string(utf8Text);
    }
    const auto consoleCodePage = ::GetConsoleOutputCP();
    if (consoleCodePage == 0u || consoleCodePage == CP_UTF8) {
        return std::string(utf8Text);
    }
    if (utf8Text.size() > static_cast<std::size_t>(
            std::numeric_limits<int>::max())) {
        return std::string(utf8Text);
    }
    const auto sourceSize = static_cast<int>(utf8Text.size());
    const int wideSize = ::MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        utf8Text.data(),
        sourceSize,
        nullptr,
        0);
    if (wideSize <= 0) {
        return std::string(utf8Text);
    }
    std::wstring wideText(static_cast<std::size_t>(wideSize), L'\0');
    if (::MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            utf8Text.data(),
            sourceSize,
            wideText.data(),
            wideSize) != wideSize) {
        return std::string(utf8Text);
    }
    const int encodedSize = ::WideCharToMultiByte(
        consoleCodePage,
        0u,
        wideText.data(),
        wideSize,
        nullptr,
        0,
        nullptr,
        nullptr);
    if (encodedSize <= 0) {
        return std::string(utf8Text);
    }
    std::string encodedText(static_cast<std::size_t>(encodedSize), '\0');
    if (::WideCharToMultiByte(
            consoleCodePage,
            0u,
            wideText.data(),
            wideSize,
            encodedText.data(),
            encodedSize,
            nullptr,
            nullptr) != encodedSize) {
        return std::string(utf8Text);
    }
    return encodedText;
#else
    return std::string(utf8Text);
#endif
}

std::string SanitizeFileComponent(std::string value) {
    if (value.empty()) {
        value = "report";
    }
    for (auto& character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (!(byte >= static_cast<unsigned char>('a') && byte <= static_cast<unsigned char>('z')) &&
            !(byte >= static_cast<unsigned char>('A') && byte <= static_cast<unsigned char>('Z')) &&
            !(byte >= static_cast<unsigned char>('0') && byte <= static_cast<unsigned char>('9')) &&
            character != '_' && character != '-' && character != '.') {
            character = '_';
        }
    }
    return value;
}

std::string NormalizeExtension(std::string extension) {
    extension = SanitizeFileComponent(std::move(extension));
    if (extension.empty() || extension == "report") {
        return ".txt";
    }
    if (extension.front() != '.') {
        extension.insert(extension.begin(), '.');
    }
    return extension;
}

std::string PathToUtf8(const std::filesystem::path& path) {
    const auto encoded = path.u8string();
    return std::string(
        reinterpret_cast<const char*>(encoded.data()),
        encoded.size());
}

} // namespace

std::mutex& iGameDataCodecProgressBarSink::GlobalSubmitMutex() {
    static std::mutex mutex;
    return mutex;
}

void iGameDataCodecProgressBarSink::SubmitProgress(
    const ::datacodec::DataCodecProgressUpdate& progress) {
    std::scoped_lock lock(GlobalSubmitMutex(), m_mutex);
    if (m_output.callback) {
        m_output.callback(progress);
    }
    if (!m_output.updateProgressObserver) {
        return;
    }

    auto* observer = ProgressObserver::Instance();
    if (observer == nullptr) {
        return;
    }

    double normalized = ::datacodec::callback::NormalizeProgress(progress.normalized);
    if (progress.phase == ::datacodec::DataCodecProgressPhase::Begin) {
        m_lastNormalized = 0.0;
        normalized = 0.0;
    } else if (progress.phase == ::datacodec::DataCodecProgressPhase::Finish) {
        m_lastNormalized = progress.success ? 1.0 : 0.0;
        normalized = m_lastNormalized;
    } else if (normalized < m_lastNormalized) {
        return;
    } else {
        normalized = std::min(normalized, std::nextafter(1.0, 0.0));
        m_lastNormalized = std::max(m_lastNormalized, normalized);
        normalized = m_lastNormalized;
    }

    observer->UpdateText(::datacodec::FormatDataCodecProgressText(progress));
    observer->UpdateProgress(normalized);
}

void iGameSpdlogDataCodecConsoleSink::SubmitConsoleStatus(
    const ::datacodec::DataCodecStatusRecord& status) {
    const auto text = ::datacodec::FormatDataCodecStatusText(status);
    if (text.empty()) {
        return;
    }
    auto& logger = DataCodecConsoleLogger();
    const auto consoleText = EncodeForActiveConsole(text);
    switch (status.severity) {
        case ::datacodec::DataCodecStatusSeverity::Warning:
            logger.warn("{}", consoleText);
            break;
        case ::datacodec::DataCodecStatusSeverity::Error:
            logger.error("{}", consoleText);
            break;
        case ::datacodec::DataCodecStatusSeverity::Critical:
            logger.critical("{}", consoleText);
            break;
        case ::datacodec::DataCodecStatusSeverity::Info:
        default:
            logger.info("{}", consoleText);
            break;
    }
}

::datacodec::DataCodecReportWriteResult
iGameDataCodecReportFileSink::WriteReportFile(
    const ::datacodec::DataCodecReportFile& report) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::error_code errorCode;
    std::filesystem::create_directories(m_directory, errorCode);
    if (errorCode) {
        return {
            .success = false,
            .error = "failed to create report directory: " + errorCode.message(),
        };
    }

    const auto stem = SanitizeFileComponent(report.name);
    const auto extension = NormalizeExtension(report.preferredExtension);
    auto path = m_directory / (stem + extension);
    while (std::filesystem::exists(path, errorCode) && !errorCode) {
        path = m_directory /
            (stem + "_" + std::to_string(++m_nextIndex) + extension);
    }
    if (errorCode) {
        return {
            .success = false,
            .error = "failed to inspect report file path: " + errorCode.message(),
        };
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return {
            .success = false,
            .error = "failed to open report file",
        };
    }
    output.write(
        report.content.data(),
        static_cast<std::streamsize>(report.content.size()));
    output.flush();
    if (!output) {
        return {
            .success = false,
            .error = "failed to write report file",
        };
    }
    return {
        .success = true,
        .path = PathToUtf8(path),
    };
}

IGAME_NAMESPACE_END
