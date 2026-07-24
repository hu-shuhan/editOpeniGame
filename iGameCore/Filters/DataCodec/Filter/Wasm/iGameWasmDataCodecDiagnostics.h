#ifndef iGameDataCodeciGameWasmDataCodecDiagnostics_h
#define iGameDataCodeciGameWasmDataCodecDiagnostics_h

#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "iGameMacro.h"

#include <mutex>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class iGameWasmDataCodecDiagnosticsSink final : public ::datacodec::IRunRecordSink {
public:
    [[nodiscard]] ::datacodec::RunRecordMask Interests() const noexcept override;
    void Submit(const ::datacodec::RunRecord& record) override;

    [[nodiscard]] std::string BuildTopologyTimingDetail() const;

private:
    mutable std::mutex m_mutex;
    std::vector<::datacodec::TelemetryStageRecord> m_stages;
    double m_rootElapsedMs{0.0};
};

IGAME_NAMESPACE_END

#endif
