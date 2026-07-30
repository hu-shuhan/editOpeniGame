#ifndef DATACODEC_LOCALIZATION_DATACODECMESSAGECATALOG_H
#define DATACODEC_LOCALIZATION_DATACODECMESSAGECATALOG_H

#include "DataCodec/Localization/DataCodecLanguage.h"
#include "DataCodec/Localization/DataCodecMessageId.h"

#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace datacodec {

struct DataCodecMessageArgument {
    std::string name;
    std::string value;
};

struct DataCodecLocalizedMessage {
    DataCodecLanguage language{DataCodecLanguage::SimplifiedChinese};
    DataCodecMessageId id{DataCodecMessageId::None};
    std::vector<DataCodecMessageArgument> arguments;
    std::string text;
    std::string technicalDetail;
};

[[nodiscard]] std::string_view DataCodecMessageTemplate(
    DataCodecLanguage language,
    DataCodecMessageId id) noexcept;

[[nodiscard]] std::string FormatDataCodecMessage(
    DataCodecLanguage language,
    DataCodecMessageId id,
    std::span<const DataCodecMessageArgument> arguments = {});

[[nodiscard]] DataCodecLocalizedMessage LocalizeDataCodecMessage(
    DataCodecLanguage language,
    DataCodecMessageId id,
    std::initializer_list<DataCodecMessageArgument> arguments = {},
    std::string technicalDetail = {});

namespace localizationdetail {

[[nodiscard]] std::string_view DataCodecEnglishMessageTemplate(
    DataCodecMessageId id) noexcept;
[[nodiscard]] std::string_view DataCodecSimplifiedChineseMessageTemplate(
    DataCodecMessageId id) noexcept;

} // namespace localizationdetail

} // namespace datacodec

#endif
