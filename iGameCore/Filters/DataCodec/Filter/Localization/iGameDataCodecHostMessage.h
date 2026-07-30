#ifndef iGameDataCodeciGameDataCodecHostMessage_h
#define iGameDataCodeciGameDataCodecHostMessage_h

#include "DataCodec/Localization/DataCodecLanguage.h"
#include "iGameMacro.h"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>

IGAME_NAMESPACE_BEGIN

enum class iGameDataCodecHostMessageId : std::uint8_t {
    EncodeRequiresInputObject,
    EncodeRequiresSupportedLeafObject,
    EncodeFailed,
    EncodeEmptyOutput,
    FrameSequenceRequiresMultipleFrames,
    FrameSequenceEncodeFailed,
    FrameSequenceNoOutputFiles,
    FrameSequenceEmptyOutputFiles,
    DecodedOutputUnavailable,
    ResolveFrameSequenceFailed,
    MultiplePathsRequireFramePackages,
    MemoryInputEmpty,
    DecodeFailed,
    CompressionEnhancementEnabled,
    PredictionEncodingRecommendation,
    LoadDataBeforeOutputPath,
    OutputPathSet,
    SelectCompressionData,
    WaitForFeatureComputation,
    MissingRegionFeatures,
    OverlappingRegionsBlockCompression,
    SetOutputPath,
    CompressionNoData,
    CreateOutputDirectoryFailed,
    CreateReportDirectoryFailed,
    CompressionFailedWithDetail,
    CompressionNoOutputFile,
    WriteReportFailed,
    WriteTelemetryFailed,
    CompressionReportBegin,
    SizeBeforeCompression,
    SizeAfterCompression,
    CompressedSizeRatio,
    CompressionTime,
    TelemetryDataPath,
    CompressionReportEnd,
    SequenceOutputCount,
    FirstFrameFile,
    LastFrameFile,
    FeatureComputationNoData,
    FeatureComputationStarted,
    FeatureComputationFailed,
    FeatureComputationCompleted,
    CannotAddCustomRegion,
    CustomRegionRangeUnavailable,
    CustomRegionOverlap,
    AllFieldsLossless,
    AllFieldsHighPrecisionLossy,
    DefaultPrecisionApplied,
    TimeSeriesCustomRegionUnsupported,
    SelectCurrentField,
    DisableLosslessCompression,
    WaitUntilFeatureComputationCompleted,
    ComputeCurrentFeature,
    CustomRegionLimitReached,
    CustomRegionStateUnavailable,
};

struct iGameDataCodecHostMessageArgument {
    std::string name;
    std::string value;
};

[[nodiscard]] std::string iGameDataCodecHostMessage(
    ::datacodec::DataCodecLanguage language,
    iGameDataCodecHostMessageId id,
    std::initializer_list<iGameDataCodecHostMessageArgument> arguments = {});

IGAME_NAMESPACE_END

#endif
