#ifndef DATACODEC_API_ADAPTER_IRUNRECORDSINK_H
#define DATACODEC_API_ADAPTER_IRUNRECORDSINK_H

#include "DataCodec/API/Adapter/RunRecord.h"

namespace datacodec {

class IRunRecordSink {
public:
    virtual ~IRunRecordSink() = default;

    [[nodiscard]] virtual RunRecordMask Interests() const noexcept = 0;
    [[nodiscard]] virtual RunCollectionMask CollectionRequests() const noexcept {
        return 0u;
    }
    virtual void Submit(const RunRecord& record) = 0;

    [[nodiscard]] bool Wants(const RunRecordKind kind) const noexcept {
        return (Interests() & RunRecordBit(kind)) != 0u;
    }

    [[nodiscard]] bool Requests(const RunCollectionKind kind) const noexcept {
        return (CollectionRequests() & RunCollectionBit(kind)) != 0u;
    }
};

} // 命名空间 datacodec

#endif
