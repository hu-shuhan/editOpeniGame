#ifndef DATACODEC_API_PARAMS_REFERENCECONTROLPARAMS_H
#define DATACODEC_API_PARAMS_REFERENCECONTROLPARAMS_H

#include <cstddef>
#include <cstdint>
namespace datacodec {

enum class TemporalPredictorSearchStrategy : std::uint8_t {
    ExhaustiveL2 = 0,
    ExhaustiveEstimatedBytes = 1,
    CoarseToFineL2 = 2,
    CoarseToFineEstimatedBytes = 3,
};

enum class IntraFieldReferenceCodec : std::uint8_t {
    Disabled = 0,
    Wavelet = 1,
    Affine = 2,
    Predictor = 3,
};

enum class ReferenceSelectionMode : std::uint8_t {
    Auto = 0,
    Forced = 1,
};

enum class ReferenceAutoSelectionStrategy : std::uint8_t {
    Exact = 0,
    BoundedProbe = 1,
};

enum class TemporalFieldReferenceCodec : std::uint8_t {
    Disabled = 0,
    Wavelet = 1,
    Predictor = 2,
};

struct IntraFieldAffineControlParams {
    // 帧内父场采样粗筛阈值
    double precheckRSquared{0.92};
    // 帧内块级 affine 拟合阈值
    double blockRSquared{0.95};
};

struct IntraFieldReferenceControlParams {
    // 帧内引用默认使用 affine，调用方仍可通过参数选择其他实现
    IntraFieldReferenceCodec codec{IntraFieldReferenceCodec::Affine};
    // Auto 按空间块比较 Ordinary 与 Reference
    // Forced 要求每个空间块使用指定 Reference Codec
    ReferenceSelectionMode selectionMode{ReferenceSelectionMode::Auto};
    // Exact 完整编码两个候选，BoundedProbe 使用固定规模块内探针
    ReferenceAutoSelectionStrategy autoSelectionStrategy{
        ReferenceAutoSelectionStrategy::Exact};
    // 每个字段只读取一次的帧内父场采样数量
    std::size_t sampleCount{256u};
    // predictor 和 wavelet 的采样粗筛阈值
    double minimumSampleScore{0.5};
    // affine 子策略
    IntraFieldAffineControlParams affine;
};

struct TemporalPredictorControlParams {
    // 局部窗口搜索默认关闭，调用方可按数据特征显式启用
    bool enableLocalWindowSearch{false};
    // 时域 predictor 的局部窗口半径
    std::int32_t windowRadius{8};
    // 时域 predictor 的 offset 搜索策略
    TemporalPredictorSearchStrategy searchStrategy{
        TemporalPredictorSearchStrategy::ExhaustiveL2};
};

struct TemporalFieldReferenceControlParams {
    // 帧间同名 field 默认使用 predictor
    TemporalFieldReferenceCodec codec{TemporalFieldReferenceCodec::Predictor};
    // Auto 按空间块比较 Ordinary 与 Reference
    // Forced 在 Reference 失败时终止编码
    ReferenceSelectionMode selectionMode{ReferenceSelectionMode::Auto};
    // 属性关键帧按固定 GOP 间隔规划，0 表示只有首帧是关键帧
    std::uint32_t keyFrameInterval{8};
    // 调试时可强制首帧后全部按属性预测帧规划
    bool forcePredFrames{false};
    // predictor 子策略
    TemporalPredictorControlParams predictor;
};

struct AttrReferenceControlParams {
    // 关闭后属性帧内 reference 和时域 reference 都不启用
    bool enabled{true};
    // 帧内引用策略
    IntraFieldReferenceControlParams intraField;
    // 时域引用策略
    TemporalFieldReferenceControlParams temporalField;
};

struct GeometryReferenceControlParams {
    // 关闭后几何时域 reference 不启用
    bool enabled{true};
    // 几何关键帧按固定 GOP 间隔规划
    TemporalFieldReferenceControlParams temporalField;
};

struct TopologyReferenceControlParams {
    // 关闭后拓扑指纹复用不启用，每帧都写入自有拓扑
    bool enabled{true};
};

} // namespace datacodec

#endif
