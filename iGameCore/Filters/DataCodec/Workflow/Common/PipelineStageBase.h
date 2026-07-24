#ifndef DATACODEC_WORKFLOW_COMMON_PIPELINESTAGEBASE_H
#define DATACODEC_WORKFLOW_COMMON_PIPELINESTAGEBASE_H

#include <compare>
#include <cstddef>
#include <string>
#include <string_view>
namespace datacodec {

// Stage 的唯一标识和元数据
struct StageId {
    // 可读的 stage 类型名
    std::string_view name{};
    // 多次实例化 stage 的 repeat 索引
    std::size_t index{0};
    // 字符串化时是否需要带上 repeat 索引
    bool indexedName{false};

    auto operator<=>(const StageId& other) const {
        if (const auto nameCompare = name <=> other.name; nameCompare != 0) {
            return nameCompare;
        }
        return index <=> other.index;
    }

    bool operator==(const StageId& other) const { return name == other.name && index == other.index; }

    [[nodiscard]] std::string ToString() const {
        if (!indexedName) {
            return std::string(name);
        }
        return std::string(name) + "[" + std::to_string(index) + "]";
    }
};

inline StageId MakeStageId(
    const std::string_view name,
    const std::size_t index = 0,
    const bool indexedName = false) {
    return {name, index, indexedName};
}

// 前置声明
struct EncodeContext;
struct DecodeContext;
class EncodeLeafWorkspace;
struct DecodeLeafWorkspace;

// 统一的 Stage 接口基类（仅包含元数据）
struct IStage {
    virtual ~IStage() = default;

    // 供 pipeline 调度和追踪使用的稳定 stage 元数据
    virtual const char* Name() const = 0;
    virtual std::size_t StageIndex() const { return 0; }
    virtual bool UsesIndexedName() const { return false; }

    [[nodiscard]] StageId Id() const { return MakeStageId(Name(), StageIndex(), UsesIndexedName()); }
    [[nodiscard]] std::string Describe() const { return Id().ToString(); }
};

// encode 和 decode 共用同一套 stage id 表达
using EncodeStageId = StageId;
using DecodeStageId = StageId;

enum class EncodeStageExecutionStatus {
    Completed,
    Failed,
    EmptyInput
};

inline const char* EncodeStageExecutionStatusName(
    const EncodeStageExecutionStatus status) noexcept {
    switch (status) {
        case EncodeStageExecutionStatus::Completed:
            return "Completed";
        case EncodeStageExecutionStatus::EmptyInput:
            return "EmptyInput";
        case EncodeStageExecutionStatus::Failed:
        default:
            return "Failed";
    }
}

struct EncodeStageExecutionRecord {
    EncodeStageId stageId;
    EncodeStageExecutionStatus status{EncodeStageExecutionStatus::Failed};
};

inline EncodeStageId MakeEncodeStageId(
    const std::string_view name,
    const std::size_t index = 0,
    const bool indexedName = false) {
    return MakeStageId(name, index, indexedName);
}

inline DecodeStageId MakeDecodeStageId(
    const std::string_view name,
    const std::size_t index = 0,
    const bool indexedName = false) {
    return MakeStageId(name, index, indexedName);
}

// Encode Stage 接口（包含 EncodeContext 特化的 Execute）
struct EncodeStage : IStage {
    ~EncodeStage() override = default;
    virtual void PrepareOutputs(EncodeContext&, EncodeLeafWorkspace&) {}
    virtual EncodeStageExecutionStatus Execute(
        EncodeContext& context,
        EncodeLeafWorkspace& graph) = 0;
};

// Decode Stage 接口（包含 DecodeContext 特化的 Execute）
struct DecodeStage : IStage {
    ~DecodeStage() override = default;
    [[nodiscard]] virtual bool UsesInternalParallelism() const noexcept { return false; }
    virtual void Execute(DecodeContext& context, DecodeLeafWorkspace& workspace) = 0;
};

} // namespace datacodec

#endif
