#ifndef iGamePointSetToOctreeFilter_h
#define iGamePointSetToOctreeFilter_h

#include "iGameFilter.h"

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @brief 将点集 (PointSet) 转换为八叉树图像 (StructuredMesh)。
 *
 * 本过滤器是 VTK 中 vtkPointSetToOctreeImageFilter 的移植实现，功能保持一致：
 * 根据输入点集的包围盒与 NumberOfPointsPerCell 把包围盒划分为一个规则的
 * 体网格（分箱数量 numBuckets = numPoints / numPointsPerCell，再经
 * vtkBoundingBox::ComputeDivisions/ClampDivisions 等价算法得到各轴分割数 nDivs），
 * 输出维度 dimensions[i] = nDivs[i] + 1 的 StructuredMesh。
 *
 * 输出网格的每一个体素（cell）带有一个单元标量 "octree"（unsigned char），
 * 它用一个 8 位 bitfield 记录该体素内落入的 8 个八分区（子八叉树节点）：
 *   bit0 = (x<=cx, y<=cy, z<=cz)  bit1 = (x>cx, y<=cy, z<=cz)
 *   bit2 = (x<=cx, y>cy, z<=cz)  bit3 = (x>cx, y>cy, z<=cz)
 *   bit4 = (x<=cx, y<=cy, z>cz)  bit5 = (x>cx, y<=cy, z>cz)
 *   bit6 = (x<=cx, y>cy, z>cz)   bit7 = (x>cx, y>cy, z>cz)
 * 其中 (cx,cy,cz) 为该体素中心的坐标。多个点落在同一体素时按位 OR 累加。
 *
 * 若开启 ProcessInputPointArray，则同时处理一个点属性数组，并把结果作为
 * 一个多分量（每分量一个统计函数）的单元属性数组附加到输出，分量顺序为
 * LastValue/Min/Max/Count/Sum/Mean（依所请求的函数决定），与 VTK 一致。
 *
 * 说明：iGameVis 没有独立的 vtkImageData/vtkPartitionedDataSet 概念，
 * 因此 VTK 的「分片数据集 + 单张图像」在此直接映射为一个 StructuredMesh，
 * 图像的 origin/spacing 通过网格点坐标（origin + ijk * spacing）原样表达。
 */
class PointSetToOctreeFilter : public Filter {
public:
    I_OBJECT(PointSetToOctreeFilter);
    static Pointer New() { return new PointSetToOctreeFilter; }

    bool Execute() override;

    //@{
    /// 每个体素期望容纳的平均点数。默认 1，必须为正整数（自动钳制到 >= 1）。
    void SetNumberOfPointsPerCell(igIndex64 n) { m_NumberOfPointsPerCell = (n < 1 ? 1 : n); }
    igIndex64 GetNumberOfPointsPerCell() const { return m_NumberOfPointsPerCell; }
    //@}

    //@{
    /// 是否处理输入点属性数组（等价 VTK 的 ProcessInputPointArray）。默认 false。
    void SetProcessInputPointArray(bool b) { m_ProcessInputPointArray = b; }
    bool GetProcessInputPointArray() const { return m_ProcessInputPointArray; }
    //@}

    //@{
    /// 指定要处理的输入点属性数组名称（等价 VTK 的 SetInputArrayToProcess）。
    /// 为空时默认取第一个挂载在点 (IG_POINT) 上的标量 (IG_SCALAR) 属性。
    void SetInputPointArrayName(const std::string& name) { m_InputArrayName = name; }
    const std::string& GetInputPointArrayName() const { return m_InputArrayName; }
    //@}

    //@{
    /// 需要计算的统计函数。默认仅开启 Min/Max/Count/Mean（当 ProcessInputPointArray
    /// 关闭时这些开关不影响输出）。语义与 VTK 一致：
    ///   - ComputeMean 开启时必然同时计算 Count 与 Sum；
    ///   - 至少需开启一个函数，否则 Execute 报错。
    void SetComputeLastValue(bool b) { m_ComputeLastValue = b; }
    bool GetComputeLastValue() const { return m_ComputeLastValue; }
    void SetComputeMin(bool b) { m_ComputeMin = b; }
    bool GetComputeMin() const { return m_ComputeMin; }
    void SetComputeMax(bool b) { m_ComputeMax = b; }
    bool GetComputeMax() const { return m_ComputeMax; }
    void SetComputeCount(bool b) { m_ComputeCount = b; }
    bool GetComputeCount() const { return m_ComputeCount; }
    void SetComputeSum(bool b) { m_ComputeSum = b; }
    bool GetComputeSum() const { return m_ComputeSum; }
    void SetComputeMean(bool b) { m_ComputeMean = b; }
    bool GetComputeMean() const { return m_ComputeMean; }
    //@}

    //@{
    /// 诊断信息（执行后填充）：输出图像维度、体素数量、参与统计的点属性与分量数等，
    /// 供界面提示使用。
    const std::string& GetMessage() const { return m_Message; }
    //@}

protected:
    PointSetToOctreeFilter();
    ~PointSetToOctreeFilter() override = default;

    enum class FieldFunctions {
        LAST_VALUE,
        MIN,
        MAX,
        COUNT,
        SUM,
        MEAN,
    };

    igIndex64 m_NumberOfPointsPerCell{1};
    bool m_ProcessInputPointArray{false};
    bool m_ComputeLastValue{false};
    bool m_ComputeMin{true};
    bool m_ComputeMax{true};
    bool m_ComputeCount{true};
    bool m_ComputeSum{false};
    bool m_ComputeMean{true};
    std::string m_InputArrayName{};
    std::string m_Message;

    // VTK vtkBoundingBox::ComputeDivisions / ClampDivisions 的等价实现。
    static void ComputeDivisions(igIndex64 totalBins, const double minPnt[3], const double maxPnt[3],
                                 double bounds[6], int divs[3]);
    static void ClampDivisions(igIndex64 targetBins, int divs[3]);
};

IGAME_NAMESPACE_END
#endif
