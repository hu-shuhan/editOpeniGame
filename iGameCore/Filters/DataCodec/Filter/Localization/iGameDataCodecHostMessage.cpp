#include "DataCodec/Filter/Localization/iGameDataCodecHostMessage.h"

IGAME_NAMESPACE_BEGIN

namespace {

std::string_view EnglishMessage(const iGameDataCodecHostMessageId id) noexcept {
    switch (id) {
        case iGameDataCodecHostMessageId::EncodeRequiresInputObject:
            return "iGame DataCodec encoding requires an input object";
        case iGameDataCodecHostMessageId::EncodeRequiresSupportedLeafObject:
            return "iGame DataCodec encoding requires a supported leaf object";
        case iGameDataCodecHostMessageId::EncodeFailed:
            return "iGame DataCodec encoding failed";
        case iGameDataCodecHostMessageId::EncodeEmptyOutput:
            return "iGame DataCodec encoding produced an empty output file";
        case iGameDataCodecHostMessageId::FrameSequenceRequiresMultipleFrames:
            return "iGame DataCodec frame sequence encoding requires multiple frames";
        case iGameDataCodecHostMessageId::FrameSequenceEncodeFailed:
            return "iGame DataCodec frame sequence encoding failed";
        case iGameDataCodecHostMessageId::FrameSequenceNoOutputFiles:
            return "iGame DataCodec frame sequence encoding produced no output files";
        case iGameDataCodecHostMessageId::FrameSequenceEmptyOutputFiles:
            return "iGame DataCodec frame sequence encoding produced empty output files";
        case iGameDataCodecHostMessageId::DecodedOutputUnavailable:
            return "The iGame DataCodec decoded output is unavailable";
        case iGameDataCodecHostMessageId::ResolveFrameSequenceFailed:
            return "iGame failed to resolve the selected DataCodec frame sequence";
        case iGameDataCodecHostMessageId::MultiplePathsRequireFramePackages:
            return "Multiple selected paths require DataCodec frame packages";
        case iGameDataCodecHostMessageId::MemoryInputEmpty:
            return "The iGame DataCodec memory input is empty";
        case iGameDataCodecHostMessageId::DecodeFailed:
            return "iGame DataCodec decoding failed";
        case iGameDataCodecHostMessageId::CompressionEnhancementEnabled:
            return "Compression enhancement improves compression ratio and reduces codec speed.";
        case iGameDataCodecHostMessageId::PredictionEncodingRecommendation:
            return "Data prediction is enabled. High-precision lossy compression is recommended to improve residual compressibility";
        case iGameDataCodecHostMessageId::LoadDataBeforeOutputPath:
            return "Load data before setting the output path";
        case iGameDataCodecHostMessageId::OutputPathSet:
            return "Output path set to: {path}";
        case iGameDataCodecHostMessageId::SelectCompressionData:
            return "Select at least one data field";
        case iGameDataCodecHostMessageId::WaitForFeatureComputation:
            return "Cannot start compression: wait for feature computation to finish";
        case iGameDataCodecHostMessageId::MissingRegionFeatures:
            return "Cannot start compression: {count} fields have custom regions without computed features";
        case iGameDataCodecHostMessageId::OverlappingRegionsBlockCompression:
            return "Cannot start compression: adjust {count} overlapping custom regions";
        case iGameDataCodecHostMessageId::SetOutputPath:
            return "Set the output path first";
        case iGameDataCodecHostMessageId::CompressionNoData:
            return "Compression failed: no data is loaded";
        case iGameDataCodecHostMessageId::CreateOutputDirectoryFailed:
            return "Compression failed: cannot create output directory {path}";
        case iGameDataCodecHostMessageId::CreateReportDirectoryFailed:
            return "Compression failed: cannot create report directory {path}";
        case iGameDataCodecHostMessageId::CompressionFailedWithDetail:
            return "Compression failed: {detail}";
        case iGameDataCodecHostMessageId::CompressionNoOutputFile:
            return "Compression failed: no output file was generated";
        case iGameDataCodecHostMessageId::WriteReportFailed:
            return "Compression failed: cannot write report file";
        case iGameDataCodecHostMessageId::WriteTelemetryFailed:
            return "Compression failed: cannot write telemetry data";
        case iGameDataCodecHostMessageId::CompressionReportBegin:
            return "================ Compression report begin ================";
        case iGameDataCodecHostMessageId::SizeBeforeCompression:
            return "Size before compression: {size}";
        case iGameDataCodecHostMessageId::SizeAfterCompression:
            return "Size after compression: {size}";
        case iGameDataCodecHostMessageId::CompressedSizeRatio:
            return "Compressed size ratio: {ratio}";
        case iGameDataCodecHostMessageId::CompressionTime:
            return "Compression time: {milliseconds} ms";
        case iGameDataCodecHostMessageId::TelemetryDataPath:
            return "Telemetry data: {path}";
        case iGameDataCodecHostMessageId::CompressionReportEnd:
            return "================ Compression report end ================";
        case iGameDataCodecHostMessageId::SequenceOutputCount:
            return "Sequence output: {count} frames";
        case iGameDataCodecHostMessageId::FirstFrameFile:
            return "First frame file: {path}";
        case iGameDataCodecHostMessageId::LastFrameFile:
            return "Last frame file: {path}";
        case iGameDataCodecHostMessageId::FeatureComputationNoData:
            return "Feature computation failed: no data is loaded";
        case iGameDataCodecHostMessageId::FeatureComputationStarted:
            return "Computing features: {field} / {basis}, {count} items";
        case iGameDataCodecHostMessageId::FeatureComputationFailed:
            return "Feature computation failed: {detail}";
        case iGameDataCodecHostMessageId::FeatureComputationCompleted:
            return "Feature computation completed";
        case iGameDataCodecHostMessageId::CannotAddCustomRegion:
            return "Cannot add custom region: {reason}";
        case iGameDataCodecHostMessageId::CustomRegionRangeUnavailable:
            return "Cannot add custom region: existing regions cover the entire feature range";
        case iGameDataCodecHostMessageId::CustomRegionOverlap:
            return "Current region overlaps existing regions: {count} items";
        case iGameDataCodecHostMessageId::AllFieldsLossless:
            return "All data fields were set to lossless compression";
        case iGameDataCodecHostMessageId::AllFieldsHighPrecisionLossy:
            return "All data fields were set to high-precision lossy compression with relative error bound 0.00001";
        case iGameDataCodecHostMessageId::DefaultPrecisionApplied:
            return "The current default error bound was applied to all data fields";
        case iGameDataCodecHostMessageId::TimeSeriesCustomRegionUnsupported:
            return "Custom regions are not supported for time-series data";
        case iGameDataCodecHostMessageId::SelectCurrentField:
            return "Select the current data field first";
        case iGameDataCodecHostMessageId::DisableLosslessCompression:
            return "Disable lossless compression first";
        case iGameDataCodecHostMessageId::WaitUntilFeatureComputationCompleted:
            return "Wait for feature computation to finish";
        case iGameDataCodecHostMessageId::ComputeCurrentFeature:
            return "Compute the current feature first";
        case iGameDataCodecHostMessageId::CustomRegionLimitReached:
            return "Each feature supports at most 8 custom regions";
        case iGameDataCodecHostMessageId::CustomRegionStateUnavailable:
            return "The current region state is unavailable";
        case iGameDataCodecHostMessageId::DecodeStageTiming:
            return "Decode stage {stage}: {milliseconds} ms";
        case iGameDataCodecHostMessageId::DecodeTime:
            return "Decompression time: {milliseconds} ms";
        case iGameDataCodecHostMessageId::WriteDecodeTelemetryFailed:
            return "Failed to write decode telemetry data: {detail}";
    }
    return {};
}

std::string_view SimplifiedChineseMessage(
    const iGameDataCodecHostMessageId id) noexcept {
    switch (id) {
        case iGameDataCodecHostMessageId::EncodeRequiresInputObject:
            return "iGame DataCodec 编码需要输入对象";
        case iGameDataCodecHostMessageId::EncodeRequiresSupportedLeafObject:
            return "iGame DataCodec 编码需要受支持的叶节点对象";
        case iGameDataCodecHostMessageId::EncodeFailed:
            return "iGame DataCodec 编码失败";
        case iGameDataCodecHostMessageId::EncodeEmptyOutput:
            return "iGame DataCodec 编码生成了空输出文件";
        case iGameDataCodecHostMessageId::FrameSequenceRequiresMultipleFrames:
            return "iGame DataCodec 帧序列编码至少需要两帧";
        case iGameDataCodecHostMessageId::FrameSequenceEncodeFailed:
            return "iGame DataCodec 帧序列编码失败";
        case iGameDataCodecHostMessageId::FrameSequenceNoOutputFiles:
            return "iGame DataCodec 帧序列编码未生成输出文件";
        case iGameDataCodecHostMessageId::FrameSequenceEmptyOutputFiles:
            return "iGame DataCodec 帧序列编码生成了空输出文件";
        case iGameDataCodecHostMessageId::DecodedOutputUnavailable:
            return "iGame DataCodec 解码结果不可用";
        case iGameDataCodecHostMessageId::ResolveFrameSequenceFailed:
            return "iGame 无法解析选中的 DataCodec 帧序列";
        case iGameDataCodecHostMessageId::MultiplePathsRequireFramePackages:
            return "选择多个路径时需要使用 DataCodec 帧包";
        case iGameDataCodecHostMessageId::MemoryInputEmpty:
            return "iGame DataCodec 内存输入为空";
        case iGameDataCodecHostMessageId::DecodeFailed:
            return "iGame DataCodec 解码失败";
        case iGameDataCodecHostMessageId::CompressionEnhancementEnabled:
            return "启用压缩率增强功能后，数据压缩率将得到提升，编解码速度将有所下降。";
        case iGameDataCodecHostMessageId::PredictionEncodingRecommendation:
            return "已启用数据预测编码。建议使用高精度有损压缩，以提高预测残差的可压缩性";
        case iGameDataCodecHostMessageId::LoadDataBeforeOutputPath:
            return "请先加载数据，再设置输出路径";
        case iGameDataCodecHostMessageId::OutputPathSet:
            return "输出路径已设置为：{path}";
        case iGameDataCodecHostMessageId::SelectCompressionData:
            return "请至少选择一项数据";
        case iGameDataCodecHostMessageId::WaitForFeatureComputation:
            return "无法开始压缩：请等待特征计算完成";
        case iGameDataCodecHostMessageId::MissingRegionFeatures:
            return "无法开始压缩：{count} 项数据的自定义区域缺少已计算特征";
        case iGameDataCodecHostMessageId::OverlappingRegionsBlockCompression:
            return "无法开始压缩：存在 {count} 个重叠自定义区域，请先调整";
        case iGameDataCodecHostMessageId::SetOutputPath:
            return "请先设置输出路径";
        case iGameDataCodecHostMessageId::CompressionNoData:
            return "压缩失败：未载入数据";
        case iGameDataCodecHostMessageId::CreateOutputDirectoryFailed:
            return "压缩失败：无法创建输出目录 {path}";
        case iGameDataCodecHostMessageId::CreateReportDirectoryFailed:
            return "压缩失败：无法创建报告目录 {path}";
        case iGameDataCodecHostMessageId::CompressionFailedWithDetail:
            return "压缩失败：{detail}";
        case iGameDataCodecHostMessageId::CompressionNoOutputFile:
            return "压缩失败：未生成输出文件";
        case iGameDataCodecHostMessageId::WriteReportFailed:
            return "压缩失败：无法写出报告文件";
        case iGameDataCodecHostMessageId::WriteTelemetryFailed:
            return "压缩失败：无法写出检测数据";
        case iGameDataCodecHostMessageId::CompressionReportBegin:
            return "================ 压缩报告开始 ================";
        case iGameDataCodecHostMessageId::SizeBeforeCompression:
            return "压缩前文件大小：{size}";
        case iGameDataCodecHostMessageId::SizeAfterCompression:
            return "压缩后文件大小：{size}";
        case iGameDataCodecHostMessageId::CompressedSizeRatio:
            return "压缩后大小占比：{ratio}";
        case iGameDataCodecHostMessageId::CompressionTime:
            return "压缩时间：{milliseconds} ms";
        case iGameDataCodecHostMessageId::TelemetryDataPath:
            return "检测数据：{path}";
        case iGameDataCodecHostMessageId::CompressionReportEnd:
            return "================ 压缩报告结束 ================";
        case iGameDataCodecHostMessageId::SequenceOutputCount:
            return "序列输出：{count} 帧";
        case iGameDataCodecHostMessageId::FirstFrameFile:
            return "首帧文件：{path}";
        case iGameDataCodecHostMessageId::LastFrameFile:
            return "末帧文件：{path}";
        case iGameDataCodecHostMessageId::FeatureComputationNoData:
            return "特征计算失败：未载入数据";
        case iGameDataCodecHostMessageId::FeatureComputationStarted:
            return "计算特征：{field} / {basis}，{count} 个数据项";
        case iGameDataCodecHostMessageId::FeatureComputationFailed:
            return "特征计算失败：{detail}";
        case iGameDataCodecHostMessageId::FeatureComputationCompleted:
            return "特征计算完成";
        case iGameDataCodecHostMessageId::CannotAddCustomRegion:
            return "无法添加自定义区域：{reason}";
        case iGameDataCodecHostMessageId::CustomRegionRangeUnavailable:
            return "无法添加自定义区域：全部特征值范围已被现有自定义区域占用";
        case iGameDataCodecHostMessageId::CustomRegionOverlap:
            return "当前区域与已有区域重叠：{count} 个数据项";
        case iGameDataCodecHostMessageId::AllFieldsLossless:
            return "已将全部数据设为无损压缩";
        case iGameDataCodecHostMessageId::AllFieldsHighPrecisionLossy:
            return "已将全部数据设为高精度有损压缩：相对误差限 0.00001";
        case iGameDataCodecHostMessageId::DefaultPrecisionApplied:
            return "已将当前默认误差限应用到全部数据";
        case iGameDataCodecHostMessageId::TimeSeriesCustomRegionUnsupported:
            return "时序数据暂不支持自定义区域";
        case iGameDataCodecHostMessageId::SelectCurrentField:
            return "请先选择当前数据";
        case iGameDataCodecHostMessageId::DisableLosslessCompression:
            return "请先取消无损压缩";
        case iGameDataCodecHostMessageId::WaitUntilFeatureComputationCompleted:
            return "请等待特征计算完成";
        case iGameDataCodecHostMessageId::ComputeCurrentFeature:
            return "请先计算当前特征";
        case iGameDataCodecHostMessageId::CustomRegionLimitReached:
            return "每个特征最多支持 8 个自定义区域";
        case iGameDataCodecHostMessageId::CustomRegionStateUnavailable:
            return "当前区域状态不可用";
        case iGameDataCodecHostMessageId::DecodeStageTiming:
            return "解码阶段 {stage}：{milliseconds} ms";
        case iGameDataCodecHostMessageId::DecodeTime:
            return "解压时间：{milliseconds} ms";
        case iGameDataCodecHostMessageId::WriteDecodeTelemetryFailed:
            return "无法写出解码检测数据：{detail}";
    }
    return {};
}

} // 匿名命名空间

std::string iGameDataCodecHostMessage(
    const ::datacodec::DataCodecLanguage language,
    const iGameDataCodecHostMessageId id,
    const std::initializer_list<iGameDataCodecHostMessageArgument> arguments) {
    std::string result(language == ::datacodec::DataCodecLanguage::SimplifiedChinese
        ? SimplifiedChineseMessage(id)
        : EnglishMessage(id));
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

IGAME_NAMESPACE_END
