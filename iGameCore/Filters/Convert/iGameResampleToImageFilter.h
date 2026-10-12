#ifndef iGameResampleToImageFilter_h
#define iGameResampleToImageFilter_h

#include "iGameFilter.h"

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @brief 将输入网格重采样到规则图像网格 (Resample To Image)。
 *
 * 本过滤器是 VTK 中 vtkResampleToImage 的移植实现，功能保持一致：
 * 对输入的网格（PointSet 及其子类）在指定的采样区域上建立一个规则的
 * 图像网格（StructuredMesh），并在每一个网格格点处对输入的点属性进行
 * 探针插值（probe），从而把网格上的场重采样为图像上的点场。
 *
 * 输出是一个 StructuredMesh（等价 vtkImageData）：
 *   - 原点 origin   = samplingBounds 的 (xmin, ymin, zmin)；
 *   - 间距 spacing  = (SamplingDimensions[i] == 1) ? 0
 *                     : (bounds[2i+1] - bounds[2i]) / (SamplingDimensions[i]-1)；
 *   - 维度 dimensions = SamplingDimensions（默认 10 x 10 x 10）；
 *   - 点数据中带有一个 char 数组 "vtkValidPointMask"，格点落在输入网格内为 1，
 *     否则为 0（同时对应插值点场数组默认值 0）。
 *
 * 参数语义与 VTK 一致：
 *   - UseInputBounds = true 时使用输入包围盒（并向内收缩 epsilon 以避免
 *     浮点误差导致的越界采样）；否则使用显式 SamplingBounds。
 *   - SamplingBounds / SamplingDimensions 默认为 {0,1,0,1,0,1} / {10,10,10}。
 */
class ResampleToImageFilter : public Filter {
public:
    I_OBJECT(ResampleToImageFilter);
    static Pointer New() { return new ResampleToImageFilter; }

    bool Execute() override;

    //@{
    /// 输出图像的采样维度（格点数）。默认 {10, 10, 10}，等价 VTK 的 SamplingDimensions。
    void SetSamplingDimensions(int i, int j, int k);
    void SetSamplingDimensions(int dims[3]);
    int* GetSamplingDimensions() { return this->SamplingDimensions; }
    void GetSamplingDimensions(int dims[3]) const;
    //@}

    //@{
    /// 显式采样区域（UseInputBounds 为 false 时生效）。默认 {0,1,0,1,0,1}。
    void SetSamplingBounds(const double bounds[6]);
    void SetSamplingBounds(double x0, double x1, double y0, double y1, double z0, double z1);
    void GetSamplingBounds(double bounds[6]) const;
    double* GetSamplingBounds() { return this->SamplingBounds; }
    //@}

    //@{
    /// 是否使用输入数据的包围盒作为采样区域。默认 true，等价 VTK 的 UseInputBounds。
    void SetUseInputBounds(bool b) { this->UseInputBounds = b; }
    bool GetUseInputBounds() const { return this->UseInputBounds; }
    //@}

    /// 有效点掩膜数组名。固定为 "vtkValidPointMask"，等价 VTK 的 GetMaskArrayName()。
    static const char* GetMaskArrayName() { return "vtkValidPointMask"; }

    /// 本过滤器支持的单元类型清单（用于界面提示，避免用户误以为所有单元都被采样）。
    static const char* GetSupportedCellTypesText();

    /// 判断某个单元类型是否受支持（供界面提示）。
    static bool IsCellTypeSupported(IGenum cellType);

    //@{
    /// 是否对「整型/字符型（离散，含各类 ID）」的点数组禁用线性插值。默认 true。
    /// 这类数组不做线性插值，改用包含该格点的源单元中权重最大的顶点取值（最近顶点
    /// 采样）——对 ID 做线性插值会插出并不存在的 ID，语义错误。
    /// 浮点数组（float/double）始终按 VTK 语义做线性插值。
    void SetDisableInterpolationForDiscreteArrays(bool b) { m_DisableInterpolationForDiscrete = b; }
    bool GetDisableInterpolationForDiscreteArrays() const { return m_DisableInterpolationForDiscrete; }
    //@}

    //@{
    /// 遇到「不支持的单元类型」时是否直接判定失败。默认 true：不产出可能不完整的
    /// 结果，并在 GetMessage() 中给出原因；置为 false 时继续执行，但在 GetMessage()
    /// 中明确列出不支持的单元类型、数量及其影响，避免静默产出不完整结果。
    void SetFailOnUnsupportedCells(bool b) { m_FailOnUnsupportedCells = b; }
    bool GetFailOnUnsupportedCells() const { return m_FailOnUnsupportedCells; }
    //@}

    //@{
    /// 与 VTK vtkProbeFilter 一致的「格点是否落在单元内」容差参数。
    /// ComputeTolerance 为 true（默认，与 VTK 构造函数一致）时容差按
    /// tol2 = 最大单元长度² × 1e-6（vtkProbeFilter::CELL_TOLERANCE_FACTOR_SQR）自动推算，
    /// Tolerance 被忽略；为 false 时 tol2 = Tolerance²（VTK 中 Tolerance 默认 1.0）。
    /// 判定语义也与 VTK 一致：先解出单元局部坐标，截断到单元参数域后重算权重，
    /// 用「点到该最近点的距离」与 tol2 比较，而不是只看重心坐标是否非负。
    void SetTolerance(double t) { m_Tolerance = t; }
    double GetTolerance() const { return m_Tolerance; }
    void SetComputeTolerance(bool b) { m_ComputeTolerance = b; }
    bool GetComputeTolerance() const { return m_ComputeTolerance; }
    //@}

    //@{
    /// 诊断信息（执行后填充）：支持/不支持的单元类型统计、同名点/单元数组冲突、
    /// 离散数组的非插值处理清单等。供界面提示使用，避免静默的不完整/可疑结果。
    const std::string& GetMessage() const { return m_Message; }
    //@}

    //@{
    /// 执行前预估输出规模（不真正执行），供界面在大尺寸时提示用户。
    /// gridPoints / gridCells 为输出的格点数与单元数，memoryMB 为点数据内存量级（MB）。
    /// 返回 false 表示输入无效或采样维度非法。
    bool EstimateOutputSize(IGsize& gridPoints, IGsize& gridCells, double& memoryMB);
    //@}

protected:
    ResampleToImageFilter();
    ~ResampleToImageFilter() override = default;

    int SamplingDimensions[3] = {10, 10, 10};
    double SamplingBounds[6] = {0.0, 1.0, 0.0, 1.0, 0.0, 1.0};
    bool UseInputBounds = true;
    bool m_DisableInterpolationForDiscrete{true};
    bool m_FailOnUnsupportedCells{true};
    double m_Tolerance{1.0};
    bool m_ComputeTolerance{true};
    std::string m_Message;
};

IGAME_NAMESPACE_END
#endif
