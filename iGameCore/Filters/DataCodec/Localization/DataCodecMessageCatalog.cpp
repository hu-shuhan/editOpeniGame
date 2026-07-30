#include "DataCodec/Localization/DataCodecMessageCatalog.h"

#include <utility>

namespace datacodec {

const char* DataCodecMessageIdName(const DataCodecMessageId id) noexcept {
    switch (id) {
        case DataCodecMessageId::EncodePreparing: return "EncodePreparing";
        case DataCodecMessageId::EncodeSorting: return "EncodeSorting";
        case DataCodecMessageId::EncodeTopology: return "EncodeTopology";
        case DataCodecMessageId::EncodeGeometry: return "EncodeGeometry";
        case DataCodecMessageId::EncodeAttribute: return "EncodeAttribute";
        case DataCodecMessageId::EncodeAttributeNamed: return "EncodeAttributeNamed";
        case DataCodecMessageId::EncodeAttributeUnnamed: return "EncodeAttributeUnnamed";
        case DataCodecMessageId::EncodeSingleBlock: return "EncodeSingleBlock";
        case DataCodecMessageId::EncodeBlock: return "EncodeBlock";
        case DataCodecMessageId::EncodePackageCompress: return "EncodePackageCompress";
        case DataCodecMessageId::EncodePackageWrite: return "EncodePackageWrite";
        case DataCodecMessageId::EncodeWriteFile: return "EncodeWriteFile";
        case DataCodecMessageId::EncodeFinalizeResult: return "EncodeFinalizeResult";
        case DataCodecMessageId::EncodeFinalizeFile: return "EncodeFinalizeFile";
        case DataCodecMessageId::EncodeCompleted: return "EncodeCompleted";
        case DataCodecMessageId::EncodeWarning: return "EncodeWarning";
        case DataCodecMessageId::EncodeFailed: return "EncodeFailed";
        case DataCodecMessageId::DecodeStarted: return "DecodeStarted";
        case DataCodecMessageId::DecodeParams: return "DecodeParams";
        case DataCodecMessageId::DecodeGeometry: return "DecodeGeometry";
        case DataCodecMessageId::DecodeTopology: return "DecodeTopology";
        case DataCodecMessageId::DecodeAttribute: return "DecodeAttribute";
        case DataCodecMessageId::DecodeCommit: return "DecodeCommit";
        case DataCodecMessageId::DecodeInProgress: return "DecodeInProgress";
        case DataCodecMessageId::DecodeCompleted: return "DecodeCompleted";
        case DataCodecMessageId::DecodeWarning: return "DecodeWarning";
        case DataCodecMessageId::DecodeFailed: return "DecodeFailed";
        case DataCodecMessageId::DecodeSingleBlock: return "DecodeSingleBlock";
        case DataCodecMessageId::DecodeBlock: return "DecodeBlock";
        case DataCodecMessageId::DecodeValidateCommit: return "DecodeValidateCommit";
        case DataCodecMessageId::DecodeCommitGeometry: return "DecodeCommitGeometry";
        case DataCodecMessageId::DecodeCommitTopology: return "DecodeCommitTopology";
        case DataCodecMessageId::DecodeCommitAttribute: return "DecodeCommitAttribute";
        case DataCodecMessageId::DecodeFinalizeResult: return "DecodeFinalizeResult";
        case DataCodecMessageId::AttributeRequestCompleted: return "AttributeRequestCompleted";
        case DataCodecMessageId::AttributeProcessingStarted: return "AttributeProcessingStarted";
        case DataCodecMessageId::AttributeProcessingCompleted: return "AttributeProcessingCompleted";
        case DataCodecMessageId::PrepareReferenceFrame: return "PrepareReferenceFrame";
        case DataCodecMessageId::DecodeTargetFrame: return "DecodeTargetFrame";
        case DataCodecMessageId::CacheHit: return "CacheHit";
        case DataCodecMessageId::PackageDecodeStarted: return "PackageDecodeStarted";
        case DataCodecMessageId::PackageDecodeCompleted: return "PackageDecodeCompleted";
        case DataCodecMessageId::PackageDecodeFailed: return "PackageDecodeFailed";
        case DataCodecMessageId::FrameCounter: return "FrameCounter";
        case DataCodecMessageId::None:
        default: return "None";
    }
}

std::string_view DataCodecMessageTemplate(
    const DataCodecLanguage language,
    const DataCodecMessageId id) noexcept {
    if (language == DataCodecLanguage::SimplifiedChinese) {
        const auto text = localizationdetail::DataCodecSimplifiedChineseMessageTemplate(id);
        if (!text.empty()) {
            return text;
        }
    }
    return localizationdetail::DataCodecEnglishMessageTemplate(id);
}

std::string FormatDataCodecMessage(
    const DataCodecLanguage language,
    const DataCodecMessageId id,
    const std::span<const DataCodecMessageArgument> arguments) {
    std::string result(DataCodecMessageTemplate(language, id));
    for (const auto& argument : arguments) {
        const std::string placeholder = "{" + argument.name + "}";
        std::size_t position = 0u;
        while ((position = result.find(placeholder, position)) != std::string::npos) {
            result.replace(position, placeholder.size(), argument.value);
            position += argument.value.size();
        }
    }
    return result;
}

DataCodecLocalizedMessage LocalizeDataCodecMessage(
    const DataCodecLanguage language,
    const DataCodecMessageId id,
    const std::initializer_list<DataCodecMessageArgument> arguments,
    std::string technicalDetail) {
    DataCodecLocalizedMessage result;
    result.language = language;
    result.id = id;
    result.arguments.assign(arguments.begin(), arguments.end());
    result.text = FormatDataCodecMessage(language, id, result.arguments);
    result.technicalDetail = std::move(technicalDetail);
    return result;
}

} // namespace datacodec
