#ifndef iGameGroupDatasetsFilter_h
#define iGameGroupDatasetsFilter_h

#include "iGameFilter.h"
#include "iGameDrawObject.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * GroupDatasetsFilter —— 把 N 个数据对象打包成一个多块复合组（MultiBlock）
 *
 * 对齐 ParaView 的 Group Datasets（vtkGroupDataSetsFilter）语义：
 *
 *   1. 只做【容器组合】，**不合并几何** —— 每个输入在输出里对应一个
 *      【同类型深拷贝克隆】：几何（Points/Cells/Faces/Edges/…）、属性表、
 *      显示状态、动画帧全部**各持一份** —— 新旧对象完全独立，改一边不影响另一边；
 *      原对象零修改（父指针、显隐、数据都不被接管），删除组后源对象不受任何影响。
 *      这与 AppendReduceFilter（Append Datasets，真正合并点/单元）是两回事。
 *   2. 允许**混合类型**输入：体网格 / 表面网格 / 点云可以放进同一个多块组。
 *   3. 输入本身若已是多块组，则**嵌套**保留其结构，不做展平（克隆里递归克隆）——
 *      因此输出的【顶层块数】恒等于【去重后的输入对象数】。
 *   4. 输出的每个子块沿用输入的名字（克隆名复制自源名，改克隆名不影响源对象）；
 *      重名子块在 UI 报告中按序号编号区分。
 *   5. 暂不支持的类型（StructuredMesh / Lagrange / Spline / 编码类等）与深拷贝失败的
 *      输入会被**跳过**并记入 GetSkippedNames()，不影响其余输入正常合组。
 *
 * 语义对照（与"直接挂原对象"的旧实现相比）：
 *   - 数据（几何 / 属性表）：完全独立（深拷贝）—— 改组的、改原件的互不可见；
 *   - 显示（显隐 / 样式 / 配色器）：彼此独立 —— 克隆带"源配色器的拷贝"出生，此后互不影响；
 *     隐藏源对象也不影响组（未改动框架原有的伪彩逻辑）。
 *
 * 用法：
 * @code
 *   auto filter = GroupDatasetsFilter::New();
 *   filter->SetInputs(selectedObjects);      // 选中的 DataObject 列表
 *   filter->SetOutputName("GroupDatasets_1");// 由上层算好的唯一名
 *   if (!filter->Execute()) { 用 filter->GetMessage() 提示用户 }
 *   auto group = filter->GetOutput(0);       // 多块组（子块为深拷贝克隆）
 *   // 可选：filter->GetSkippedNames() 列出被跳过的输入名
 * @endcode
 */
class GroupDatasetsFilter : public Filter {
public:
    I_OBJECT(GroupDatasetsFilter);
    static Pointer New() { return new GroupDatasetsFilter; }

    /// 执行：把已设置的输入打包成多块组，写入输出槽 0。
    /// 返回 false 时用 GetMessage() 取失败原因（供 UI 弹窗）。
    bool Execute() override;

    /// 一次性设置全部输入（内部会先 SetNumberOfInputs 再逐个 SetInput）。
    /// 传入的 nullptr 会被跳过，不计入输入数；同一个对象重复出现只保留一次
    /// （按对象指针去重，与底层 SubDataObjectsHelper 的"同一对象只留一份"语义一致）。
    void SetInputs(const std::vector<DataObject::Pointer>& objs);

    /// 结果组节点名（由上层决定，通常是 UI 层算出的唯一名）。
    void SetOutputName(const std::string& name) { m_OutputName = name; }
    const std::string& GetOutputName() const { return m_OutputName; }

    /// 失败/提示信息（UTF-8，供 UI 直接显示）。
    const std::string& GetMessage() const { return m_Message; }

    /// 实际合并进组的数据对象个数。
    int GetGroupedCount() const { return m_GroupedCount; }

    /// 各子块的名字（按加入顺序，取自输入对象的 GetName()，可能重复）。
    const std::vector<std::string>& GetBlockNames() const { return m_BlockNames; }

    /// 执行时被跳过的输入名字（类型暂不支持 / 嵌套超深），按遇到顺序。
    /// 部分跳过时 Execute() 仍返回 true，可用本接口向用户展示明细。
    const std::vector<std::string>& GetSkippedNames() const { return m_SkippedNames; }

protected:
    GroupDatasetsFilter();
    ~GroupDatasetsFilter() override = default;

private:
    std::string m_OutputName;
    std::string m_Message;
    int m_GroupedCount{0};
    std::vector<std::string> m_BlockNames;
    std::vector<std::string> m_SkippedNames;

    // 注意：输入数组沿用基类 Filter::m_Inputs（ElementArray<DataObject::Pointer>），
    //       不要在这里另建同名成员，否则会遮蔽基类成员。
};

IGAME_NAMESPACE_END
#endif
