#include "iGameComputeQuartilesFilter.h"
#include "iGameAttributeSet.h"
#include <algorithm>
#include <iostream>

IGAME_NAMESPACE_BEGIN

ComputeQuartilesFilter::ComputeQuartilesFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool ComputeQuartilesFilter::CollectValues(ArrayObject::Pointer array,
                                           std::vector<double>& values) {
    if (!array) return false;

    int dim = array->GetDimension();
    if (dim < 1) return false;

    IGsize numElements = array->GetNumberOfElements();
    if (numElements == 0) return false;

    values.reserve(numElements);
    std::vector<float> buf(dim, 0.0f);

    for (IGsize i = 0; i < numElements; i++) {
        array->GetElement(i, buf.data());
        values.push_back(static_cast<double>(buf[0])); // 取第 0 分量作为标量
    }

    return !values.empty();
}

double ComputeQuartilesFilter::ComputeMedian(
    const std::vector<double>& sortedValues) {
    size_t n = sortedValues.size();
    if (n == 0) return 0.0;
    if (n % 2) return sortedValues[n / 2];
    return 0.5 * (sortedValues[n / 2 - 1] + sortedValues[n / 2]);
}

double ComputeQuartilesFilter::ComputeHinge(
    const std::vector<double>& sortedValues, bool lowerHalf) {
    size_t n = sortedValues.size();
    if (n == 0) return 0.0;
    if (n == 1) return sortedValues[0];

    size_t half = n / 2;
    // Tukey hinges：奇数个数据时中位数不参与任何一半
    size_t begin = lowerHalf ? 0 : (n % 2 ? half + 1 : half);
    size_t end   = lowerHalf ? half : n;

    std::vector<double> sub(sortedValues.begin() + begin, sortedValues.begin() + end);
    return ComputeMedian(sub);
}

bool ComputeQuartilesFilter::ComputeOne(ArrayObject::Pointer array,
                                        QuartileResult& result) {
    std::vector<double> values;
    if (!CollectValues(array, values)) {
        return false;
    }

    std::sort(values.begin(), values.end());

    result.min    = values.front();
    result.max    = values.back();
    result.median = ComputeMedian(values);
    result.q1     = ComputeHinge(values, true);
    result.q3     = ComputeHinge(values, false);
    return true;
}

bool ComputeQuartilesFilter::Execute() {
    m_Results.clear();

    auto input = GetInput(0);
    if (!input) {
        m_Message = "ComputeQuartilesFilter: no input data.";
        return false;
    }

    auto attrSet = input->GetAttributeSet();
    if (!attrSet || attrSet->GetNumberOfAttributes() == 0) {
        m_Message = "ComputeQuartilesFilter: input has no attributes.";
        return false;
    }

    if (m_ProcessAll) {
        // 遍历所有点标量属性，对标 ParaView Compute Quartiles
        int n = attrSet->GetNumberOfAttributes();
        for (int i = 0; i < n; i++) {
            auto& attr = attrSet->GetAttribute(i);
            if (attr.IsNone()) continue;
            // vtkComputeQuartiles 只统计点数据数组，单元数据数组不参与
            if (attr.attachmentType != IG_POINT) continue;
            // Compute Quartiles 只对标量（dim==1）做；向量属性跳过
            if (attr.pointer->GetDimension() != 1) continue;

            QuartileResult r;
            r.arrayName = attr.pointer->GetName();
            r.attachmentType = attr.attachmentType;
            if (ComputeOne(attr.pointer, r)) {
                m_Results.push_back(std::move(r));
            }
        }
        if (m_Results.empty()) {
            m_Message = "ComputeQuartilesFilter: no scalar attributes found.";
            return false;
        }
    } else {
        // 单属性模式（向后兼容）
        int attrIndex = m_AttrIndex;
        if (attrIndex < 0 && !m_AttrName.empty()) {
            attrIndex = attrSet->GetAttributeIndex(m_AttrName);
        }
        if (attrIndex < 0 || attrIndex >= attrSet->GetNumberOfAttributes()) {
            m_Message = "ComputeQuartilesFilter: attribute not found.";
            return false;
        }
        auto& attr = attrSet->GetAttribute(attrIndex);
        if (attr.IsNone()) {
            m_Message = "ComputeQuartilesFilter: attribute is invalid.";
            return false;
        }
        QuartileResult r;
        r.arrayName = attr.pointer->GetName();
        r.attachmentType = attr.attachmentType;
        if (!ComputeOne(attr.pointer, r)) {
            m_Message = "ComputeQuartilesFilter: failed to collect values from attribute.";
            return false;
        }
        m_Results.push_back(std::move(r));
    }

    // 构造输出：5 顶点 PointSet 承载 5 行统计（Min/Q1/Median/Q3/Max）
    // 对每个被统计属性 A，添加标量数组 "A_Quartiles"（dim=1，长度=5）
    auto outPS = PointSet::New();
    auto outPoints = Points::New();
    outPS->RequestEditStatus();
    // 5 个占位顶点（坐标无意义，仅承载统计标量）
    for (int i = 0; i < 5; i++) {
        Point p = { 0.0f, 0.0f, 0.0f };
        outPoints->AddPoint(p);
    }
    outPS->SetPoints(outPoints);

    auto outAttrSet = outPS->GetAttributeSet();
    if (!outAttrSet) {
        m_Message = "ComputeQuartilesFilter: failed to get output attribute set.";
        return false;
    }

    static const char* kRowNames[5] = { "Min", "Q1", "Median", "Q3", "Max" };

    for (const auto& r : m_Results) {
        auto outArray = DoubleArray::New();
        std::string outName = r.arrayName.empty() ? std::string("Quartiles") : (r.arrayName + "_Quartiles");
        outArray->SetName(outName);
        outArray->SetDimension(1);
        outArray->Reserve(5);

        double vals[5] = { r.min, r.q1, r.median, r.q3, r.max };
        for (int i = 0; i < 5; i++) {
            outArray->AddElement(&vals[i]);
        }
        outAttrSet->AddAttribute(IG_SCALAR, IG_POINT, outArray);
    }

    outPS->Modified();
    if (outAttrSet->GetNumberOfAttributes() > 0) {
        outAttrSet->ForceReConvertToDrawableData();
    }

    // 控制台打印
    std::cout << "[ComputeQuartilesFilter] Processed " << m_Results.size()
              << " scalar array(s). Rows: Min/Q1/Median/Q3/Max" << std::endl;
    for (const auto& r : m_Results) {
        std::cout << "  '" << r.arrayName << "' ("
                  << (r.attachmentType == IG_POINT ? "point" : "cell") << "): "
                  << "Min=" << r.min << "  Q1=" << r.q1 << "  Median=" << r.median
                  << "  Q3=" << r.q3 << "  Max=" << r.max << std::endl;
    }

    SetOutput(outPS);
    return true;
}

IGAME_NAMESPACE_END
