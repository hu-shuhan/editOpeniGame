#ifndef iGameComputeQuartilesFilter_h
#define iGameComputeQuartilesFilter_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameArrayObject.h"
#include "iGamePointSet.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * ComputeQuartilesFilter — 对标 ParaView 的 Compute Quartiles 滤镜
 * (VTK 类: vtkComputeQuartiles，等价于 vtkComputeQuantiles N=4)
 *
 * 对输入网格的**所有点标量**属性数组计算 5 行统计：
 *   Min / Q1 / Median / Q3 / Max
 *
 * 与 ParaView 对齐的口径（依据本机 ParaView 6.1.1 实测结果）：
 *   1. 只统计点数据（point data）标量数组，单元数据数组不参与；
 *   2. 分位数采用 Tukey hinges 定义（下/上半区的中位数；样本数为奇数时
 *      中位数不参与任何一半），而非线性插值（R7 / PERCENTILE.INC）。
 *
 * 输出：一个包含 5 个顶点的 PointSet，每个顶点对应一行统计结果。
 *   对每个被统计的输入属性 A，在输出 PointSet 上添加一个标量数组
 *   "A_Quartiles"（dim=1，长度=5），依次为 [Min, Q1, Median, Q3, Max]。
 *
 * 行索引：0=Min, 1=Q1, 2=Median, 3=Q3, 4=Max
 *
 * 若通过 SetAttributeByName / SetAttributeByIndex 指定单个属性，
 * 则只统计该属性（向后兼容模式；该模式为 iGame 扩展，可指定任意属性，
 * ParaView 无对应入口，其属性面板仅有 Input）。
 */
class ComputeQuartilesFilter : public Filter {
public:
    I_OBJECT(ComputeQuartilesFilter);
    static Pointer New() { return new ComputeQuartilesFilter; }

    ~ComputeQuartilesFilter() override = default;

    bool Execute() override;

    // 设置只统计某个属性（可选，二选一即可；不设则统计全部标量属性）
    void SetAttributeByName(const std::string& name) { m_AttrName = name; m_ProcessAll = false; }
    void SetAttributeByIndex(int index) { m_AttrIndex = index; m_ProcessAll = false; }

    std::string GetMessage() const { return m_Message; }

    // Per-array 结果（多属性模式下可用）
    struct QuartileResult {
        std::string arrayName;
        IGenum      attachmentType{IG_POINT}; // IG_POINT or IG_CELL
        double      min{0}, q1{0}, median{0}, q3{0}, max{0};
    };
    const std::vector<QuartileResult>& GetResults() const { return m_Results; }

    // 兼容旧 API：单属性模式（或全部模式下的第一个结果）下返回该属性的 5 项统计
    double GetMin()    const { return m_Results.empty() ? 0.0 : m_Results.front().min; }
    double GetQ1()     const { return m_Results.empty() ? 0.0 : m_Results.front().q1; }
    double GetMedian() const { return m_Results.empty() ? 0.0 : m_Results.front().median; }
    double GetQ3()     const { return m_Results.empty() ? 0.0 : m_Results.front().q3; }
    double GetMax()    const { return m_Results.empty() ? 0.0 : m_Results.front().max; }

    // 已废弃：保留返回 0 以兼容旧调用方（属于 Descriptive Statistics 范畴，不属于 Compute Quartiles）
    double GetMean()   const { return 0.0; }
    double GetSum()    const { return 0.0; }
    double GetStdDev() const { return 0.0; }
    int    GetCount()  const { return 0; }

protected:
    ComputeQuartilesFilter();

private:
    // 从属性数组中收集所有标量值到 values（取第 0 分量）
    static bool CollectValues(ArrayObject::Pointer array, std::vector<double>& values);

    // 中位数：偶数个样本取中间两个的平均值
    static double ComputeMedian(const std::vector<double>& sortedValues);

    // Tukey hinge：lowerHalf=true 取下半区中位数(Q1)，false 取上半区中位数(Q3)
    // 样本数为奇数时，中位数本身不参与任何一半
    static double ComputeHinge(const std::vector<double>& sortedValues, bool lowerHalf);

    // 对单个属性计算 5 行统计，写入 result
    static bool ComputeOne(ArrayObject::Pointer array, QuartileResult& result);

    std::string m_AttrName;
    int         m_AttrIndex{-1};
    bool        m_ProcessAll{true};
    std::string m_Message;
    std::vector<QuartileResult> m_Results;
};

IGAME_NAMESPACE_END
#endif
