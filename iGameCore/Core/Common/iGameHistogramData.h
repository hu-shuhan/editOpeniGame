#pragma once

#include "iGameDataObject.h"
#include "iGameFlatArray.h"

#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @class HistogramData
 * @brief 属性值分布（直方图）结果容器，对齐 VTK `vtkExtractHistogram` 输出的 `vtkTable`：
 *        - "bin_extents"：每个 bin 的中心（vtkDoubleArray -> DoubleArray），长度 = 分箱数；
 *        - "bin_values" ：每个 bin 的计数（vtkIntArray -> IntArray；Normalize 打开时是
 *          vtkDoubleArray -> DoubleArray，与 VTK 的 NormalizeBins 一致）；
 *        - "bin_accumulation"：Accumulation 打开时的累计列（与 bin_values 同类型）；
 *        - "<数组名>_total" / "<数组名>_average"：CalculateAverages 打开时，属性集里其余每个
 *          数组各两列（多分量按原维数展开，与 VTK 的 RequestData 写法一致）。
 *        只承载统计结果，不含几何数据，不参与渲染。
 */
class HistogramData : public DataObject {
public:
    I_OBJECT(HistogramData);
    static Pointer New() { return new HistogramData(); }

    void SetBinExtents(DoubleArray::Pointer extents) {
        m_BinExtents = extents;
        this->Modified();
    }
    DoubleArray::Pointer GetBinExtents() const { return m_BinExtents; }

    /** 计数列：默认 IntArray；打开 Normalize 后是 DoubleArray（与 VTK 一致）。 */
    void SetBinValues(ArrayObject::Pointer values) {
        m_BinValues = values;
        this->Modified();
    }
    ArrayObject::Pointer GetBinValues() const { return m_BinValues; }

    /** bin_values 是否为归一化后的浮点列。 */
    void SetNormalized(bool normalized) { m_Normalized = normalized; }
    bool GetNormalized() const { return m_Normalized; }

    void SetBinAccumulation(ArrayObject::Pointer accumulation) {
        m_BinAccumulation = accumulation;
        this->Modified();
    }
    ArrayObject::Pointer GetBinAccumulation() const { return m_BinAccumulation; }

    /** CalculateAverages 产生的 (total, average) 列，按属性集顺序。 */
    void AddAverageColumns(ArrayObject::Pointer total, ArrayObject::Pointer average) {
        m_AverageColumns.emplace_back(total, average);
    }
    const std::vector<std::pair<ArrayObject::Pointer, ArrayObject::Pointer>>& GetAverageColumns() const {
        return m_AverageColumns;
    }

    int GetNumberOfBins() const {
        return m_BinValues ? static_cast<int>(m_BinValues->GetNumberOfValues()) : 0;
    }

    void SetSourceArrayName(const std::string& name) { m_SourceArrayName = name; }
    const std::string& GetSourceArrayName() const { return m_SourceArrayName; }

    void SetSourceAttachmentType(IGenum attachmentType) { m_SourceAttachmentType = attachmentType; }
    IGenum GetSourceAttachmentType() const { return m_SourceAttachmentType; }

    void SetBinRange(double minimum, double maximum) {
        m_BinRange[0] = minimum;
        m_BinRange[1] = maximum;
    }
    double GetBinMinimum() const { return m_BinRange[0]; }
    double GetBinMaximum() const { return m_BinRange[1]; }

    /** 实际计入统计的元组数（排除 blanked 元组）。 */
    void SetNumberOfCountedValues(IGsize count) { m_NumberOfCountedValues = count; }
    IGsize GetNumberOfCountedValues() const { return m_NumberOfCountedValues; }

protected:
    HistogramData() = default;
    ~HistogramData() override = default;

protected:
    DoubleArray::Pointer m_BinExtents;
    ArrayObject::Pointer m_BinValues;
    ArrayObject::Pointer m_BinAccumulation;
    std::vector<std::pair<ArrayObject::Pointer, ArrayObject::Pointer>> m_AverageColumns;
    std::string m_SourceArrayName;
    IGenum m_SourceAttachmentType{IG_POINT};
    double m_BinRange[2]{0.0, 0.0};
    IGsize m_NumberOfCountedValues{0};
    bool m_Normalized{false};
};

IGAME_NAMESPACE_END
