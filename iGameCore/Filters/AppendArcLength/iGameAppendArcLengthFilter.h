#ifndef iGameAppendArcLengthFilter_h
#define iGameAppendArcLengthFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @class AppendArcLengthFilter
 * @brief 为折线（一维线单元）各点累计弧长，语义对齐 VTK `vtkAppendArcLength`。
 *
 * 与 `vtkAppendArcLength::RequestData` 逐项对齐的语义：
 *  - 输出独立模型，保留输入几何及属性，并追加点标量 "arc_length"（维数 1），元素数 = 输入点数，初值 0；
 *  - 只遍历线 / 折线单元（IG_LINE、IG_POLY_LINE，对应 `vtkPolyData::GetLines()`）：
 *    沿单元的顶点顺序累加相邻点的距离，折线第 k 个顶点写入"从起点到该顶点的累计弧长"，
 *    折线首顶点保持原值（通常为 0）；
 *  - 其它单元（顶点 / 三角形 / 四边形 / 多边形 / 体单元）不参与，其顶点保持 0；
 *  - 数组类型：VTK 按输入点精度选择 double / float，iGameVis 的 Points 固定为 float32，
 *    因此输出 FloatArray；
 *  - 同名属性按"覆盖"语义处理（等价 `vtkFieldData::AddArray`），重复执行不会堆积同名数组；
 *  - 与 VTK 相同的限制：假设单元之间不共享点；若共享，后处理的折线会覆盖前一条的结果。
 *
 * 折线来源（对应 `vtkPolyData::GetLines()`）：
 *  1) UnstructuredMesh 的显式线单元：单元类型为 IG_LINE / IG_POLY_LINE 的单元；
 *  2) SurfaceMesh 的边数组：legacy VTK POLYDATA 的 LINES 段会被读入 `SurfaceMesh::SetEdges()`。
 *     `BuildEdges()`（渲染线框、若干过滤器会调用）会用"面派生边"覆盖该数组，
 *     因此只在 `AreEdgesDerivedFromFaces() == false`（边不是面派生）时才把边当作折线源，
 *     避免把面网格的派生边误当成折线。
 *     注意：一旦在对含 LINES 的文件执行过 `BuildEdges()`，文件里的折线已被派生边覆盖，无法再还原。
 *
 * 其它输入（VolumeMesh / StructuredMesh / 纯点集、以及只有面单元的表面网格）没有折线单元，
 * 此时输出的 arc_length 全为 0，与 VTK 对不含 lines 的 vtkPolyData 的行为一致。
 */
class AppendArcLengthFilter : public Filter {
public:
    I_OBJECT(AppendArcLengthFilter);
    static Pointer New() { return new AppendArcLengthFilter; }

    bool Execute() override;

    std::string GetMessage() const { return m_Message; }

    /// 输出属性名，固定为 "arc_length"（与 vtkAppendArcLength 一致）
    static const char* GetArrayName() { return "arc_length"; }

    /// 本次执行实际处理的折线单元数（面板提示 / 诊断用）
    IGsize GetNumberOfProcessedLines() const noexcept { return m_LineCount; }

protected:
    AppendArcLengthFilter();
    ~AppendArcLengthFilter() override = default;

    std::string m_Message;
    IGsize m_LineCount{0};
};

IGAME_NAMESPACE_END
#endif
