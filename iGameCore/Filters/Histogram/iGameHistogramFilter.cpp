#include "iGameHistogramFilter.h"

#include "iGameAttributeSet.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {
// vtkDataSetAttributes 的 ghost 位（D:\iGame\vtkDataSetAttributes.h）：
//   DUPLICATECELL = 1, DUPLICATEPOINT = 1, HIDDENPOINT = 2, HIDDENCELL = 32
constexpr unsigned char kDuplicatePoint = 1;
constexpr unsigned char kHiddenPoint = 2;
constexpr unsigned char kDuplicateCell = 1;
constexpr unsigned char kHiddenCell = 32;
constexpr const char* kGhostArrayName = "vtkghosttype";

// 列名与 vtkExtractHistogram 的输出表一致
constexpr const char* kBinExtentsArrayName = "bin_extents";
constexpr const char* kBinValuesArrayName = "bin_values";
constexpr const char* kBinAccumulationArrayName = "bin_accumulation";

std::string ToLower(const std::string& text) {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower;
}

// 取名为 vtkGhostType 的数组（附着位置须与所选数组一致），找不到返回 nullptr。
// 名字统一转小写比较：legacy VTK 的 FIELD 读取路径（ReadFieldData）会把数组名写成小写，
// 与 iGameRemoveGhostInformationFilter.cpp 中的判断保持一致。
ArrayObject::Pointer GetGhostArray(AttributeSet* attributeSet, IGenum attachmentType) {
    if (attributeSet == nullptr) { return nullptr; }
    auto attributes = attributeSet->GetAllAttributes();
    if (!attributes) { return nullptr; }
    for (IGsize i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto& attribute = attributes->GetElement(i);
        if (attribute.isDeleted || attribute.pointer == nullptr) { continue; }
        if (attribute.attachmentType != attachmentType) { continue; }
        if (ToLower(attribute.pointer->GetName()) != kGhostArrayName) { continue; }
        return attribute.pointer;
    }
    return nullptr;
}

// CalculateAverages：属性集里其余某个数组在每个 bin 上的分量累加值
struct AverageAccumulator {
    ArrayObject::Pointer source;
    int dimension{1};
    std::vector<std::vector<double>> totals;
};
} // namespace

HistogramFilter::HistogramFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool HistogramFilter::Execute() {
    m_Message.clear();
    m_Data = nullptr;

    if (!m_Inputs || m_Inputs->GetNumberOfElements() == 0) {
        m_Message = "没有输入数据，请先加载模型。";
        igError("HistogramFilter: no input data");
        return false;
    }
    DataObject::Pointer input = m_Inputs->GetElement(0);
    if (!input) {
        m_Message = "输入数据为空。";
        igError("HistogramFilter: input is null");
        return false;
    }
    AttributeSet* attributeSet = input->GetAttributeSet();
    if (!attributeSet || attributeSet->GetNumberOfAttributes() == 0) {
        m_Message = "输入没有属性集，无法统计属性值分布。";
        igError("HistogramFilter: input has no attribute set");
        return false;
    }

    // 数组选择：未指定时沿用输入对象当前的属性（等价 ParaView 面板里默认选中的当前数组）
    int attributeIndex = m_AttributeIndex;
    if (attributeIndex < 0) { attributeIndex = input->GetAttributeIndex(); }
    if (attributeIndex < 0 || attributeIndex >= static_cast<int>(attributeSet->GetNumberOfAttributes())) {
        m_Message = "请选择有效的属性数组。";
        igError("HistogramFilter: invalid attribute index {}", attributeIndex);
        return false;
    }
    auto& attribute = attributeSet->GetAttribute(attributeIndex);
    if (attribute.isDeleted || attribute.pointer == nullptr) {
        m_Message = "所选属性数组为空。";
        igError("HistogramFilter: selected attribute is empty");
        return false;
    }
    ArrayObject::Pointer array = attribute.pointer;
    const IGenum attachmentType = attribute.attachmentType;

    const int dimension = std::max(1, array->GetDimension());
    const IGsize numberOfTuples = array->GetNumberOfValues() / dimension;
    if (numberOfTuples <= 0) {
        m_Message = "所选属性数组没有数据。";
        igError("HistogramFilter: selected attribute has no value");
        return false;
    }

    // 分量：VTK 里 Component == 维数 表示取模长 sqrt(Σv²)，Component < 0 或越界则报错
    const int component = m_Component;
    if (component > dimension) {
        m_Message = "所选分量超出数组维数。";
        igError("HistogramFilter: component {} out of range {}", component, dimension);
        return false;
    }
    const bool magnitude = (component == dimension);

    // Blanking：点属性 HIDDENPOINT|DUPLICATEPOINT，单元属性 HIDDENCELL|DUPLICATECELL
    ArrayObject::Pointer ghostArray = GetGhostArray(attributeSet, attachmentType);
    const unsigned char ghostMask = (attachmentType == IG_POINT)
                                            ? static_cast<unsigned char>(kHiddenPoint | kDuplicatePoint)
                                            : static_cast<unsigned char>(kHiddenCell | kDuplicateCell);
    auto isBlanked = [&](IGsize tuple) {
        if (ghostArray.IsNull() || tuple >= ghostArray->GetNumberOfValues()) { return false; }
        auto ghostValues = DynamicCast<UnsignedCharArray>(ghostArray);
        if (ghostValues.IsNull()) {
            return (static_cast<unsigned char>(ghostArray->GetValue(tuple)) & ghostMask) != 0;
        }
        return (ghostValues->RawPointer()[tuple] & ghostMask) != 0;
    };
    auto valueOf = [&](IGsize tuple) {
        if (magnitude) {
            double squareSum = 0.0;
            for (int c = 0; c < dimension; ++c) {
                const double value = array->GetElementValue(tuple, c);
                squareSum += value * value;
            }
            return std::sqrt(squareSum);
        }
        return array->GetElementValue(tuple, component);
    };

    const int numberOfBins = m_NumberOfBins; // 已保证 >= 1

    // ---- 统计范围（InitializeBinExtents）----
    double minimum = std::numeric_limits<double>::max();
    double maximum = std::numeric_limits<double>::lowest();
    if (m_UseCustomBinRanges) {
        minimum = m_CustomBinRanges[0];
        maximum = m_CustomBinRanges[1];
        if (maximum < minimum) { std::swap(minimum, maximum); } // VTK：两端反了先交换并告警
    } else {
        // FiniteMinAndMaxWithBlankingFunctor：排除 blanked 元组，且跳过非有限值
        for (IGsize tuple = 0; tuple < numberOfTuples; ++tuple) {
            if (isBlanked(tuple)) { continue; }
            const double value = valueOf(tuple);
            if (!std::isfinite(value)) { continue; }
            minimum = std::min(minimum, value);
            maximum = std::max(maximum, value);
        }
        if (minimum > maximum) {
            m_Message = "所选数组没有可统计的值（可能所有元组都被 blanked，或取值全为非有限值）。";
            igError("HistogramFilter: no countable value in the selected array");
            return false;
        }
    }
    if (minimum == maximum) {
        minimum -= 0.5;
        maximum += 0.5;
    }

    // ---- bin 中心（FillBinExtents）----
    const double binDelta =
            (maximum - minimum) / static_cast<double>(m_CenterBinsAroundMinAndMax ? (numberOfBins - 1)
                                                                                 : numberOfBins);
    const double halfDelta = binDelta / 2.0;
    auto binExtents = DoubleArray::New();
    binExtents->SetName(kBinExtentsArrayName);
    binExtents->SetDimension(1);
    binExtents->Resize(numberOfBins);
    for (int i = 0; i < numberOfBins; ++i) {
        binExtents->SetValue(i, minimum + i * binDelta + (m_CenterBinsAroundMinAndMax ? 0.0 : halfDelta));
    }

    // ---- CalculateAverages 的累加器：属性集里除被统计数组外的每个数组 ----
    std::vector<AverageAccumulator> accumulators;
    if (m_CalculateAverages) {
        auto attributes = attributeSet->GetAllAttributes();
        for (IGsize i = 0; i < attributes->GetNumberOfElements(); ++i) {
            auto& other = attributes->GetElement(i);
            if (other.isDeleted || other.pointer == nullptr) { continue; }
            if (other.attachmentType != attachmentType) { continue; }
            if (other.pointer == array) { continue; }              // VTK: array != DataArray
            if (other.pointer->GetName().empty()) { continue; }    // VTK: array->GetName() 非空
            AverageAccumulator accumulator;
            accumulator.source = other.pointer;
            accumulator.dimension = std::max(1, other.pointer->GetDimension());
            accumulator.totals.assign(numberOfBins, std::vector<double>(accumulator.dimension, 0.0));
            accumulators.push_back(std::move(accumulator));
        }
    }

    // ---- 计数（BinAnArrayFunctor）----
    std::vector<long long> counts(numberOfBins, 0);
    IGsize countedValues = 0;
    for (IGsize tuple = 0; tuple < numberOfTuples; ++tuple) {
        if ((tuple % 50000) == 0) {
            this->UpdateProgress(static_cast<double>(tuple) / static_cast<double>(numberOfTuples));
        }
        if (isBlanked(tuple)) { continue; }
        const double value = valueOf(tuple);
        ++countedValues;

        int binIndex = 0;
        if (std::isfinite(value)) {
            binIndex = static_cast<int>((value - minimum + (m_CenterBinsAroundMinAndMax ? halfDelta : 0.0)) /
                                        binDelta);
            binIndex = std::clamp(binIndex, 0, numberOfBins - 1);
        }
        // 非有限值：VTK 的计数遍不筛 NaN/±Inf，static_cast<int> 在本平台得到 INT_MIN 后被 Clamp 夹到 0，
        // 因此这里显式把非有限值计入第一个 bin（与实测 [1,nan,2,inf,3,4,5,6] -> [4,1,1,2] 一致）
        counts[binIndex] += 1;

        for (auto& accumulator : accumulators) {
            for (int c = 0; c < accumulator.dimension; ++c) {
                accumulator.totals[binIndex][c] += accumulator.source->GetElementValue(tuple, c);
            }
        }
    }

    // ---- 结果容器 ----
    auto data = HistogramData::New();
    data->SetName(array->GetName() + "_histogram");
    data->SetBinExtents(binExtents);
    data->SetSourceArrayName(array->GetName());
    data->SetSourceAttachmentType(attachmentType);
    data->SetBinRange(minimum, maximum);
    data->SetNumberOfCountedValues(countedValues);

    long long totalCount = 0;
    for (const long long count : counts) { totalCount += count; }

    if (m_Normalize) {
        // VTK 的 NormalizeBins：整列换成 vtkDoubleArray，值 = count / 总计数
        auto values = DoubleArray::New();
        values->SetName(kBinValuesArrayName);
        values->SetDimension(1);
        values->Resize(numberOfBins);
        for (int i = 0; i < numberOfBins; ++i) {
            values->SetValue(i, totalCount != 0 ? static_cast<double>(counts[i]) / static_cast<double>(totalCount)
                                                : 0.0);
        }
        data->SetBinValues(values);
        data->SetNormalized(true);
    } else {
        auto values = IntArray::New();
        values->SetName(kBinValuesArrayName);
        values->SetDimension(1);
        values->Resize(numberOfBins);
        for (int i = 0; i < numberOfBins; ++i) { values->SetValue(i, static_cast<double>(counts[i])); }
        data->SetBinValues(values);
        data->SetNormalized(false);
    }

    // CalculateAverages 的两列：<名>_total / <名>_average（多分量按原维数展开）
    for (auto& accumulator : accumulators) {
        const int components = accumulator.dimension;
        auto totals = DoubleArray::New();
        totals->SetName(accumulator.source->GetName() + "_total");
        totals->SetDimension(components);
        totals->Resize(numberOfBins);
        auto averages = DoubleArray::New();
        averages->SetName(accumulator.source->GetName() + "_average");
        averages->SetDimension(components);
        averages->Resize(numberOfBins);
        for (int i = 0; i < numberOfBins; ++i) {
            for (int c = 0; c < components; ++c) {
                const double total = accumulator.totals[i][c];
                totals->SetValue(i * components + c, total);
                averages->SetValue(i * components + c,
                                   counts[i] != 0 ? total / static_cast<double>(counts[i]) : 0.0);
            }
        }
        data->AddAverageColumns(totals, averages);
    }

    // Accumulation：与 bin_values 同类型的累计列（在 Normalize 之后累计，与 RequestData 顺序一致）
    if (m_Accumulation) {
        double running = 0.0;
        if (m_Normalize) {
            auto accumulation = DoubleArray::New();
            accumulation->SetName(kBinAccumulationArrayName);
            accumulation->SetDimension(1);
            accumulation->Resize(numberOfBins);
            const auto values = DynamicCast<DoubleArray>(data->GetBinValues());
            for (int i = 0; i < numberOfBins; ++i) {
                running += values->GetValue(i);
                accumulation->SetValue(i, running);
            }
            data->SetBinAccumulation(accumulation);
        } else {
            auto accumulation = IntArray::New();
            accumulation->SetName(kBinAccumulationArrayName);
            accumulation->SetDimension(1);
            accumulation->Resize(numberOfBins);
            for (int i = 0; i < numberOfBins; ++i) {
                running += static_cast<double>(counts[i]);
                accumulation->SetValue(i, running);
            }
            data->SetBinAccumulation(accumulation);
        }
    }

    this->UpdateProgress(1.0);
    m_Data = data;
    this->SetOutput(0, data);
    return true;
}

IGAME_NAMESPACE_END
