#ifndef iGameDataCodeciGameDataCodecOutputSinks_h
#define iGameDataCodeciGameDataCodecOutputSinks_h

#include "DataCodec/API/Output/DataCodecOutputSinks.h"
#include "iGameProgressObserver.h"

#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <utility>

IGAME_NAMESPACE_BEGIN

struct iGameDataCodecProgressBarOutput {
    bool updateProgressObserver{true};
    std::function<void(const ::datacodec::DataCodecProgressUpdate&)> callback;
};

class iGameDataCodecProgressBarSink final
    : public ::datacodec::IDataCodecProgressSink {
public:
    iGameDataCodecProgressBarSink() = default;

    explicit iGameDataCodecProgressBarSink(iGameDataCodecProgressBarOutput output)
        : m_output(std::move(output)) {}

    void SubmitProgress(
        const ::datacodec::DataCodecProgressUpdate& progress) override;

private:
    [[nodiscard]] static std::mutex& GlobalSubmitMutex();

    iGameDataCodecProgressBarOutput m_output;
    std::mutex m_mutex;
    double m_lastNormalized{0.0};
};

class iGameSpdlogDataCodecConsoleSink final
    : public ::datacodec::IDataCodecConsoleSink {
public:
    void SubmitConsoleStatus(
        const ::datacodec::DataCodecStatusRecord& status) override;
};

class iGameDataCodecReportFileSink final
    : public ::datacodec::IDataCodecReportFileSink {
public:
    explicit iGameDataCodecReportFileSink(std::filesystem::path directory)
        : m_directory(std::move(directory)) {}

    [[nodiscard]] ::datacodec::DataCodecReportWriteResult WriteReportFile(
        const ::datacodec::DataCodecReportFile& report) override;

private:
    std::filesystem::path m_directory;
    std::mutex m_mutex;
    std::size_t m_nextIndex{0u};
};

IGAME_NAMESPACE_END

#endif
