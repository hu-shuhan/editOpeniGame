#ifndef DATACODEC_API_ENTRY_DATACODECDECODEENTRY_H
#define DATACODEC_API_ENTRY_DATACODECDECODEENTRY_H

#include "DataCodec/API/Adapter/IDecodeAdapter.h"
#include "DataCodec/API/Adapter/IFramePackageDecodeAssembly.h"
#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Runtime/Execution/DataCodecExecutionResources.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Storage/FramePackage/FramePackageFormat.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <stop_token>
#include <string>
#include <vector>

namespace datacodec {

class DecodeSession;

struct DecodePackageRequest {
    std::shared_ptr<IByteRangeReader> inputReader;
    const FramePackage* framePackageMetadata{nullptr};
    IDecodeAdapter* leafAdapter{nullptr};
    IFramePackageDecodeAssembly* frameAssembly{nullptr};
    std::optional<std::uint32_t> requestedFrameIndex;
    AttributeSelectionMode attributeSelection{AttributeSelectionMode::AllAvailable};
    std::vector<AttributeTarget> attributeTargets;
    std::string topologyReferenceKey;
    std::uint32_t topologyOwnerFrameIndex{0u};
    DataCodecDecodePackageConfigurationParams configuration{
        MakeDefaultDecodePackageConfigurationParams()};
    std::shared_ptr<IRunRecordSink> runRecordSink;
    DecodeSession* session{nullptr};
    DataCodecExecutionResources executionResources;
    std::stop_token stopToken;
};

struct DecodePackageResult {
    bool success{false};
    bool cancelled{false};
    bool decodedFramePackage{false};
    std::uint64_t inputBytes{0u};
    std::vector<TelemetryMessageRecord> messages;
};

[[nodiscard]] DecodePackageResult DecodePackage(const DecodePackageRequest& request);

} // namespace datacodec

#endif
