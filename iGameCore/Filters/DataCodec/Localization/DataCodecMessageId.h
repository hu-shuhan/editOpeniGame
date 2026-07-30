#ifndef DATACODEC_LOCALIZATION_DATACODECMESSAGEID_H
#define DATACODEC_LOCALIZATION_DATACODECMESSAGEID_H

#include <cstdint>

namespace datacodec {

enum class DataCodecMessageId : std::uint16_t {
    None = 0u,
    EncodePreparing,
    EncodeSorting,
    EncodeTopology,
    EncodeGeometry,
    EncodeAttribute,
    EncodeAttributeNamed,
    EncodeAttributeUnnamed,
    EncodeSingleBlock,
    EncodeBlock,
    EncodePackageCompress,
    EncodePackageWrite,
    EncodeWriteFile,
    EncodeFinalizeResult,
    EncodeFinalizeFile,
    EncodeCompleted,
    EncodeWarning,
    EncodeFailed,
    DecodeStarted,
    DecodeParams,
    DecodeGeometry,
    DecodeTopology,
    DecodeAttribute,
    DecodeCommit,
    DecodeInProgress,
    DecodeCompleted,
    DecodeWarning,
    DecodeFailed,
    DecodeSingleBlock,
    DecodeBlock,
    DecodeValidateCommit,
    DecodeCommitGeometry,
    DecodeCommitTopology,
    DecodeCommitAttribute,
    DecodeFinalizeResult,
    AttributeRequestCompleted,
    AttributeProcessingStarted,
    AttributeProcessingCompleted,
    PrepareReferenceFrame,
    DecodeTargetFrame,
    CacheHit,
    PackageDecodeStarted,
    PackageDecodeCompleted,
    PackageDecodeFailed,
    FrameCounter,
};

[[nodiscard]] const char* DataCodecMessageIdName(DataCodecMessageId id) noexcept;

} // namespace datacodec

#endif
