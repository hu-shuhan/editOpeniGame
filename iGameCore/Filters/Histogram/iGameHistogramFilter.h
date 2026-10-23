#ifndef iGameHistogramFilter_h
#define iGameHistogramFilter_h

#include "iGameFilter.h"
#include "iGameHistogramData.h"

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @class HistogramFilter
 * @brief 统计属性值的分布（直方图），语义逐项对齐 VTK `vtkExtractHistogram`（ParaView 的 "Histogram" 过滤器）。
 *
 * 参数与 ParaView 面板项的对应关系：
 *  - 数组选择   `SetAttributeIndex()`            ↔ `SelectInputArray`
 *  - 分箱数     `SetNumberOfBins()`              ↔ `BinCount`（默认 10）
 *  - 分量       `SetComponent()`                 ↔ `Component`（默认 0；等于数组维数时按模长 sqrt(Σv²)）
 *  - 居中分箱   `SetCenterBinsAroundMinAndMax()`  ↔ `CenterBinsAroundMinAndMax`（默认 false）
 *  - 自定义范围 `SetUseCustomBinRanges()` + `SetCustomBinRanges()` ↔ `UseCustomBinRanges` / `CustomBinRanges`
 *  - 归一化     `SetNormalize()`                 ↔ `Normalize`（默认 false）
 *  - 累计       `SetAccumulation()`              ↔ `Accumulation`（默认 false，输出 bin_accumulation）
 *  - 计算均值   `SetCalculateAverages()`         ↔ `CalculateAverages`（默认 false，输出 <名>_total / <名>_average）
 *
 * 语义（逐项抄自 `vtkExtractHistogram.cxx`）：
 *  1. 统计范围 = 所选数组该分量的 min/max，排除被 blank 的元组
 *     （点属性 HIDDENPOINT|DUPLICATEPOINT = 2|1，单元属性 HIDDENCELL|DUPLICATECELL = 32|1），
 *     并像 `FiniteMinAndMaxWithBlankingFunctor` 一样**跳过非有限值**；
 *  2. `UseCustomBinRanges` 时用 `CustomBinRanges`（两端反了会先交换，与 VTK 的警告分支一致）；
 *  3. `min == max` 时范围扩成 `[min-0.5, max+0.5]`（`InitializeBinExtents`）；
 *  4. `bin_delta = (max-min)/(CenterBinsAroundMinAndMax ? BinCount-1 : BinCount)`，
 *     `bin_extents[i] = min + i*bin_delta + (居中 ? 0 : bin_delta/2)`（`FillBinExtents`）；
 *  5. 计数 `bin = clamp((int)((value - min + (居中 ? bin_delta/2 : 0)) / bin_delta), 0, BinCount-1)`；
 *     计数遍**不筛非有限值**：VTK 里 `static_cast<int>(NaN/±Inf)` 得到 INT_MIN 后被 Clamp 夹到 0，
 *     本实现显式把非有限值计入 bin 0，结果与该 VTK 构建一致；
 *  6. `Normalize` 把 bin_values 换成 vtkDoubleArray（count / 总计数）；`Accumulation` 追加累计列；
 *     `CalculateAverages` 为属性集里其余每个数组追加 `<名>_total` / `<名>_average` 两列。
 *
 * 输出：`HistogramData`（见 iGameHistogramData.h），可由 Qt 侧直接画成柱状图。
 */
class HistogramFilter : public Filter {
public:
    I_OBJECT(HistogramFilter);
    static Pointer New() { return new HistogramFilter; }

    /** 数组选择：属性集里的属性下标；< 0 时沿用输入对象当前的属性（DataObject::GetAttributeIndex()）。 */
    void SetAttributeIndex(int attributeIndex) { m_AttributeIndex = attributeIndex; }
    int GetAttributeIndex() const { return m_AttributeIndex; }

    /** 分箱数：等价 ParaView 的 BinCount，默认 10；小于 1 时按 1 处理。 */
    void SetNumberOfBins(int numberOfBins) { m_NumberOfBins = numberOfBins < 1 ? 1 : numberOfBins; }
    int GetNumberOfBins() const { return m_NumberOfBins; }

    /** 分量：等价 ParaView 的 Component，默认 0。 */
    void SetComponent(int component) { m_Component = component < 0 ? 0 : component; }
    int GetComponent() const { return m_Component; }

    /** 居中分箱：等价 ParaView 的 CenterBinsAroundMinAndMax，默认 false。 */
    void SetCenterBinsAroundMinAndMax(bool center) { m_CenterBinsAroundMinAndMax = center; }
    bool GetCenterBinsAroundMinAndMax() const { return m_CenterBinsAroundMinAndMax; }

    /** 自定义范围：等价 ParaView 的 UseCustomBinRanges + CustomBinRanges。 */
    void SetUseCustomBinRanges(bool use) { m_UseCustomBinRanges = use; }
    bool GetUseCustomBinRanges() const { return m_UseCustomBinRanges; }
    void SetCustomBinRanges(double minimum, double maximum) {
        m_CustomBinRanges[0] = minimum;
        m_CustomBinRanges[1] = maximum;
    }
    void GetCustomBinRanges(double range[2]) const {
        range[0] = m_CustomBinRanges[0];
        range[1] = m_CustomBinRanges[1];
    }

    /** 归一化：等价 ParaView 的 Normalize，默认 false。 */
    void SetNormalize(bool normalize) { m_Normalize = normalize; }
    bool GetNormalize() const { return m_Normalize; }

    /** 累计：等价 ParaView 的 Accumulation，默认 false。 */
    void SetAccumulation(bool accumulation) { m_Accumulation = accumulation; }
    bool GetAccumulation() const { return m_Accumulation; }

    /** 计算均值：等价 ParaView 的 CalculateAverages，默认 false。 */
    void SetCalculateAverages(bool calculateAverages) { m_CalculateAverages = calculateAverages; }
    bool GetCalculateAverages() const { return m_CalculateAverages; }

    bool Execute() override;

    /** 失败原因（Execute() 返回 false 时有效）。 */
    std::string GetMessage() const { return m_Message; }

    /** 统计结果，Execute() 成功后非空。 */
    HistogramData::Pointer GetHistogramData() const { return m_Data; }

protected:
    HistogramFilter();
    ~HistogramFilter() override = default;

private:
    std::string m_Message;
    int m_AttributeIndex{-1};
    int m_NumberOfBins{10};
    int m_Component{0};
    bool m_CenterBinsAroundMinAndMax{false};
    bool m_UseCustomBinRanges{false};
    bool m_CalculateAverages{false};
    bool m_Normalize{false};
    bool m_Accumulation{false};
    double m_CustomBinRanges[2]{0.0, 1.0};
    HistogramData::Pointer m_Data;
};

IGAME_NAMESPACE_END

#endif
