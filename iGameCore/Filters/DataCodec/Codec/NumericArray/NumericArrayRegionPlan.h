#ifndef DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYREGIONPLAN_H
#define DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYREGIONPLAN_H

#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/NumericArrayParams.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec::numericarray {

enum class PrecisionConfigComparison {
    Equivalent,
    LeftLooser,
    LeftStricter,
    Incomparable
};

struct RegionPlanQuality {
    double coreRatio{0.0};
    double fragmentRatio{0.0};
    double expansionRatio{0.0};
    std::uint32_t runCount{0u};
};

struct RegionPlan {
    std::uint32_t regionId{0u};
    CompressorConfig refineCompressor;
    ParamSize originalElementCount{0u};
    ParamSize refinedElementCount{0u};
    ParamSize expandedBackgroundCount{0u};
    std::vector<NumericArrayRegionRunLayoutParams> runs;
    RegionPlanQuality quality;
};

struct LayeredResidualRegionPlan {
    CompressorConfig backgroundCompressor;
    std::vector<RegionPlan> layers;
};

struct RegionPrecisionLevel {
    CompressorConfig compressor;
    std::uint32_t firstLabel{0u};
};

struct PrecisionGapCandidate {
    std::size_t gapIndex{0u};
    ParamSize promotedElementCount{0u};
    std::uint64_t weightedLiftCost{0u};
    ParamSize begin{0u};
};

inline bool IsPrecisionOptionKey(const std::string& key) noexcept {
    return key == "pressio:abs" || key == "pressio:rel";
}

inline bool TryGetCompressorOption(
    const CompressorConfig& compressor,
    const char* key,
    double& value) noexcept {
    const auto iterator = compressor.options.find(key);
    if (iterator == compressor.options.end()) {
        return false;
    }
    value = iterator->second;
    return true;
}

inline bool AreNonPrecisionOptionsEqual(
    const CompressorConfig& left,
    const CompressorConfig& right) noexcept {
    for (const auto& option : left.options) {
        if (IsPrecisionOptionKey(option.first)) {
            continue;
        }
        const auto rightOption = right.options.find(option.first);
        if (rightOption == right.options.end() || rightOption->second != option.second) {
            return false;
        }
    }
    for (const auto& option : right.options) {
        if (IsPrecisionOptionKey(option.first)) {
            continue;
        }
        if (left.options.find(option.first) == left.options.end()) {
            return false;
        }
    }
    return true;
}

inline PrecisionConfigComparison CompareCompressorPrecision(
    const CompressorConfig& left,
    const CompressorConfig& right) noexcept {
    if (!AreNonPrecisionOptionsEqual(left, right)) {
        return PrecisionConfigComparison::Incomparable;
    }

    int direction = 0;
    bool sawPrecisionOption = false;
    const char* precisionKeys[] = {"pressio:abs", "pressio:rel"};
    for (const auto* key : precisionKeys) {
        double leftValue = 0.0;
        double rightValue = 0.0;
        const bool hasLeft = TryGetCompressorOption(left, key, leftValue);
        const bool hasRight = TryGetCompressorOption(right, key, rightValue);
        if (hasLeft != hasRight) {
            return PrecisionConfigComparison::Incomparable;
        }
        if (!hasLeft) {
            continue;
        }
        sawPrecisionOption = true;
        if (!(std::isfinite(leftValue) && std::isfinite(rightValue)) ||
            leftValue < 0.0 ||
            rightValue < 0.0) {
            return PrecisionConfigComparison::Incomparable;
        }
        const int currentDirection = leftValue > rightValue ? 1 : (leftValue < rightValue ? -1 : 0);
        if (currentDirection == 0) {
            continue;
        }
        if (direction != 0 && direction != currentDirection) {
            return PrecisionConfigComparison::Incomparable;
        }
        direction = currentDirection;
    }

    if (direction > 0) {
        return PrecisionConfigComparison::LeftLooser;
    }
    if (direction < 0) {
        return PrecisionConfigComparison::LeftStricter;
    }
    return sawPrecisionOption || left.options == right.options
        ? PrecisionConfigComparison::Equivalent
        : PrecisionConfigComparison::Incomparable;
}

inline bool ValidateRegionPrecision(
    const NumericArrayRegionPrecision& precision,
    const std::string& name,
    std::string* error) {
    if (!precision.hasCompressor) {
        return validation::AssignError(error, name + " is missing an explicit compressor");
    }
    return true;
}

inline bool ValidateRegionControlForEncode(
    const NumericArrayRegionControlParams& control,
    std::string* error = nullptr) {
    if (!ValidateRegionPrecision(control.defaultPrecision, "default region precision", error)) {
        return false;
    }
    if (control.regions.size() > control.runPolicy.maxRegionCount) {
        return validation::AssignError(error, "region precision count exceeds the configured region limit");
    }
    if (control.runPolicy.maxRunsPerRegion == 0u) {
        return validation::AssignError(error, "region run policy requires at least one run per precision layer");
    }
    if (control.runPolicy.maxFragmentElementRatio < 0.0 ||
        control.runPolicy.maxFragmentElementRatio > 1.0 ||
        control.runPolicy.maxExpansionRatio < 0.0 ||
        control.runPolicy.maxExpansionRatio > 1.0 ||
        control.runPolicy.maxRefinedElementRatio < 0.0) {
        return validation::AssignError(error, "region run policy ratio is invalid");
    }
    for (std::size_t index = 0u; index < control.regions.size(); ++index) {
        if (!ValidateRegionPrecision(
                control.regions[index],
                "region " + std::to_string(index + 1u) + " precision",
                error)) {
            return false;
        }
    }
    return true;
}

inline bool BuildPrecisionLevels(
    const NumericArrayRegionControlParams& control,
    std::vector<RegionPrecisionLevel>& levels,
    std::vector<std::size_t>& labelToLevel,
    std::string* error = nullptr) {
    levels.clear();
    labelToLevel.clear();
    if (!ValidateRegionControlForEncode(control, error)) {
        return false;
    }

    const auto labelCount = control.regions.size() + 1u;
    std::vector<RegionPrecisionLevel> targets;
    targets.reserve(labelCount);
    targets.push_back(RegionPrecisionLevel{
        .compressor = control.defaultPrecision.compressor,
        .firstLabel = 0u,
    });
    for (std::size_t index = 0u; index < control.regions.size(); ++index) {
        targets.push_back(RegionPrecisionLevel{
            .compressor = control.regions[index].compressor,
            .firstLabel = static_cast<std::uint32_t>(index + 1u),
        });
    }

    for (std::size_t left = 0u; left < targets.size(); ++left) {
        for (std::size_t right = left + 1u; right < targets.size(); ++right) {
            if (CompareCompressorPrecision(targets[left].compressor, targets[right].compressor) ==
                PrecisionConfigComparison::Incomparable) {
                return validation::AssignError(
                    error,
                    "region precision compressors are not comparable by supported precision options");
            }
        }
    }

    for (const auto& target : targets) {
        bool merged = false;
        for (auto& level : levels) {
            if (CompareCompressorPrecision(level.compressor, target.compressor) ==
                PrecisionConfigComparison::Equivalent) {
                level.firstLabel = std::min(level.firstLabel, target.firstLabel);
                merged = true;
                break;
            }
        }
        if (!merged) {
            levels.push_back(target);
        }
    }

    std::stable_sort(
        levels.begin(),
        levels.end(),
        [](const RegionPrecisionLevel& left, const RegionPrecisionLevel& right) {
            const auto comparison = CompareCompressorPrecision(left.compressor, right.compressor);
            if (comparison == PrecisionConfigComparison::LeftLooser) {
                return true;
            }
            if (comparison == PrecisionConfigComparison::LeftStricter) {
                return false;
            }
            return left.firstLabel < right.firstLabel;
        });

    labelToLevel.assign(labelCount, 0u);
    for (std::size_t label = 0u; label < labelCount; ++label) {
        const auto& compressor = label == 0u
            ? control.defaultPrecision.compressor
            : control.regions[label - 1u].compressor;
        bool found = false;
        for (std::size_t levelIndex = 0u; levelIndex < levels.size(); ++levelIndex) {
            if (CompareCompressorPrecision(levels[levelIndex].compressor, compressor) ==
                PrecisionConfigComparison::Equivalent) {
                labelToLevel[label] = levelIndex;
                found = true;
                break;
            }
        }
        if (!found) {
            return validation::AssignError(error, "failed to resolve region precision level");
        }
    }
    return true;
}

inline bool ValidateRegionRunsForEncode(
    const std::span<const RegionRun> runs,
    const ParamSize totalElementCount,
    const std::size_t regionCount,
    std::string* error = nullptr) {
    std::vector<RegionRun> sortedRuns(runs.begin(), runs.end());
    for (const auto& run : sortedRuns) {
        if (run.count == 0u) {
            return validation::AssignError(error, "region run count must be non-zero");
        }
        if (run.regionId == 0u || run.regionId > regionCount) {
            return validation::AssignError(error, "region run id exceeds configured region count");
        }
        if (run.begin > totalElementCount || run.count > totalElementCount - run.begin) {
            return validation::AssignError(error, "region run exceeds numeric array element count");
        }
    }
    std::sort(
        sortedRuns.begin(),
        sortedRuns.end(),
        [](const RegionRun& left, const RegionRun& right) {
            if (left.begin != right.begin) {
                return left.begin < right.begin;
            }
            return left.count < right.count;
        });
    ParamSize previousEnd = 0u;
    bool havePrevious = false;
    for (const auto& run : sortedRuns) {
        if (havePrevious && run.begin < previousEnd) {
            return validation::AssignError(error, "region runs must not overlap");
        }
        previousEnd = run.begin + run.count;
        havePrevious = true;
    }
    return true;
}

inline bool BuildPrecisionTargetRunsFromRegionRuns(
    const std::span<const RegionRun> regionRuns,
    const std::span<const std::size_t> labelToLevel,
    const std::size_t precisionLevel,
    const ParamSize blockElementOffset,
    const ParamSize blockElementCount,
    std::vector<NumericArrayRegionRunLayoutParams>& runs,
    ParamSize& targetElementCount,
    std::string* error = nullptr) {
    runs.clear();
    targetElementCount = 0u;
    ParamSize blockEnd = 0u;
    if (!validation::CheckedAddU64(
            blockElementOffset,
            blockElementCount,
            blockEnd,
            "region run block range",
            error)) {
        return false;
    }

    struct ClippedRegionRun {
        ParamSize begin{0u};
        ParamSize end{0u};
        std::uint32_t regionId{0u};
    };
    std::vector<ClippedRegionRun> clippedRegionRuns;
    clippedRegionRuns.reserve(regionRuns.size());
    for (const auto& run : regionRuns) {
        if (run.regionId >= labelToLevel.size()) {
            return validation::AssignError(error, "region run id exceeds configured precision levels");
        }
        ParamSize runEnd = 0u;
        if (!validation::CheckedAddU64(run.begin, run.count, runEnd, "region run range", error)) {
            return false;
        }
        const auto clippedBegin = std::max(run.begin, blockElementOffset);
        const auto clippedEnd = std::min(runEnd, blockEnd);
        if (clippedBegin >= clippedEnd) {
            continue;
        }
        clippedRegionRuns.push_back(ClippedRegionRun{
            .begin = clippedBegin,
            .end = clippedEnd,
            .regionId = run.regionId,
        });
    }
    std::sort(
        clippedRegionRuns.begin(),
        clippedRegionRuns.end(),
        [](const ClippedRegionRun& left, const ClippedRegionRun& right) {
            if (left.begin != right.begin) {
                return left.begin < right.begin;
            }
            return left.end < right.end;
        });

    const bool includeDefaultRegion = !labelToLevel.empty() && labelToLevel[0u] >= precisionLevel;
    ParamSize cursor = blockElementOffset;
    for (const auto& run : clippedRegionRuns) {
        if (run.begin < cursor) {
            return validation::AssignError(error, "selected precision region runs overlap");
        }
        if (includeDefaultRegion && cursor < run.begin) {
            runs.push_back(NumericArrayRegionRunLayoutParams{
                .begin = cursor - blockElementOffset,
                .count = run.begin - cursor,
            });
        }
        if (labelToLevel[run.regionId] >= precisionLevel) {
            runs.push_back(NumericArrayRegionRunLayoutParams{
                .begin = run.begin - blockElementOffset,
                .count = run.end - run.begin,
            });
        }
        cursor = run.end;
    }
    if (includeDefaultRegion && cursor < blockEnd) {
        runs.push_back(NumericArrayRegionRunLayoutParams{
            .begin = cursor - blockElementOffset,
            .count = blockEnd - cursor,
        });
    }

    std::vector<NumericArrayRegionRunLayoutParams> mergedRuns;
    for (const auto& run : runs) {
        if (mergedRuns.empty()) {
            mergedRuns.push_back(run);
            continue;
        }
        auto& previous = mergedRuns.back();
        const auto previousEnd = previous.begin + previous.count;
        if (run.begin < previousEnd) {
            return validation::AssignError(error, "selected precision region runs overlap");
        }
        if (run.begin == previousEnd) {
            previous.count += run.count;
            continue;
        }
        mergedRuns.push_back(run);
    }
    runs = std::move(mergedRuns);
    for (const auto& run : runs) {
        if (!validation::CheckedAddU64(
                targetElementCount,
                run.count,
                targetElementCount,
                "precision target element count",
                error)) {
            return false;
        }
    }
    return true;
}

inline bool BuildPrecisionGapCandidatesFromRuns(
    const NumericArrayRegionRunNormalizePolicy& policy,
    const std::span<const NumericArrayRegionRunLayoutParams> runs,
    std::vector<PrecisionGapCandidate>& gaps,
    std::string* error = nullptr) {
    gaps.clear();
    if (runs.size() < 2u) {
        return true;
    }
    gaps.reserve(runs.size() - 1u);
    for (std::size_t index = 0u; index + 1u < runs.size(); ++index) {
        const auto previousEnd = runs[index].begin + runs[index].count;
        if (runs[index + 1u].begin < previousEnd) {
            return validation::AssignError(error, "precision layer run normalization produced overlapping runs");
        }
        const auto gapCount = runs[index + 1u].begin - previousEnd;
        if (gapCount == 0u || gapCount > policy.maxCoalesceGap) {
            continue;
        }
        gaps.push_back(PrecisionGapCandidate{
            .gapIndex = index,
            .promotedElementCount = gapCount,
            .weightedLiftCost = gapCount,
            .begin = previousEnd,
        });
    }
    return true;
}

inline bool SelectPrecisionGaps(
    const std::span<const PrecisionGapCandidate> gaps,
    const std::size_t requiredGapCount,
    const ParamSize totalElementCount,
    const NumericArrayRegionRunNormalizePolicy& policy,
    std::vector<std::uint8_t>& selectedGaps,
    ParamSize& promotedElementCount,
    std::string* error = nullptr) {
    promotedElementCount = 0u;
    if (requiredGapCount == 0u) {
        return true;
    }
    std::vector<PrecisionGapCandidate> sortedGaps(gaps.begin(), gaps.end());
    std::sort(
        sortedGaps.begin(),
        sortedGaps.end(),
        [](const PrecisionGapCandidate& left, const PrecisionGapCandidate& right) {
            if (left.promotedElementCount != right.promotedElementCount) {
                return left.promotedElementCount < right.promotedElementCount;
            }
            if (left.weightedLiftCost != right.weightedLiftCost) {
                return left.weightedLiftCost < right.weightedLiftCost;
            }
            return left.begin < right.begin;
        });

    std::size_t selectedCount = 0u;
    for (const auto& gap : sortedGaps) {
        const auto projectedPromoted = promotedElementCount + gap.promotedElementCount;
        const double expansionRatio = totalElementCount == 0u
            ? 0.0
            : static_cast<double>(projectedPromoted) / static_cast<double>(totalElementCount);
        if (expansionRatio > policy.maxExpansionRatio) {
            continue;
        }
        if (gap.gapIndex >= selectedGaps.size()) {
            return validation::AssignError(error, "precision layer gap index is invalid");
        }
        selectedGaps[gap.gapIndex] = 1u;
        promotedElementCount = projectedPromoted;
        ++selectedCount;
        if (selectedCount == requiredGapCount) {
            return true;
        }
    }
    return validation::AssignError(
        error,
        "precision layer requires more gap merges than the policy can accept");
}

inline void BuildMergedPrecisionRuns(
    const std::span<const NumericArrayRegionRunLayoutParams> inputRuns,
    const std::span<const std::uint8_t> selectedGaps,
    std::vector<NumericArrayRegionRunLayoutParams>& outputRuns) {
    outputRuns.clear();
    if (inputRuns.empty()) {
        return;
    }
    ParamSize currentBegin = inputRuns.front().begin;
    ParamSize currentEnd = inputRuns.front().begin + inputRuns.front().count;
    for (std::size_t index = 0u; index + 1u < inputRuns.size(); ++index) {
        const auto nextEnd = inputRuns[index + 1u].begin + inputRuns[index + 1u].count;
        if (index < selectedGaps.size() && selectedGaps[index] != 0u) {
            currentEnd = nextEnd;
            continue;
        }
        outputRuns.push_back(NumericArrayRegionRunLayoutParams{
            .begin = currentBegin,
            .count = currentEnd - currentBegin,
        });
        currentBegin = inputRuns[index + 1u].begin;
        currentEnd = nextEnd;
    }
    outputRuns.push_back(NumericArrayRegionRunLayoutParams{
        .begin = currentBegin,
        .count = currentEnd - currentBegin,
    });
}

inline bool ValidatePrecisionLayerQuality(
    const std::size_t precisionLevel,
    const NumericArrayRegionRunNormalizePolicy& policy,
    const ParamSize targetElementCount,
    const ParamSize totalElementCount,
    const ParamSize promotedElementCount,
    const std::vector<NumericArrayRegionRunLayoutParams>& runs,
    RegionPlanQuality& quality,
    std::string* error = nullptr) {
    quality = {};
    if (targetElementCount == 0u) {
        return true;
    }
    ParamSize coreElementCount = 0u;
    ParamSize longestRunCount = 0u;
    std::size_t longestRunIndex = 0u;
    std::vector<std::uint8_t> isCore(runs.size(), 0u);
    for (std::size_t index = 0u; index < runs.size(); ++index) {
        const auto& run = runs[index];
        if (run.count > longestRunCount) {
            longestRunCount = run.count;
            longestRunIndex = index;
        }
        const double runRatio = static_cast<double>(run.count) / static_cast<double>(targetElementCount);
        if (run.count >= policy.minCoreRunLength || runRatio >= policy.coreMinRatio) {
            isCore[index] = 1u;
            coreElementCount += run.count;
        }
    }
    if (coreElementCount == 0u) {
        const double longestRatio =
            static_cast<double>(longestRunCount) / static_cast<double>(targetElementCount);
        if (longestRatio < policy.minLongestRunRatio) {
            return validation::AssignError(
                error,
                "precision layer " + std::to_string(precisionLevel) + " does not contain an acceptable core run");
        }
        isCore[longestRunIndex] = 1u;
        coreElementCount = longestRunCount;
    }

    ParamSize fragmentElementCount = 0u;
    for (std::size_t index = 0u; index < runs.size(); ++index) {
        if (isCore[index] != 0u) {
            continue;
        }
        if (runs[index].count > policy.maxFragmentRunLength) {
            return validation::AssignError(
                error,
                "precision layer " + std::to_string(precisionLevel) + " contains a fragment run above the limit");
        }
        fragmentElementCount += runs[index].count;
    }

    const double fragmentRatio =
        static_cast<double>(fragmentElementCount) / static_cast<double>(targetElementCount);
    if (fragmentRatio > policy.maxFragmentElementRatio) {
        return validation::AssignError(
            error,
            "precision layer " + std::to_string(precisionLevel) + " fragment ratio exceeds the configured limit");
    }

    quality.coreRatio = static_cast<double>(coreElementCount) / static_cast<double>(targetElementCount);
    quality.fragmentRatio = fragmentRatio;
    quality.expansionRatio = totalElementCount == 0u
        ? 0.0
        : static_cast<double>(promotedElementCount) / static_cast<double>(totalElementCount);
    quality.runCount = static_cast<std::uint32_t>(runs.size());
    return true;
}

inline bool BuildPrecisionLayerPlanFromRegionRuns(
    const std::span<const RegionRun> regionRuns,
    const std::span<const std::size_t> labelToLevel,
    const std::size_t precisionLevel,
    const ParamSize blockElementOffset,
    const ParamSize blockElementCount,
    const NumericArrayRegionRunNormalizePolicy& policy,
    RegionPlan& plan,
    std::string* error = nullptr) {
    plan = {};
    std::vector<NumericArrayRegionRunLayoutParams> targetRuns;
    ParamSize targetElementCount = 0u;
    if (!BuildPrecisionTargetRunsFromRegionRuns(
            regionRuns,
            labelToLevel,
            precisionLevel,
            blockElementOffset,
            blockElementCount,
            targetRuns,
            targetElementCount,
            error)) {
        return false;
    }
    if (targetElementCount == 0u) {
        return true;
    }

    ParamSize promotedElementCount = 0u;
    std::vector<NumericArrayRegionRunLayoutParams> outputRuns;
    if (targetRuns.size() > policy.maxRunsPerRegion) {
        std::vector<PrecisionGapCandidate> gaps;
        if (!BuildPrecisionGapCandidatesFromRuns(
                policy,
                std::span<const NumericArrayRegionRunLayoutParams>(targetRuns.data(), targetRuns.size()),
                gaps,
                error)) {
            return false;
        }
        const auto requiredGapCount =
            targetRuns.size() - static_cast<std::size_t>(policy.maxRunsPerRegion);
        std::vector<std::uint8_t> selectedGaps(targetRuns.size() - 1u, 0u);
        if (!SelectPrecisionGaps(
                std::span<const PrecisionGapCandidate>(gaps.data(), gaps.size()),
                requiredGapCount,
                blockElementCount,
                policy,
                selectedGaps,
                promotedElementCount,
                error)) {
            return false;
        }
        BuildMergedPrecisionRuns(
            std::span<const NumericArrayRegionRunLayoutParams>(targetRuns.data(), targetRuns.size()),
            std::span<const std::uint8_t>(selectedGaps.data(), selectedGaps.size()),
            outputRuns);
    } else {
        outputRuns = std::move(targetRuns);
    }

    if (outputRuns.size() > policy.maxRunsPerRegion) {
        return validation::AssignError(
            error,
            "precision layer " + std::to_string(precisionLevel) + " run count exceeds the configured limit");
    }
    ParamSize refinedElementCount = 0u;
    for (const auto& run : outputRuns) {
        refinedElementCount += run.count;
    }
    RegionPlanQuality quality;
    if (!ValidatePrecisionLayerQuality(
            precisionLevel,
            policy,
            targetElementCount,
            blockElementCount,
            promotedElementCount,
            outputRuns,
            quality,
            error)) {
        return false;
    }

    plan.regionId = static_cast<std::uint32_t>(precisionLevel);
    plan.originalElementCount = targetElementCount;
    plan.refinedElementCount = refinedElementCount;
    plan.expandedBackgroundCount = promotedElementCount;
    plan.runs = std::move(outputRuns);
    plan.quality = quality;
    return true;
}

inline bool BuildNormalizedRegionPlansFromRegionRuns(
    const std::span<const RegionRun> regionRuns,
    const ParamSize totalElementCount,
    const ParamSize blockElementOffset,
    const ParamSize blockElementCount,
    const NumericArrayRegionControlParams& control,
    LayeredResidualRegionPlan& plan,
    std::string* error = nullptr) {
    plan = {};
    if (!ValidateRegionControlForEncode(control, error)) {
        return false;
    }
    if (!control.regions.empty() && regionRuns.empty()) {
        return validation::AssignError(error, "region precision control requires region runs");
    }
    if (!ValidateRegionRunsForEncode(regionRuns, totalElementCount, control.regions.size(), error)) {
        return false;
    }

    std::vector<RegionPrecisionLevel> levels;
    std::vector<std::size_t> labelToLevel;
    if (!BuildPrecisionLevels(control, levels, labelToLevel, error)) {
        return false;
    }
    if (levels.empty()) {
        return validation::AssignError(error, "region precision control did not produce any precision level");
    }
    plan.backgroundCompressor = levels.front().compressor;

    ParamSize totalRefinedElementCount = 0u;
    for (std::size_t precisionLevel = 1u; precisionLevel < levels.size(); ++precisionLevel) {
        RegionPlan layer;
        if (!BuildPrecisionLayerPlanFromRegionRuns(
                regionRuns,
                std::span<const std::size_t>(labelToLevel.data(), labelToLevel.size()),
                precisionLevel,
                blockElementOffset,
                blockElementCount,
                control.runPolicy,
                layer,
                error)) {
            return false;
        }
        if (layer.originalElementCount == 0u) {
            continue;
        }
        layer.refineCompressor = levels[precisionLevel].compressor;
        if (layer.refinedElementCount >
            std::numeric_limits<ParamSize>::max() - totalRefinedElementCount) {
            return validation::AssignError(error, "precision layer refined element count overflow");
        }
        totalRefinedElementCount += layer.refinedElementCount;
        const double refinedRatio = blockElementCount == 0u
            ? 0.0
            : static_cast<double>(totalRefinedElementCount) / static_cast<double>(blockElementCount);
        if (refinedRatio > control.runPolicy.maxRefinedElementRatio) {
            return validation::AssignError(error, "precision layer refined element ratio exceeds the configured limit");
        }
        plan.layers.push_back(std::move(layer));
    }
    return true;
}

inline NumericArrayRegionLayerLayoutParams MakeRegionLayerLayoutFromPlan(const RegionPlan& plan) {
    NumericArrayRegionLayerLayoutParams layout;
    layout.regionId = plan.regionId;
    layout.originalElementCount = plan.originalElementCount;
    layout.refinedElementCount = plan.refinedElementCount;
    layout.expandedBackgroundCount = plan.expandedBackgroundCount;
    layout.refineCompressor = plan.refineCompressor;
    layout.residualBytesCodec = NumericArrayBytesCodec::NumericArrayCodec;
    layout.runs = plan.runs;
    return layout;
}

} // namespace datacodec::numericarray

#endif
