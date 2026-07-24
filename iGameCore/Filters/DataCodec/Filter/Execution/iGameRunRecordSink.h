#ifndef iGameDataCodeciGameRunRecordSink_h
#define iGameDataCodeciGameRunRecordSink_h

#include "DataCodec/Filter/Execution/iGameRunRecordProgressSink.h"
#include "DataCodec/Log/Capture/RemapOrderCapture.h"
#include "DataCodec/Log/Telemetry/Sinks/TelemetrySessionSink.h"
#include "DataCodec/Runtime/Record/RunMessageCaptureSink.h"
#include "DataCodec/Runtime/Record/RunRecordDispatcher.h"
#include "DataCodec/Runtime/Record/RunRecordSubmit.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

class iGameRunRecordCallbackSink final : public ::datacodec::IRunRecordSink {
public:
    using Callback = std::function<void(const ::datacodec::RunRecord&)>;

    iGameRunRecordCallbackSink(
        const ::datacodec::RunRecordMask interests,
        Callback callback,
        const ::datacodec::RunCollectionMask collectionRequests = 0u)
        : m_interests(interests),
          m_collectionRequests(collectionRequests),
          m_callback(std::move(callback)) {}

    [[nodiscard]] ::datacodec::RunRecordMask Interests() const noexcept override {
        return m_interests;
    }

    [[nodiscard]] ::datacodec::RunCollectionMask CollectionRequests() const noexcept override {
        return m_collectionRequests;
    }

    void Submit(const ::datacodec::RunRecord& record) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_callback) {
            m_callback(record);
        }
    }

private:
    ::datacodec::RunRecordMask m_interests{0u};
    ::datacodec::RunCollectionMask m_collectionRequests{0u};
    Callback m_callback;
    std::mutex m_mutex;
};

class iGameRunRecordSinkSet {
public:
    explicit iGameRunRecordSinkSet(
        const bool includeProgressSink = false,
        std::shared_ptr<::datacodec::IRunRecordSink> downstream = {})
        : m_dispatcher(std::make_shared<::datacodec::RunRecordDispatcher>()) {
        if (includeProgressSink) {
            m_dispatcher->AddSink(std::make_shared<iGameRunRecordProgressSink>());
        }
        m_dispatcher->AddSink(std::move(downstream));
    }

    void AddSink(std::shared_ptr<::datacodec::IRunRecordSink> sink) {
        m_dispatcher->AddSink(std::move(sink));
    }

    void AddCallback(
        const ::datacodec::RunRecordMask interests,
        iGameRunRecordCallbackSink::Callback callback,
        const ::datacodec::RunCollectionMask collectionRequests = 0u) {
        AddSink(std::make_shared<iGameRunRecordCallbackSink>(
            interests,
            std::move(callback),
            collectionRequests));
    }

    void CaptureMessages() {
        if (m_messageCapture != nullptr) {
            return;
        }
        m_messageCapture = std::make_shared<::datacodec::RunMessageCaptureSink>();
        AddSink(m_messageCapture);
    }

    void CaptureTelemetry(
        const ::datacodec::RunRecordMask interests,
        const ::datacodec::RunCollectionMask collectionRequests = 0u) {
        if (m_telemetryCapture != nullptr) {
            return;
        }
        m_telemetryCapture = std::make_shared<::datacodec::TelemetrySessionSink>(
            interests,
            collectionRequests);
        AddSink(m_telemetryCapture);
    }

    void CaptureRemapOrders() {
        if (m_remapCapture != nullptr) {
            return;
        }
        m_remapCapture = std::make_shared<::datacodec::log::RemapOrderCapture>();
        AddSink(m_remapCapture);
    }

    [[nodiscard]] std::shared_ptr<::datacodec::IRunRecordSink> Sink() const {
        return m_dispatcher;
    }

    [[nodiscard]] std::vector<::datacodec::TelemetryMessageRecord> TakeMessages() const {
        return m_messageCapture != nullptr
            ? m_messageCapture->TakeMessages()
            : std::vector<::datacodec::TelemetryMessageRecord>{};
    }

    [[nodiscard]] std::vector<::datacodec::TelemetrySession>
    SnapshotCompletedTelemetrySessions() const {
        return m_telemetryCapture != nullptr
            ? m_telemetryCapture->SnapshotCompletedSessions()
            : std::vector<::datacodec::TelemetrySession>{};
    }

    [[nodiscard]] ::datacodec::log::RemapOrderSnapshot TakeRemapOrders() const {
        return m_remapCapture != nullptr
            ? m_remapCapture->TakeSnapshot()
            : ::datacodec::log::RemapOrderSnapshot{};
    }

private:
    std::shared_ptr<::datacodec::RunRecordDispatcher> m_dispatcher;
    std::shared_ptr<::datacodec::RunMessageCaptureSink> m_messageCapture;
    std::shared_ptr<::datacodec::TelemetrySessionSink> m_telemetryCapture;
    std::shared_ptr<::datacodec::log::RemapOrderCapture> m_remapCapture;
};

[[nodiscard]] inline std::shared_ptr<::datacodec::IRunRecordSink>
MakeiGameRunRecordSink(
    std::shared_ptr<::datacodec::IRunRecordSink> downstream = {},
    const bool includeProgressSink = false) {
    return iGameRunRecordSinkSet(includeProgressSink, std::move(downstream)).Sink();
}

[[nodiscard]] inline ::datacodec::TelemetryMessageRecord MakeiGameRunMessage(
    const ::datacodec::TelemetryMessageSeverity severity,
    std::string origin,
    std::string text,
    std::string code = {}) {
    return ::datacodec::TelemetryMessageRecord{
        .severity = severity,
        .origin = std::move(origin),
        .code = std::move(code),
        .text = std::move(text),
    };
}

inline void SubmitiGameRunMessage(
    ::datacodec::IRunRecordSink* sink,
    ::datacodec::TelemetryMessageRecord message,
    const std::uint64_t runId = 0u) {
    ::datacodec::SubmitRunMessage(sink, std::move(message), runId);
}

inline void SubmitiGameRunError(
    ::datacodec::IRunRecordSink* sink,
    std::string origin,
    std::string text,
    std::string code = {},
    const std::uint64_t runId = 0u) {
    SubmitiGameRunMessage(
        sink,
        MakeiGameRunMessage(
            ::datacodec::TelemetryMessageSeverity::Error,
            std::move(origin),
            std::move(text),
            std::move(code)),
        runId);
}

IGAME_NAMESPACE_END

#endif
