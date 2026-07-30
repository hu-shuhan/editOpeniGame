#ifndef iGameDataCodecFeatureLocalization_h
#define iGameDataCodecFeatureLocalization_h

#include "DataCodec/Filter/Localization/iGameDataCodecHostMessage.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <iostream>

namespace iGame::datacodec_test {

[[nodiscard]] inline int RunDataCodecFeatureHostLocalization() {
    ::datacodec::test::TestResult result;

    ::datacodec::test::Require(
        result,
        iGameDataCodecHostMessage(
            ::datacodec::DataCodecLanguage::SimplifiedChinese,
            iGameDataCodecHostMessageId::OutputPathSet,
            {{"path", "D:/data/output.igc"}}) ==
            "输出路径已设置为：D:/data/output.igc",
        "hostLocalization.zh.namedArgument",
        "Chinese host messages should replace named arguments");
    ::datacodec::test::Require(
        result,
        iGameDataCodecHostMessage(
            ::datacodec::DataCodecLanguage::English,
            iGameDataCodecHostMessageId::MissingRegionFeatures,
            {{"count", "3"}}) ==
            "Cannot start compression: 3 fields have custom regions without computed features",
        "hostLocalization.en.namedArgument",
        "English host messages should replace named arguments");
    ::datacodec::test::Require(
        result,
        iGameDataCodecHostMessage(
            ::datacodec::DataCodecLanguage::SimplifiedChinese,
            iGameDataCodecHostMessageId::DecodeFailed) == "iGame DataCodec 解码失败" &&
            iGameDataCodecHostMessage(
                ::datacodec::DataCodecLanguage::English,
                iGameDataCodecHostMessageId::DecodeFailed) ==
                "iGame DataCodec decoding failed",
        "hostLocalization.languageSelection",
        "host message language selection should not depend on Qt translation state");

    for (const auto& failure : result.failures) {
        std::cerr << failure.check << ": " << failure.message << '\n';
    }
    return result.passed ? 0 : 1;
}

} // 命名空间 iGame::datacodec_test

#endif
