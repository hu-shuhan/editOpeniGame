#include "iGameRegionFeatureBasisFilter.h"

#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGamePointSet.h"

#include <algorithm>
#include <cmath>
#include <utility>

IGAME_NAMESPACE_BEGIN

namespace {
double coordinateMagnitude(const Point& point) {
    const double x = point[0];
    const double y = point[1];
    const double z = point[2];
    return std::sqrt(x * x + y * y + z * z);
}
} // 匿名命名空间

RegionFeatureBasisFilter::RegionFeatureBasisFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void RegionFeatureBasisFilter::SetGeometryField() {
    m_UseGeometry = true;
    m_AttributeIndex = -1;
    m_AttributeName.clear();
    m_AttachmentType = IG_POINT;
    m_Component = -1;
}

void RegionFeatureBasisFilter::SetAttributeByIndex(int index, IGenum attachmentType, int component) {
    m_UseGeometry = false;
    m_AttributeIndex = index;
    m_AttributeName.clear();
    m_AttachmentType = attachmentType;
    m_Component = component;
}

void RegionFeatureBasisFilter::SetAttributeByName(const std::string& name, IGenum attachmentType, int component) {
    m_UseGeometry = false;
    m_AttributeIndex = -1;
    m_AttributeName = name;
    m_AttachmentType = attachmentType;
    m_Component = component;
}

bool RegionFeatureBasisFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "input is empty";
        return false;
    }

    std::vector<double> baseValues;
    IGenum attachmentType = IG_NONE;
    std::string fieldName = m_UseGeometry ? "Geometry" : m_AttributeName;

    if (m_UseGeometry) {
        if (!BuildGeometryValues(input, baseValues, attachmentType)) return false;
    } else {
        auto* attributeSet = input->GetAttributeSet();
        if (attributeSet == nullptr) {
            m_Message = "attribute set is empty";
            return false;
        }

        auto lookup = FindAttribute(attributeSet);
        if (lookup.attribute == nullptr) {
            m_Message = "attribute is not found";
            return false;
        }

        if (lookup.attribute->pointer != nullptr) fieldName = lookup.attribute->pointer->GetName();
        if (!BuildAttributeValues(lookup.attribute, baseValues, attachmentType)) return false;
    }

    auto output = RegionFeatureBasisData::New();
    output->SetFieldName(fieldName);
    output->SetAttachmentType(attachmentType);
    output->SetValues(ApplyBasisMode(std::move(baseValues)));
    if (m_HistogramBinCount > 0) {
        output->BuildHistogram(m_HistogramBinCount);
    }

    m_OutputData = output;
    SetOutput(output);
    UpdateProgress(1.0);
    return true;
}

RegionFeatureBasisFilter::AttributeLookupResult RegionFeatureBasisFilter::FindAttribute(AttributeSet* attributeSet) const {
    if (attributeSet == nullptr) return {};

    if (m_AttributeIndex >= 0 && m_AttributeIndex < static_cast<int>(attributeSet->GetNumberOfAttributes())) {
        auto& attribute = attributeSet->GetAttribute(static_cast<IGsize>(m_AttributeIndex));
        if (!attribute.isDeleted && attribute.pointer != nullptr &&
            (m_AttachmentType == IG_NONE || attribute.attachmentType == m_AttachmentType)) {
            return {m_AttributeIndex, &attribute};
        }
    }

    if (m_AttributeName.empty()) return {};

    auto attributes = attributeSet->GetAllAttributes();
    if (attributes == nullptr) return {};

    for (int i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto& attribute = attributes->GetElement(i);
        if (attribute.isDeleted || attribute.pointer == nullptr) continue;
        if (m_AttachmentType != IG_NONE && attribute.attachmentType != m_AttachmentType) continue;
        if (attribute.pointer->GetName() == m_AttributeName) return {i, &attribute};
    }
    return {};
}

bool RegionFeatureBasisFilter::BuildGeometryValues(DataObject::Pointer input, std::vector<double>& values,
                                                   IGenum& attachmentType) {
    auto pointSet = DynamicCast<PointSet>(input);
    if (pointSet == nullptr || pointSet->GetNumberOfPoints() == 0) {
        m_Message = "geometry is empty";
        return false;
    }

    attachmentType = IG_POINT;
    values.reserve(static_cast<std::size_t>(pointSet->GetNumberOfPoints()));
    for (IGsize i = 0; i < pointSet->GetNumberOfPoints(); ++i) {
        values.push_back(coordinateMagnitude(pointSet->GetPoint(i)));
    }
    return true;
}

bool RegionFeatureBasisFilter::BuildAttributeValues(AttributeSet::Attribute* attribute, std::vector<double>& values,
                                                    IGenum& attachmentType) {
    if (attribute == nullptr || attribute->pointer == nullptr) {
        m_Message = "attribute is empty";
        return false;
    }

    auto array = attribute->pointer;
    const int dimension = array->GetDimension();
    if (dimension <= 0 || array->GetNumberOfElements() == 0) {
        m_Message = "attribute values are empty";
        return false;
    }
    if (m_Component >= dimension) {
        m_Message = "attribute component is out of range";
        return false;
    }

    attachmentType = attribute->attachmentType;
    const int readComponent = m_Component >= 0 ? m_Component : (dimension == 1 ? 0 : -1);
    values.reserve(static_cast<std::size_t>(array->GetNumberOfElements()));
    for (IGsize i = 0; i < array->GetNumberOfElements(); ++i) {
        values.push_back(array->GetElementValue(i, readComponent));
    }
    return true;
}

std::vector<double> RegionFeatureBasisFilter::ApplyBasisMode(std::vector<double> baseValues) const {
    if (m_Mode == BasisMode::Magnitude || baseValues.size() < 2) return baseValues;

    std::vector<double> result(baseValues.size(), 0.0);
    if (m_Mode == BasisMode::Jump) {
        for (std::size_t i = 0; i < baseValues.size(); ++i) {
            double jump = 0.0;
            if (i > 0) jump = std::max(jump, std::abs(baseValues[i] - baseValues[i - 1]));
            if (i + 1 < baseValues.size()) jump = std::max(jump, std::abs(baseValues[i] - baseValues[i + 1]));
            result[i] = jump;
        }
        return result;
    }

    for (std::size_t i = 0; i < baseValues.size(); ++i) {
        const std::size_t first = i == 0 ? 0 : i - 1;
        const std::size_t last = std::min(baseValues.size() - 1, i + 1);
        double sum = 0.0;
        int count = 0;
        for (std::size_t j = first; j <= last; ++j) {
            sum += baseValues[j];
            ++count;
        }
        const double mean = count > 0 ? sum / static_cast<double>(count) : 0.0;
        double variance = 0.0;
        for (std::size_t j = first; j <= last; ++j) {
            const double diff = baseValues[j] - mean;
            variance += diff * diff;
        }
        result[i] = count > 0 ? std::sqrt(variance / static_cast<double>(count)) : 0.0;
    }
    return result;
}

IGAME_NAMESPACE_END
