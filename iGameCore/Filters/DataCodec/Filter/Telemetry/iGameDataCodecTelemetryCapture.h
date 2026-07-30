#ifndef iGameDataCodeciGameDataCodecTelemetryCapture_h
#define iGameDataCodeciGameDataCodecTelemetryCapture_h

#include "DataCodec/Log/Capture/RemapOrderCapture.h"
#include "DataCodec/Log/Telemetry/Sinks/TelemetrySessionSink.h"
#include "DataCodec/Runtime/Record/RunRecordDispatcher.h"

#include <memory>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

class iGameDataCodecTelemetryCapture final {
public:
    explicit iGameDataCodecTelemetryCapture(
        std::shared_ptr<::datacodec::IRunRecordSink> downstream = {})
        : m_dispatcher(std::make_shared<::datacodec::RunRecordDispatcher>()) {
        m_dispatcher->AddSink(std::move(downstream));
    }

    void CaptureSessions(
        const ::datacodec::RunRecordMask interests,
        const ::datacodec::RunCollectionMask collectionRequests = 0u) {
        if (m_sessionCapture != nullptr) {
            return;
        }
        m_sessionCapture = std::make_shared<::datacodec::TelemetrySessionSink>(
            interests,
            collectionRequests);
        m_dispatcher->AddSink(m_sessionCapture);
    }

    void CaptureRemapOrders() {
        if (m_remapCapture != nullptr) {
            return;
        }
        m_remapCapture = std::make_shared<::datacodec::log::RemapOrderCapture>();
        m_dispatcher->AddSink(m_remapCapture);
    }

    [[nodiscard]] std::shared_ptr<::datacodec::IRunRecordSink> Sink() const {
        return m_dispatcher;
    }

    [[nodiscard]] std::vector<::datacodec::TelemetryMessageRecord> SnapshotMessages() const {
        return m_sessionCapture != nullptr
            ? m_sessionCapture->SnapshotMessages()
            : std::vector<::datacodec::TelemetryMessageRecord>{};
    }

    [[nodiscard]] std::vector<::datacodec::TelemetrySession>
    SnapshotCompletedTelemetrySessions() const {
        return m_sessionCapture != nullptr
            ? m_sessionCapture->SnapshotCompletedSessions()
            : std::vector<::datacodec::TelemetrySession>{};
    }

    [[nodiscard]] ::datacodec::log::RemapOrderSnapshot TakeRemapOrders() const {
        return m_remapCapture != nullptr
            ? m_remapCapture->TakeSnapshot()
            : ::datacodec::log::RemapOrderSnapshot{};
    }

private:
    std::shared_ptr<::datacodec::RunRecordDispatcher> m_dispatcher;
    std::shared_ptr<::datacodec::TelemetrySessionSink> m_sessionCapture;
    std::shared_ptr<::datacodec::log::RemapOrderCapture> m_remapCapture;
};

IGAME_NAMESPACE_END

#endif
