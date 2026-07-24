#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREREGIONPRECISION_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREREGIONPRECISION_H

#include "DataCodec/API/Params/NumericArrayParams.h"
#include "DataCodec/Codec/NumericArray/NumericArrayRegionPlan.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <span>
#include <string>
#include <vector>

namespace datacodec::test {

inline CompressorConfig MakeRegionPrecisionTestCompressor(const double absolutePrecision) {
    CompressorConfig compressor;
    compressor.options["pressio:abs"] = absolutePrecision;
    return compressor;
}

inline NumericArrayRegionControlParams MakeRegionPrecisionTestControl(
    const double defaultPrecision,
    const double customPrecision) {
    NumericArrayRegionControlParams control;
    control.defaultPrecision = MakeNumericArrayRegionPrecision(
        "default",
        MakeRegionPrecisionTestCompressor(defaultPrecision));
    control.regions.push_back(MakeNumericArrayRegionPrecision(
        "custom",
        MakeRegionPrecisionTestCompressor(customPrecision)));
    control.runPolicy.maxRunsPerRegion = 16u;
    control.runPolicy.minCoreRunLength = 1u;
    control.runPolicy.coreMinRatio = 0.0;
    control.runPolicy.minLongestRunRatio = 0.0;
    control.runPolicy.maxFragmentRunLength = 16u;
    control.runPolicy.maxFragmentElementRatio = 1.0;
    control.runPolicy.maxCoalesceGap = 16u;
    control.runPolicy.maxExpansionRatio = 1.0;
    control.runPolicy.maxRefinedElementRatio = 8.0;
    return control;
}

[[nodiscard]] inline TestResult RunDataCodecFeatureRegionPrecision() noexcept {
    TestResult result;
    const std::vector<RegionRun> customRuns{
        RegionRun{.begin = 2u, .count = 2u, .regionId = 1u},
    };

    {
        auto control = MakeRegionPrecisionTestControl(0.01, 0.1);
        numericarray::LayeredResidualRegionPlan plan;
        std::string error;
        const auto built = numericarray::BuildNormalizedRegionPlansFromRegionRuns(
            std::span<const RegionRun>(customRuns.data(), customRuns.size()),
            6u,
            0u,
            6u,
            control,
            plan,
            &error);
        Require(
            result,
            built,
            "regionPrecision.defaultComplement.build",
            error.empty() ? "default complement plan was rejected" : error);
        const bool hasExpectedComplement =
            built &&
            plan.layers.size() == 1u &&
            plan.layers[0].runs.size() == 2u &&
            plan.layers[0].runs[0].begin == 0u &&
            plan.layers[0].runs[0].count == 2u &&
            plan.layers[0].runs[1].begin == 4u &&
            plan.layers[0].runs[1].count == 2u;
        Require(
            result,
            hasExpectedComplement,
            "regionPrecision.defaultComplement.runs",
            "stricter default precision did not produce the custom-region complement");
    }

    {
        auto control = MakeRegionPrecisionTestControl(0.1, 0.01);
        numericarray::LayeredResidualRegionPlan plan;
        std::string error;
        const auto built = numericarray::BuildNormalizedRegionPlansFromRegionRuns(
            std::span<const RegionRun>(customRuns.data(), customRuns.size()),
            6u,
            0u,
            6u,
            control,
            plan,
            &error);
        Require(
            result,
            built,
            "regionPrecision.customRefinement.build",
            error.empty() ? "custom refinement plan was rejected" : error);
        const bool hasExpectedCustomRun =
            built &&
            plan.layers.size() == 1u &&
            plan.layers[0].runs.size() == 1u &&
            plan.layers[0].runs[0].begin == 2u &&
            plan.layers[0].runs[0].count == 2u;
        Require(
            result,
            hasExpectedCustomRun,
            "regionPrecision.customRefinement.runs",
            "stricter custom precision did not retain its selected run");
    }

    {
        auto control = MakeRegionPrecisionTestControl(0.01, 0.1);
        numericarray::LayeredResidualRegionPlan plan;
        std::string error;
        const auto built = numericarray::BuildNormalizedRegionPlansFromRegionRuns(
            std::span<const RegionRun>(customRuns.data(), customRuns.size()),
            6u,
            1u,
            4u,
            control,
            plan,
            &error);
        const bool hasExpectedClippedComplement =
            built &&
            plan.layers.size() == 1u &&
            plan.layers[0].runs.size() == 2u &&
            plan.layers[0].runs[0].begin == 0u &&
            plan.layers[0].runs[0].count == 1u &&
            plan.layers[0].runs[1].begin == 3u &&
            plan.layers[0].runs[1].count == 1u;
        Require(
            result,
            hasExpectedClippedComplement,
            "regionPrecision.defaultComplement.blockClip",
            error.empty() ? "default complement was not clipped to the block" : error);
    }

    return result;
}

} // namespace datacodec::test

#endif
