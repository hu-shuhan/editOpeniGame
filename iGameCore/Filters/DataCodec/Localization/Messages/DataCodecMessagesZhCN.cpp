#include "DataCodec/Localization/DataCodecMessageCatalog.h"

namespace datacodec::localizationdetail {

std::string_view DataCodecSimplifiedChineseMessageTemplate(const DataCodecMessageId id) noexcept {
    switch (id) {
        case DataCodecMessageId::EncodePreparing: return "压缩准备中";
        case DataCodecMessageId::EncodeSorting: return "排序";
        case DataCodecMessageId::EncodeTopology: return "拓扑压缩";
        case DataCodecMessageId::EncodeGeometry: return "坐标压缩";
        case DataCodecMessageId::EncodeAttribute: return "属性数据压缩";
        case DataCodecMessageId::EncodeAttributeNamed: return "属性数据压缩：{name}";
        case DataCodecMessageId::EncodeAttributeUnnamed: return "属性数据压缩：未命名属性 {index}";
        case DataCodecMessageId::EncodeSingleBlock: return "单块数据压缩";
        case DataCodecMessageId::EncodeBlock: return "多块数据压缩 {index}/{count}";
        case DataCodecMessageId::EncodePackageCompress: return "打包压缩";
        case DataCodecMessageId::EncodePackageWrite: return "打包写入";
        case DataCodecMessageId::EncodeWriteFile: return "写入文件";
        case DataCodecMessageId::EncodeFinalizeResult: return "整理编码结果";
        case DataCodecMessageId::EncodeFinalizeFile: return "完成文件";
        case DataCodecMessageId::EncodeCompleted: return "压缩完成";
        case DataCodecMessageId::EncodeWarning: return "压缩警告";
        case DataCodecMessageId::EncodeFailed: return "压缩失败";
        case DataCodecMessageId::DecodeStarted: return "开始解压";
        case DataCodecMessageId::DecodeParams: return "解码参数";
        case DataCodecMessageId::DecodeGeometry: return "解码坐标";
        case DataCodecMessageId::DecodeTopology: return "解码拓扑";
        case DataCodecMessageId::DecodeAttribute: return "解码数值";
        case DataCodecMessageId::DecodeCommit: return "提交结果";
        case DataCodecMessageId::DecodeInProgress: return "解码中";
        case DataCodecMessageId::DecodeCompleted: return "解压完成";
        case DataCodecMessageId::DecodeWarning: return "解压警告";
        case DataCodecMessageId::DecodeFailed: return "解压失败";
        case DataCodecMessageId::DecodeSingleBlock: return "单块解码";
        case DataCodecMessageId::DecodeBlock: return "数据块解码 {index}/{count}";
        case DataCodecMessageId::DecodeValidateCommit: return "校验提交结果";
        case DataCodecMessageId::DecodeCommitGeometry: return "提交坐标";
        case DataCodecMessageId::DecodeCommitTopology: return "提交拓扑";
        case DataCodecMessageId::DecodeCommitAttribute: return "提交数值";
        case DataCodecMessageId::DecodeFinalizeResult: return "整理结果";
        case DataCodecMessageId::AttributeRequestCompleted: return "属性请求已完成";
        case DataCodecMessageId::AttributeProcessingStarted: return "开始处理属性";
        case DataCodecMessageId::AttributeProcessingCompleted: return "属性处理完成";
        case DataCodecMessageId::PrepareReferenceFrame: return "准备引用帧 {index}/{count}（{frame}）";
        case DataCodecMessageId::DecodeTargetFrame: return "解码目标帧（{frame}）";
        case DataCodecMessageId::CacheHit: return "缓存命中";
        case DataCodecMessageId::PackageDecodeStarted: return "DataCodec 包解压开始";
        case DataCodecMessageId::PackageDecodeCompleted: return "DataCodec 包解压完成";
        case DataCodecMessageId::PackageDecodeFailed: return "DataCodec 包解压失败";
        case DataCodecMessageId::FrameCounter: return "第 {index}/{count} 帧";
        case DataCodecMessageId::None:
        default: return {};
    }
}

} // namespace datacodec::localizationdetail
