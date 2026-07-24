#ifndef DATACODEC_API_PARAMS_NUMERICARRAYPARAMS_H
#define DATACODEC_API_PARAMS_NUMERICARRAYPARAMS_H

#include "DataCodec/API/Params/ParamSerialization.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <utility>
namespace datacodec {

inline constexpr const char* kNumericArrayCompressorId = "sz3";

struct CompressorConfig {
    // 透传给 SZ3 的数值选项
    std::map<std::string, double> options;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(options);
    }
};

struct NumericArrayRegionPrecision {
    std::string name;
    bool hasCompressor{false};
    CompressorConfig compressor;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(name, hasCompressor, compressor);
    }
};

struct NumericArrayRegionRunNormalizePolicy {
    std::uint32_t maxRegionCount{8u};
    std::uint32_t maxRunsPerRegion{8u};
    ParamSize minCoreRunLength{4096u};
    double coreMinRatio{0.10};
    double minLongestRunRatio{0.20};
    ParamSize maxFragmentRunLength{512u};
    double maxFragmentElementRatio{0.05};
    ParamSize maxCoalesceGap{256u};
    double maxExpansionRatio{0.02};
    double maxRefinedElementRatio{8.0};

    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            maxRegionCount,
            maxRunsPerRegion,
            minCoreRunLength,
            coreMinRatio,
            minLongestRunRatio,
            maxFragmentRunLength,
            maxFragmentElementRatio,
            maxCoalesceGap,
            maxExpansionRatio,
            maxRefinedElementRatio);
    }
};

struct NumericArrayRegionControlParams {
    NumericArrayRegionPrecision defaultPrecision;
    NumericArrayRegionRunNormalizePolicy runPolicy;
    std::vector<NumericArrayRegionPrecision> regions;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(defaultPrecision, runPolicy, regions);
    }
};

struct RegionRun {
    ParamSize begin{0};
    ParamSize count{0};
    std::uint32_t regionId{1u};

    template<class Archive>
    void serialize(Archive& archive) {
        archive(begin, count, regionId);
    }
};

inline NumericArrayRegionPrecision MakeNumericArrayRegionPrecision(
    std::string name,
    CompressorConfig compressor) {
    NumericArrayRegionPrecision precision;
    precision.name = std::move(name);
    precision.hasCompressor = true;
    precision.compressor = std::move(compressor);
    return precision;
}

inline NumericArrayRegionControlParams MakeSingleRegionPrecisionControl(
    CompressorConfig compressor,
    std::string name = "default") {
    NumericArrayRegionControlParams control;
    control.defaultPrecision = MakeNumericArrayRegionPrecision(std::move(name), std::move(compressor));
    control.regions.clear();
    return control;
}

inline bool ResolveDefaultRegionCompressor(
    const NumericArrayRegionControlParams& control,
    CompressorConfig& compressor,
    std::string* error = nullptr) {
    compressor = {};
    if (!control.defaultPrecision.hasCompressor) {
        return validation::AssignError(error, "numeric array default region precision compressor is missing");
    }
    compressor = control.defaultPrecision.compressor;
    return true;
}

struct NumericArrayControlParams {
    // NumericArray 统一使用区域精度配置，单一区域表示整场默认精度
    NumericArrayRegionControlParams regionControl;
    std::vector<RegionRun> regionRuns;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(regionControl, regionRuns);
    }
};

struct NumericArrayComponentLayoutParams {
    std::uint32_t componentIndex{0};
    NumericArrayBytesCodec bytesCodec{NumericArrayBytesCodec::RawBytes};
    ParamSize encodedByteLength{0};

    template<class Archive>
    void save(Archive& archive) const {
        detail::SaveEnum(archive, bytesCodec);
        archive(componentIndex, encodedByteLength);
    }

    template<class Archive>
    void load(Archive& archive) {
        detail::LoadEnum(archive, bytesCodec);
        archive(componentIndex, encodedByteLength);
    }
};

struct NumericArrayRegionRunLayoutParams {
    ParamSize begin{0};
    ParamSize count{0};

    template<class Archive>
    void serialize(Archive& archive) {
        archive(begin, count);
    }
};

struct NumericArrayRegionLayerLayoutParams {
    std::uint32_t regionId{0};
    ParamSize originalElementCount{0};
    ParamSize refinedElementCount{0};
    ParamSize expandedBackgroundCount{0};
    CompressorConfig refineCompressor;
    NumericArrayBytesCodec residualBytesCodec{NumericArrayBytesCodec::NumericArrayCodec};
    ParamSize residualEncodedByteLength{0};
    std::vector<NumericArrayRegionRunLayoutParams> runs;
    std::vector<NumericArrayComponentLayoutParams> componentLayouts;

    template<class Archive>
    void save(Archive& archive) const {
        detail::SaveEnum(archive, residualBytesCodec);
        archive(
            regionId,
            originalElementCount,
            refinedElementCount,
            expandedBackgroundCount,
            refineCompressor,
            residualEncodedByteLength,
            runs,
            componentLayouts);
    }

    template<class Archive>
    void load(Archive& archive) {
        detail::LoadEnum(archive, residualBytesCodec);
        archive(
            regionId,
            originalElementCount,
            refinedElementCount,
            expandedBackgroundCount,
            refineCompressor,
            residualEncodedByteLength,
            runs,
            componentLayouts);
    }
};

struct NumericArrayBlockLayoutParams {
    NumericArrayBlockMode mode{NumericArrayBlockMode::NonReference};
    NumericArrayReferenceKind referenceKind{NumericArrayReferenceKind::None};
    NumericArrayReferenceCodecId codecId{NumericArrayReferenceCodecId::NonReference};
    std::uint16_t localParentFieldIndex{0xFFFFu};
    ParamSize elementOffset{0};
    ParamSize elementCount{0};
    ParamSize encodedByteLength{0};
    NumericArrayBytesCodec bytesCodec{NumericArrayBytesCodec::RawBytes};
    CompressorConfig backgroundCompressor;
    ParamSize backgroundEncodedByteLength{0};
    std::int32_t predictorOffset{0};
    std::vector<NumericArrayComponentLayoutParams> componentLayouts;
    std::vector<NumericArrayRegionLayerLayoutParams> regionLayers;
    std::vector<double> alpha;
    std::vector<double> beta;

    template<class Archive>
    void save(Archive& archive) const {
        detail::SaveEnum(archive, mode);
        detail::SaveEnum(archive, referenceKind);
        detail::SaveEnum(archive, codecId);
        detail::SaveEnum(archive, bytesCodec);
        archive(
            localParentFieldIndex,
            elementOffset,
            elementCount,
            encodedByteLength,
            backgroundCompressor,
            backgroundEncodedByteLength,
            predictorOffset,
            componentLayouts,
            regionLayers,
            alpha,
            beta);
    }

    template<class Archive>
    void load(Archive& archive) {
        detail::LoadEnum(archive, mode);
        detail::LoadEnum(archive, referenceKind);
        detail::LoadEnum(archive, codecId);
        detail::LoadEnum(archive, bytesCodec);
        archive(
            localParentFieldIndex,
            elementOffset,
            elementCount,
            encodedByteLength,
            backgroundCompressor,
            backgroundEncodedByteLength,
            predictorOffset,
            componentLayouts,
            regionLayers,
            alpha,
            beta);
    }
};

struct NumericArrayStorageParams {
    // 回退或压缩成功后最终落下的 section codec
    EncodedFieldCodecType codecType{EncodedFieldCodecType::Unknown};
    // 源 NumericArray 的标量类型
    DataType dataType{DataType::Float32};
    // 字段的标量字节宽度、元素个数和元组维度
    ParamSize valueSize{sizeof(float)};
    ParamSize elementCount{0};
    std::int32_t dimension{0};
    // payload 内每个 block 的解码布局
    std::vector<NumericArrayBlockLayoutParams> blockLayouts;

    template<class Archive>
    void save(Archive& archive) const {
        detail::SaveEnum(archive, codecType);
        detail::SaveEnum(archive, dataType);
        archive(valueSize, elementCount, dimension, blockLayouts);
    }

    template<class Archive>
    void load(Archive& archive) {
        detail::LoadEnum(archive, codecType);
        detail::LoadEnum(archive, dataType);
        archive(valueSize, elementCount, dimension, blockLayouts);
    }
};

} // namespace datacodec

#endif
