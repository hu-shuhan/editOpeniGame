#ifndef iGameDataCodeciGameDataCodecOutputBinding_h
#define iGameDataCodeciGameDataCodecOutputBinding_h

#include "DataCodec/Filter/Output/iGameDataCodecOutputSinks.h"
#include "DataCodec/Runtime/Output/DataCodecOutputRouter.h"
#include "DataCodec/Runtime/Record/RunRecordDispatcher.h"

#include <memory>
#include <utility>

IGAME_NAMESPACE_BEGIN

inline ::datacodec::DataCodecOutputSinks ResolveiGameDataCodecOutputSinks(
    ::datacodec::DataCodecOutputSinks sinks,
    const bool includeProgressBar,
    const bool includeConsole = true) {
    if (includeConsole && sinks.console == nullptr) {
        sinks.console = std::make_shared<iGameSpdlogDataCodecConsoleSink>();
    }
    if (includeProgressBar && sinks.progress == nullptr) {
        sinks.progress = std::make_shared<iGameDataCodecProgressBarSink>();
    }
    return sinks;
}

class iGameDataCodecOutputBinding final {
public:
    iGameDataCodecOutputBinding(
        ::datacodec::DataCodecOutputSinks outputSinks,
        std::shared_ptr<::datacodec::IRunRecordSink> telemetrySink = {},
        const bool includeProgressBar = false,
        const bool includeConsole = true)
        : m_outputSinks(ResolveiGameDataCodecOutputSinks(
              std::move(outputSinks),
              includeProgressBar,
              includeConsole)),
          m_dispatcher(std::make_shared<::datacodec::RunRecordDispatcher>()) {
        m_dispatcher->AddSink(std::move(telemetrySink));
        if (!m_outputSinks.Empty()) {
            m_router = std::make_shared<::datacodec::DataCodecOutputRouter>(
                m_outputSinks);
            m_dispatcher->AddSink(m_router);
        }
    }

    [[nodiscard]] const ::datacodec::DataCodecOutputSinks& OutputSinks() const noexcept {
        return m_outputSinks;
    }

    [[nodiscard]] std::shared_ptr<::datacodec::IRunRecordSink> RecordSink() const {
        return m_dispatcher;
    }

private:
    ::datacodec::DataCodecOutputSinks m_outputSinks;
    std::shared_ptr<::datacodec::RunRecordDispatcher> m_dispatcher;
    std::shared_ptr<::datacodec::DataCodecOutputRouter> m_router;
};

[[nodiscard]] inline std::shared_ptr<::datacodec::IRunRecordSink>
MakeiGameDataCodecOutputRecordSink(
    ::datacodec::DataCodecOutputSinks outputSinks = {},
    std::shared_ptr<::datacodec::IRunRecordSink> telemetrySink = {},
    const bool includeProgressBar = false,
    const bool includeConsole = true) {
    return iGameDataCodecOutputBinding(
        std::move(outputSinks),
        std::move(telemetrySink),
        includeProgressBar,
        includeConsole).RecordSink();
}

IGAME_NAMESPACE_END

#endif
