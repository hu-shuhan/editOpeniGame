#ifndef DATACODEC_API_PARAMS_ENCODEPIPELINEPARAMS_H
#define DATACODEC_API_PARAMS_ENCODEPIPELINEPARAMS_H

#include <cstddef>
#include <cstdint>

namespace datacodec {

enum class EncodePointOrderMode : std::uint8_t {
    Original = 0,
    Morton = 1,
};

enum class EncodeCellOrderMode : std::uint8_t {
    Original = 0,
    Morton = 1,
};

enum class PackageFieldEncodingMode : std::uint8_t {
    Raw = 0,
    Zstd = 1,
};

struct PackageFieldEncodingParams {
    PackageFieldEncodingMode mode{PackageFieldEncodingMode::Zstd};
    int zstdLevel{3};
    std::size_t workerCount{4u};
};

struct EncodePipelineControlParams {
    EncodePointOrderMode pointOrder{EncodePointOrderMode::Morton};
    EncodeCellOrderMode cellOrder{EncodeCellOrderMode::Morton};
    PackageFieldEncodingParams packageFields;
};

[[nodiscard]] inline const char* EncodePointOrderModeName(
    const EncodePointOrderMode mode) noexcept {
    return mode == EncodePointOrderMode::Morton ? "Morton" : "Original";
}

[[nodiscard]] inline const char* EncodeCellOrderModeName(
    const EncodeCellOrderMode mode) noexcept {
    return mode == EncodeCellOrderMode::Morton ? "Morton" : "Original";
}

[[nodiscard]] inline const char* PackageFieldEncodingModeName(
    const PackageFieldEncodingMode mode) noexcept {
    return mode == PackageFieldEncodingMode::Zstd ? "Zstd" : "Raw";
}

} // namespace datacodec

#endif
