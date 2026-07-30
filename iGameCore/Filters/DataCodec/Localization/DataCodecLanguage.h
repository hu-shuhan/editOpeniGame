#ifndef DATACODEC_LOCALIZATION_DATACODECLANGUAGE_H
#define DATACODEC_LOCALIZATION_DATACODECLANGUAGE_H

#include <cstdint>

namespace datacodec {

enum class DataCodecLanguage : std::uint8_t {
    English = 0u,
    SimplifiedChinese = 1u,
};

[[nodiscard]] inline const char* DataCodecLanguageName(
    const DataCodecLanguage language) noexcept {
    return language == DataCodecLanguage::English ? "en" : "zh-CN";
}

} // namespace datacodec

#endif
