#ifndef DATACODEC_API_ENTRY_DATACODECENCODEENTRY_H
#define DATACODEC_API_ENTRY_DATACODECENCODEENTRY_H

#include "DataCodec/API/Adapter/IBlockTreeAdapter.h"
#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/Runtime/Execution/DataCodecExecutionResources.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace datacodec {

enum class EncodePackageKind : std::uint8_t {
    Auto,
    LeafPackage,
    FramePackage,
};

struct EncodeInput {
    using Adapter = std::variant<std::monostate, IEncodeAdapter*, IBlockTreeAdapter*>;

    Adapter adapter;
    std::string objectName;
    std::string meshType;
    BlockPath leafPath;
    std::string rootName;
    std::uint32_t frameIndex{0u};
    std::uint32_t frameCount{1u};
    float timeValue{0.0f};

    [[nodiscard]] static EncodeInput LeafAdapter(
        IEncodeAdapter* inputAdapter,
        BlockPath path = {},
        std::string name = {},
        std::string type = {},
        std::uint32_t inputFrameIndex = 0u);

    [[nodiscard]] static EncodeInput BlockTreeAdapter(
        IBlockTreeAdapter* inputAdapter,
        std::string name = {},
        std::uint32_t inputFrameIndex = 0u,
        std::uint32_t inputFrameCount = 1u,
        float inputTimeValue = 0.0f);
};

struct EncodeOutput {
    using Target = std::variant<std::monostate, IByteRangeOutput*>;

    EncodePackageKind packageKind{EncodePackageKind::Auto};
    Target target;

    [[nodiscard]] static EncodeOutput ByteRange(
        EncodePackageKind kind,
        IByteRangeOutput* sink);
};

struct EncodeRequest {
    EncodeInput input;
    EncodeOutput output;
    std::vector<AttributeTarget> attributeTargets;
    EncodeCodecControlParams controlParams{MakeDefaultEncodeControlParams()};
    EncodePipelineControlParams pipelineControl;
    EncodeExecutionOptions execution{MakeDefaultEncodeExecutionOptions()};
    DataCodecEncodeConfigurationSource configurationSource;
    std::shared_ptr<IRunRecordSink> runRecordSink;
    DataCodecExecutionResources executionResources;
};

struct EncodeResult {
    bool success{false};
    bool hasEncodedOutput{false};
    std::vector<std::uint8_t> encodedBytes;
    std::uint64_t encodedByteCount{0u};
    std::size_t leafCount{0u};
    EncodePackageKind packageKind{EncodePackageKind::Auto};
    std::vector<TelemetryMessageRecord> messages;
};

[[nodiscard]] EncodeResult Encode(const EncodeRequest& request);

} // namespace datacodec

#endif
