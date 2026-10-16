#ifndef iGameExtractEdgesFilter_h
#define iGameExtractEdgesFilter_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

#include <map>      // 跳过单元的类型统计
#include <set>      // 边去重用的红黑树集合
#include <string>
#include <utility>  // std::pair / std::minmax

IGAME_NAMESPACE_BEGIN

/**
 * @class ExtractEdgesFilter
 * @brief 提取网格的全部唯一边（去重），输出一个只含 IG_LINE 单元的独立 UnstructuredMesh。
 *
 * 【输出约定：独立结果节点】
 *   - 点坐标（深拷贝）与单元连接表都是独立的，不共享输入内存；
 *     每个输出单元都是 2 点线段（IG_LINE）；
 *   - Point Data 深拷贝保留（点数不变，语义仍然成立）；
 *   - Cell Data **按"每条边的来源单元"重映射**（输入单元数 N ≠ 输出边数 M，
 *     不能原样沿用、也不能丢弃）：输出第 i 条边取它来源单元的对应值，
 *     共享边取来源单元 ID 较小者（与 vtkExtractEdges 的 minimum cell id 规则一致），
 *     从而保证输出属性长度与边数一致。
 *     不额外生成"边→来源单元"的辅助数组。
 *   - 另外固定生成一个 `CellType` 数组（Cell Data，长度 = 边数）：每条边的 **VTK 单元类型编号**，
 *     提取结果全是 1 维线单元，故值恒为 `vtkLine = 3`（注意不是 iGame 内部的 IG_LINE = 2）。
 *     语义与 ParaView 中给数据集添加 "Cell Types" 数组（取自 GetCellTypesArray()）一致，
 *     便于在属性面板里以数组形式查看/筛选每条边的单元类型。
 *   - 输入中已有的 IG_LINE 会被保留，IG_POLY_LINE 会逐段拆成多条线段。
 *
 * 【失败与空结果的区分】
 *   - 没有输入 / 不支持的数据类型 / 0 个单元：返回 false（并给出 GetMessage()）。
 *     绝不会把原模型当作结果返回。
 *   - 输入有效但没有任何边可提取（例如单元全是不支持的类型）：返回 true，
 *     输出是 0 条边的空线网格（不是原模型），界面据此不创建结果节点。
 *
 * 【不支持的单元】
 *   被跳过的单元数量与类型分布会记录在 GetSkippedCellCount() / GetSkippedCellTypes() 中，
 *   并写入 GetMessage()，供界面显示"跳过了多少个、分别是什么类型"。
 */
class ExtractEdgesFilter : public Filter {

public:
    I_OBJECT(ExtractEdgesFilter);
    static Pointer New() { return new ExtractEdgesFilter; }

    bool Execute() override;

    UnstructuredMesh::Pointer GetEdgesMesh() {
        return DynamicCast<UnstructuredMesh>(this->GetOutput());
    }

    /// 最近一次执行的信息（失败原因、跳过统计等），供界面显示
    const std::string& GetMessage() const { return m_Message; }

    /// 被跳过的单元数量（不支持的类型 / 不是边的单元）
    IGsize GetSkippedCellCount() const { return m_SkippedCellCount; }

    /// 被跳过的单元类型分布：单元类型 → 数量
    const std::map<IGenum, IGsize>& GetSkippedCellTypes() const { return m_SkippedCellTypes; }

protected:
    ExtractEdgesFilter();
    ~ExtractEdgesFilter() override = default;

    bool ExecuteWithPointSet(DataObject::Pointer input);

    /// 从 input 提取边并填充 output（含按来源单元重映射的 Cell Data），返回是否成功
    bool ExtractEdgesFromMesh(UnstructuredMesh::Pointer input,
                              UnstructuredMesh::Pointer output);

    /// 记录一个被跳过的单元（用于统计与界面提示）
    void RecordSkippedCell(IGenum cellType);

    std::string m_Message;                          // 最近一次执行的信息
    IGsize m_SkippedCellCount{0};                   // 被跳过的单元总数
    std::map<IGenum, IGsize> m_SkippedCellTypes;    // 类型 → 数量
};

IGAME_NAMESPACE_END
#endif
