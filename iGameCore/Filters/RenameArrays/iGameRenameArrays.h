#pragma once
#ifndef iGameRenameArrays_h
#define iGameRenameArrays_h

#include "iGameFilter.h"
#include "iGameType.h"

#include <map>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @brief 重命名数组（对标 ParaView 的 Rename Arrays，VTK 侧是 vtkArrayRename）
 *
 * 行为与 ParaView 一致：
 *   - 输出是一个**全新的网格**（与输入同类型，几何 / 拓扑 / 属性都独立），
 *     输入对象不被修改；
 *   - 只改数组的**名字**，不碰几何与数据值；
 *   - 新名为空 → 拒绝（与 vtkArrayRename 一致）；
 *   - 新名与同组已有数组重名 → 拒绝该条并把冲突写进 GetMessage()
 *     （VTK 是「警告后覆盖」；这里选择不产生重名数组，避免 GetAttribute(name) 歧义）；
 *   - 复合 / 多块数据集暂不支持（VTK 由 executive 逐块展开，本实现直接失败并给出消息）。
 *
 * 只处理两种关联：IG_POINT（点数据）与 IG_CELL（单元数据）——本框架的 AttributeSet
 * 只有这两种附着（枚举里的 IG_EDGE 等是选择事件语义，不是属性附着），
 * 也没有 vertex 关联；字段数据（DataObject::Metadata）不属于 AttributeSet，本 filter 不涉及
 * （因此输出不带字段数据）。
 *
 * 配置既可以按「旧名 → 新名」，也可以按输入数组下标：下标在 Execute() 时按输入解析，
 * 因此允许「先配置、后 SetInput」的调用顺序（vtkArrayRename 的按下标重载要求先 SetInputData）。
 *
 * 典型用法：
 * @code
 *   auto filter = iGame::RenameArrays::New();
 *   filter->SetInput(mesh);
 *   filter->SetPointArrayName("value", "pressure");        // 按名字
 *   filter->SetCellArrayName("cid", "cell_id");
 *   filter->SetArrayName(IG_POINT, 1, "temperature");      // 按下标（Execute 时解析）
 *   filter->Execute();
 *   auto out = filter->GetOutput(0);                       // 全新网格，输入不变
 * @endcode
 */
class RenameArrays : public Filter {
public:
    I_OBJECT(RenameArrays);
    static Pointer New() { return new RenameArrays; }

    bool Execute() override;

    /* ---------------- 配置（与 vtkArrayRename 对齐） ---------------- */

    /** 按名字配置：attachmentType ∈ {IG_POINT, IG_CELL} */
    void SetArrayName(IGenum attachmentType, const std::string& inputName, const std::string& newName);
    /** 按输入数组下标配置（下标在 Execute() 时按输入解析成旧名） */
    void SetArrayName(IGenum attachmentType, IGsize idx, const std::string& newName);

    void SetPointArrayName(const std::string& inputName, const std::string& newName) {
        SetArrayName(IG_POINT, inputName, newName);
    }
    void SetCellArrayName(const std::string& inputName, const std::string& newName) {
        SetArrayName(IG_CELL, inputName, newName);
    }

    /** 清空全部映射 */
    void ClearAll();
    /** 清空某一关联分组的映射 */
    void ClearMapping(IGenum attachmentType);
    /** 已配置的映射条数（含按下标待解析的） */
    int GetNumberOfMappings() const;

    /* ---------------- 供面板填表（都从输入读，与 vtkArrayRename 一致） ---------------- */

    /** 输入中该关联分组下的数组个数 */
    int GetNumberOfArrays(IGenum attachmentType);
    /** 输入中该关联分组第 idx 个数组的原始名字；越界返回空串 */
    std::string GetArrayOriginalName(IGenum attachmentType, IGsize idx);
    /** 输入中该关联分组第 idx 个数组被配置的新名；未配置返回空串 */
    std::string GetArrayNewName(IGenum attachmentType, IGsize idx);

    std::string GetMessage() const { return m_Message; }

protected:
    RenameArrays() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~RenameArrays() override = default;

private:
    /** 按下标配置、待 Execute 时按输入解析的条目 */
    struct IndexRename {
        IGenum attachmentType{IG_POINT};
        IGsize index{0};
        std::string newName;
    };

    /** 把 m_PendingByIndex 里的下标解析成名字，写入 m_ArrayMapping */
    void ResolveIndexMappings(DataObject* input);
    /** 输出属性集里是否已存在同组同名数组（用于冲突检测） */
    static bool HasArrayNamed(AttributeSet* attributes, IGenum attachmentType, const std::string& name);

    // attachmentType -> (旧名 -> 新名)
    std::map<IGenum, std::map<std::string, std::string>> m_ArrayMapping;
    std::vector<IndexRename> m_PendingByIndex;

    std::string m_Message{"未执行"};
};

IGAME_NAMESPACE_END
#endif
