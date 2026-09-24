#ifndef iGameCountCellVerticesFilter_h
#define iGameCountCellVerticesFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @class CountCellVerticesFilter
 * @brief 统计每个单元的顶点数，结果写入独立输出网格的 Cell Data：cell_vertex_count。
 *
 * 【输出约定：完全独立的输出节点】
 *   本 Filter 不修改输入数据，而是深拷贝出一个全新的 UnstructuredMesh 作为输出：
 *   - 点、单元连接表、单元类型表全部深拷贝（指针级独立，不共享输入内存）；
 *   - 属性集为新建的 AttributeSet，输入的全部属性数组（含整数等各类型）逐个深拷贝；
 *   - 统计结果作为新的 Cell Data 数组 cell_vertex_count 写入输出网格。
 *   因此输入网格绝对不受影响，输出可作为模型树中的独立节点查看、着色。
 *
 * 【空的模型处理】
 *   输入没有任何单元时仍然算"执行成功"，产出一个长度为 0 的 cell_vertex_count 数组，
 *   保证"执行成功 ⇒ 数组一定存在"，界面不会出现"找不到数组"的矛盾状态。
 *
 * 【重复执行】
 *   深拷贝属性时跳过旧的 cell_vertex_count，重复执行不会累积同名数组。
 *
 * 【异常安全】
 *   Execute() 捕获异常并清空输出，失败返回 false 且不残留旧结果。
 */
class CountCellVerticesFilter : public Filter {

public:
    I_OBJECT(CountCellVerticesFilter);
    static Pointer New() { return new CountCellVerticesFilter; }
    bool Execute() override;

    /// 最近一次执行的信息（失败原因、空网格提示等），供界面显示
    const std::string& GetMessage() const { return m_Message; }

protected:
    CountCellVerticesFilter();
    ~CountCellVerticesFilter() override = default;

    /// 执行主体逻辑（不含异常捕获，由 Execute 包裹）
    bool ExecuteInternal();

    /// 最近一次执行的信息
    std::string m_Message;
};

IGAME_NAMESPACE_END
#endif // iGameCountCellVerticesFilter_h
