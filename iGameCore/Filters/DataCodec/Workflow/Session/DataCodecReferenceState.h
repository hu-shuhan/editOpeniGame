#ifndef DATACODEC_WORKFLOW_SESSION_DATACODECREFERENCESTATE_H
#define DATACODEC_WORKFLOW_SESSION_DATACODECREFERENCESTATE_H

#include "DataCodec/Runtime/Cache/DecodeCache/DecodedTopologyCache.h"
#include "DataCodec/Workflow/Temporal/TemporalBuilder.h"

#include <memory>

namespace datacodec {

// 编解码 workspace 持有的 reference 状态，filter 不直接接触这些字段
struct DataCodecReferenceState {
    TemporalBuilder::TemporalHistoryState temporalHistory;
    std::shared_ptr<DecodedTopologyReferenceCacheStore> decodeTopologyReferenceStore;

    void ResetDecodeReferences() {
        decodeTopologyReferenceStore.reset();
    }
};

} // namespace datacodec

#endif
