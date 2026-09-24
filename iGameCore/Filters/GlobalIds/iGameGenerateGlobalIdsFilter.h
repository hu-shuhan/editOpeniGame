#pragma once

#include "iGameFilter.h"
#include "iGameDataObject.h"

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @brief 实在找不到对应的逻辑进程号模块，因此无法实现自行对前一进程偏移的获取，只能由调用者输入进程偏移
 *
 * 生成的 Point/Cell Global ID 属性使用一维 DoubleArray；可接受的最大精确整数为 2^53。
 * 复合数据中共享同一个 Points 对象的叶子会复用同一段 Point IDs。
 * 过滤器会深拷贝输入数据树并返回独立输出，不修改原模型。
 * ID 数组只存放在输出的叶子对象上；提交后会更新所有输出中间容器的 Modified Time。
 */
class GenerateGlobalIdsFilter : public Filter {
public:
    I_OBJECT(GenerateGlobalIdsFilter);
    static Pointer New() { return new GenerateGlobalIdsFilter; }

    enum class ExistingIdPolicy {
        Error,        // 已存在同名属性则失败
        KeepExisting, // 仅保留与当前 offset/前缀完全一致的连续全局 ID
        Replace       // 覆盖
    };
    bool Execute() override;
    static bool CountEntities(DataObject::Pointer input, IGsize& pointCount, IGsize& cellCount);

    void SetGeneratePointIds(bool value) {
        if (m_GeneratePointIds != value) {
            m_GeneratePointIds = value;
            this->Modified();
        }
    }

    bool GetGeneratePointIds() const { return m_GeneratePointIds; }

    void SetGenerateCellIds(bool value) {
        if (m_GenerateCellIds != value) {
            m_GenerateCellIds = value;
            this->Modified();
        }
    }

    bool GetGenerateCellIds() const { return m_GenerateCellIds; }

    void SetPointOffset(iguIndex64 offset) {
        if (m_PointOffset != offset) {
            m_PointOffset = offset;
            this->Modified();
        }
    }

    iguIndex64 GetPointOffset() const { return m_PointOffset; }

    void SetCellOffset(iguIndex64 offset) {
        if (m_CellOffset != offset) {
            m_CellOffset = offset;
            this->Modified();
        }
    }

    iguIndex64 GetCellOffset() const { return m_CellOffset; }

    void SetOffsets(iguIndex64 pointOffset, iguIndex64 cellOffset) {
        SetPointOffset(pointOffset);
        SetCellOffset(cellOffset);
    }

    void SetPointArrayName(const std::string& name) {
        if (m_PointArrayName != name) {
            m_PointArrayName = name;
            this->Modified();
        }
    }

    const std::string& GetPointArrayName() const { return m_PointArrayName; }

    void SetCellArrayName(const std::string& name) {
        if (m_CellArrayName != name) {
            m_CellArrayName = name;
            this->Modified();
        }
    }

    const std::string& GetCellArrayName() const { return m_CellArrayName; }

    void SetExistingIdPolicy(ExistingIdPolicy policy) {
        if (m_ExistingIdPolicy != policy) {
            m_ExistingIdPolicy = policy;
            this->Modified();
        }
    }

    ExistingIdPolicy GetExistingIdPolicy() const { return m_ExistingIdPolicy; }

    const std::string& GetMessage() const { return m_Message; }

protected:
    GenerateGlobalIdsFilter();
    ~GenerateGlobalIdsFilter() override = default;

private:
    bool m_GeneratePointIds{true};
    bool m_GenerateCellIds{true};

    iguIndex64 m_PointOffset{0};
    iguIndex64 m_CellOffset{0};

    std::string m_PointArrayName{"GlobalPointIds"};
    std::string m_CellArrayName{"GlobalCellIds"};

    ExistingIdPolicy m_ExistingIdPolicy{ExistingIdPolicy::Error};

    std::string m_Message;
};

IGAME_NAMESPACE_END
