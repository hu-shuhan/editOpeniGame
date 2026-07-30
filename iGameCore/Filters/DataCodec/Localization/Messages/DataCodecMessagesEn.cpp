#include "DataCodec/Localization/DataCodecMessageCatalog.h"

namespace datacodec::localizationdetail {

std::string_view DataCodecEnglishMessageTemplate(const DataCodecMessageId id) noexcept {
    switch (id) {
        case DataCodecMessageId::EncodePreparing: return "Preparing compression";
        case DataCodecMessageId::EncodeSorting: return "Sorting";
        case DataCodecMessageId::EncodeTopology: return "Compressing topology";
        case DataCodecMessageId::EncodeGeometry: return "Compressing coordinates";
        case DataCodecMessageId::EncodeAttribute: return "Compressing attribute data";
        case DataCodecMessageId::EncodeAttributeNamed: return "Compressing attribute data: {name}";
        case DataCodecMessageId::EncodeAttributeUnnamed: return "Compressing unnamed attribute {index}";
        case DataCodecMessageId::EncodeSingleBlock: return "Compressing data block";
        case DataCodecMessageId::EncodeBlock: return "Compressing data block {index}/{count}";
        case DataCodecMessageId::EncodePackageCompress: return "Compressing package";
        case DataCodecMessageId::EncodePackageWrite: return "Writing package";
        case DataCodecMessageId::EncodeWriteFile: return "Writing file";
        case DataCodecMessageId::EncodeFinalizeResult: return "Finalizing encoded result";
        case DataCodecMessageId::EncodeFinalizeFile: return "Finalizing file";
        case DataCodecMessageId::EncodeCompleted: return "Compression completed";
        case DataCodecMessageId::EncodeWarning: return "Compression warning";
        case DataCodecMessageId::EncodeFailed: return "Compression failed";
        case DataCodecMessageId::DecodeStarted: return "Starting decompression";
        case DataCodecMessageId::DecodeParams: return "Decoding parameters";
        case DataCodecMessageId::DecodeGeometry: return "Decoding coordinates";
        case DataCodecMessageId::DecodeTopology: return "Decoding topology";
        case DataCodecMessageId::DecodeAttribute: return "Decoding attribute data";
        case DataCodecMessageId::DecodeCommit: return "Committing result";
        case DataCodecMessageId::DecodeInProgress: return "Decompressing";
        case DataCodecMessageId::DecodeCompleted: return "Decompression completed";
        case DataCodecMessageId::DecodeWarning: return "Decompression warning";
        case DataCodecMessageId::DecodeFailed: return "Decompression failed";
        case DataCodecMessageId::DecodeSingleBlock: return "Decompressing data block";
        case DataCodecMessageId::DecodeBlock: return "Decompressing data block {index}/{count}";
        case DataCodecMessageId::DecodeValidateCommit: return "Validating decoded result";
        case DataCodecMessageId::DecodeCommitGeometry: return "Committing coordinates";
        case DataCodecMessageId::DecodeCommitTopology: return "Committing topology";
        case DataCodecMessageId::DecodeCommitAttribute: return "Committing attribute data";
        case DataCodecMessageId::DecodeFinalizeResult: return "Finalizing decoded result";
        case DataCodecMessageId::AttributeRequestCompleted: return "Attribute request completed";
        case DataCodecMessageId::AttributeProcessingStarted: return "Processing attributes";
        case DataCodecMessageId::AttributeProcessingCompleted: return "Attribute processing completed";
        case DataCodecMessageId::PrepareReferenceFrame: return "Preparing reference frame {index}/{count} ({frame})";
        case DataCodecMessageId::DecodeTargetFrame: return "Decompressing target frame ({frame})";
        case DataCodecMessageId::CacheHit: return "Cache hit";
        case DataCodecMessageId::PackageDecodeStarted: return "DataCodec package decompression started";
        case DataCodecMessageId::PackageDecodeCompleted: return "DataCodec package decompression completed";
        case DataCodecMessageId::PackageDecodeFailed: return "DataCodec package decompression failed";
        case DataCodecMessageId::FrameCounter: return "Frame {index}/{count}";
        case DataCodecMessageId::None:
        default: return {};
    }
}

} // namespace datacodec::localizationdetail
