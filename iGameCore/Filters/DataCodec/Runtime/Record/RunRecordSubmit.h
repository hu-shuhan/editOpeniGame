#ifndef DATACODEC_RUNTIME_RECORD_RUNRECORDSUBMIT_H
#define DATACODEC_RUNTIME_RECORD_RUNRECORDSUBMIT_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"

#include <cstdint>
#include <utility>

namespace datacodec {

inline void SubmitRunMessage(
    IRunRecordSink* sink,
    TelemetryMessageRecord message,
    const std::uint64_t runId = 0u) {
    if (sink == nullptr || !sink->Wants(RunRecordKind::Message)) {
        return;
    }
    sink->Submit(RunRecord{RunMessageRecord{
        .runId = runId,
        .message = std::move(message),
    }});
}

} // 命名空间 datacodec

#endif
