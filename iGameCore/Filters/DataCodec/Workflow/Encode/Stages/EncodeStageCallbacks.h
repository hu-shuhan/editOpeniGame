#ifndef DATACODEC_WORKFLOW_ENCODE_STAGES_ENCODESTAGECALLBACKS_H
#define DATACODEC_WORKFLOW_ENCODE_STAGES_ENCODESTAGECALLBACKS_H

#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Runtime/Context/EncodeContext.h"

#include <string>
#include <string_view>
namespace datacodec {

inline callback::ResourceCallback MakeEncodeResourceCallback(EncodeContext& context) {
    if (!context.runRecords.Wants(RunRecordKind::ResourceUsage)) {
        return {};
    }
    return [&context](const std::string_view name, const std::uint64_t logicalBytes) {
        context.runRecords.RecordResourceUsage(std::string(name), logicalBytes);
    };
}

} // namespace datacodec

#endif
