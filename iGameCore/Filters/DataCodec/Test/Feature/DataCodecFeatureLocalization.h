#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATURELOCALIZATION_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATURELOCALIZATION_H

#include "DataCodec/Localization/DataCodecMessageCatalog.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <string>
#include <vector>

namespace datacodec::test {

[[nodiscard]] inline TestResult RunDataCodecFeatureLocalization() {
    TestResult result;

    const std::vector<DataCodecMessageArgument> attributeArguments{
        {"name", "Pressure"},
    };
    Require(
        result,
        FormatDataCodecMessage(
            DataCodecLanguage::SimplifiedChinese,
            DataCodecMessageId::EncodeAttributeNamed,
            attributeArguments) == "属性数据压缩：Pressure",
        "localization.zh.namedArgument",
        "Chinese message formatting should replace named arguments");
    Require(
        result,
        FormatDataCodecMessage(
            DataCodecLanguage::English,
            DataCodecMessageId::EncodeAttributeNamed,
            attributeArguments) == "Compressing attribute data: Pressure",
        "localization.en.namedArgument",
        "English message formatting should replace named arguments");

    const std::vector<DataCodecMessageArgument> frameArguments{
        {"count", "8"},
        {"index", "3"},
    };
    Require(
        result,
        FormatDataCodecMessage(
            DataCodecLanguage::English,
            DataCodecMessageId::FrameCounter,
            frameArguments) == "Frame 3/8",
        "localization.namedArgumentOrder",
        "message formatting should not depend on argument order");

    const auto localized = LocalizeDataCodecMessage(
        DataCodecLanguage::SimplifiedChinese,
        DataCodecMessageId::DecodeCompleted,
        {},
        "decoder returned a malformed payload");
    Require(
        result,
        localized.language == DataCodecLanguage::SimplifiedChinese &&
            localized.id == DataCodecMessageId::DecodeCompleted &&
            localized.text == "解压完成" &&
            localized.technicalDetail == "decoder returned a malformed payload",
        "localization.structuredMessage",
        "localized messages should preserve language, id, text, and technical detail");
    Require(
        result,
        std::string(DataCodecMessageIdName(DataCodecMessageId::DecodeCompleted)) ==
            "DecodeCompleted",
        "localization.stableMessageId",
        "message ids should have stable language-independent names");

    return result;
}

} // 命名空间 datacodec::test

#endif
