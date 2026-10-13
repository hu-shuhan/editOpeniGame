#ifndef iGameExtractCellsByTypeFilter_h
#define iGameExtractCellsByTypeFilter_h

#include "iGameFilter.h"
#include "iGameDataObject.h"
#include "iGameUnstructuredMesh.h"
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

//ExtractCellsByTypeFilter: 根据单元类型提取模型中的单元，组成新的网格（UnstructuredMesh）
//输出只包含被选中类型的单元；点属性只保留被使用到的点，单元属性只保留被选中的单元，属性不丢失
//输出命名 ExtractCellsByType_n，n 为该 filter 实例的全局序号（第几个提取的模型）
class ExtractCellsByTypeFilter : public Filter {
public:
    I_OBJECT(ExtractCellsByTypeFilter);
    static Pointer New() { return new ExtractCellsByTypeFilter; }

    bool Execute() override;

    ///@{ 设置需要提取的单元类型（空集合 = 不提取任何单元）
    void SetExtractCellTypes(const std::vector<IGenum>& types) { m_ExtractTypes = types; }
    void ClearExtractCellTypes() { m_ExtractTypes.clear(); }
    void AddExtractCellType(IGenum type) { m_ExtractTypes.push_back(type); }
    const std::vector<IGenum>& GetExtractCellTypes() const { return m_ExtractTypes; }
    ///@}

    // 扫描输入模型，返回模型中实际存在的单元类型（按出现顺序去重）。
    // 需先 SetInput，再调用本函数；供 UI 面板列出可勾选的单元类型。
    std::vector<IGenum> GetAvailableCellTypes();

    // 第几个提取的模型（用于命名 ExtractCellsByType_n）
    int GetInstanceId() const { return m_InstanceId; }

    // 单元类型的中文+英文显示名（覆盖框架全部单元类型；未知名返回"未知类型"）
    static std::string GetCellTypeDisplayName(IGenum type);

protected:
    ExtractCellsByTypeFilter();
    ~ExtractCellsByTypeFilter() override = default;

    // 把任意网格统一成"单元连接表 + 每单元类型"（适配非结构/表面/体积/结构化网格）
    // @param outCells 输出的单元连接表（SmartPointer，持有引用）
    // @param outTypes outTypes[i] 为 outCells 中第 i 个单元的类型
    bool BuildUnifiedCells(DataObject::Pointer input, CellArray::Pointer& outCells,
                           std::vector<IGenum>& outTypes) const;

private:
    std::vector<IGenum> m_ExtractTypes; // 要提取的单元类型集合
    int m_InstanceId = 0;               // 第几个提取模型（构造函数里从全局计数器取号）
    static int s_InstanceCounter;       // 全局实例计数器
};

IGAME_NAMESPACE_END
#endif
