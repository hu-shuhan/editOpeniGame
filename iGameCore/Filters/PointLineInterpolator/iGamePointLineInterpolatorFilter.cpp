#include "iGamePointLineInterpolatorFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePointFinder.h"
#include "iGamePoints.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN
namespace {
constexpr double ExactHitTolerance = std::numeric_limits<double>::epsilon() * 256.0;

ArrayObject::Pointer NewArrayLike(ArrayObject* input) {
    if (!input) return nullptr;
    switch (input->GetArrayType()) {
        case IG_FloatArray: return FloatArray::New();
        case IG_DoubleArray: return DoubleArray::New();
        // VTK promotes non-real arrays to float before interpolation so that
        // fractional weighted values are not truncated by integral storage.
        default: return FloatArray::New();
    }
}

double SquaredDistance(const Point& lhs, const Point& rhs) {
    return (lhs - rhs).squaredLength();
}
}

PointLineInterpolatorFilter::PointLineInterpolatorFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void PointLineInterpolatorFilter::SetValidPointsMaskArrayName(const std::string& name) {
    if (!name.empty()) m_ValidPointsMaskArrayName = name;
}

bool PointLineInterpolatorFilter::Execute() {
    m_Output = nullptr;
    SetOutput(nullptr);

    auto input = GetInput(0);
    if (!input || !input->GetPoints() || input->GetPoints()->GetNumberOfPoints() == 0) {
        igDebug("PointLineInterpolatorFilter requires input data with points.");
        return false;
    }
    if (m_Resolution < 1) {
        igDebug("PointLineInterpolatorFilter requires Resolution >= 1.");
        return false;
    }
    if (m_KernelType != VORONOI && m_KernelFootprint == RADIUS && m_Radius <= 0.0) {
        igDebug("PointLineInterpolatorFilter requires Radius > 0 for a radius footprint.");
        return false;
    }
    if (m_KernelType != VORONOI && m_KernelFootprint == N_CLOSEST && m_NumberOfPoints < 1) {
        igDebug("PointLineInterpolatorFilter requires NumberOfPoints >= 1 for an N-closest footprint.");
        return false;
    }
    if (m_KernelType == GAUSSIAN && m_Sharpness < 0.0) {
        igDebug("PointLineInterpolatorFilter requires non-negative Gaussian sharpness.");
        return false;
    }
    if (m_KernelType == SHEPARD && m_PowerParameter <= 0.0) {
        igDebug("PointLineInterpolatorFilter requires a positive Shepard power parameter.");
        return false;
    }

    auto sourcePoints = input->GetPoints();
    const IGsize sourcePointCount = sourcePoints->GetNumberOfPoints();
    const IGsize samplePointCount = static_cast<IGsize>(m_Resolution) + 1;
    PointFinder::Pointer pointFinder{};
    if (m_KernelType == VORONOI || m_NullPointsStrategy == CLOSEST_POINT) {
        pointFinder = PointFinder::New();
        pointFinder->SetPoints(sourcePoints);
        pointFinder->Initialize();
    }

    auto output = UnstructuredMesh::New();
    output->SetName(input->GetName() + "_PointLineInterpolator");
    auto outputPoints = Points::New();
    outputPoints->Reserve(samplePointCount);
    auto outputCells = CellArray::New();
    auto outputTypes = UnsignedIntArray::New();
    outputCells->Reserve(static_cast<IGsize>(m_Resolution) * 2);
    outputTypes->Reserve(m_Resolution);

    for (int i = 0; i <= m_Resolution; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(m_Resolution);
        outputPoints->AddPoint(m_Point1 * (1.0 - t) + m_Point2 * t);
        if (i > 0) {
            const igIndex line[2]{static_cast<igIndex>(i - 1), static_cast<igIndex>(i)};
            outputCells->AddCellIds(line, 2);
            outputTypes->AddValue(IG_LINE);
        }
    }
    output->SetPoints(outputPoints);
    output->SetCells(outputCells, outputTypes);

    struct OutputAttribute {
        AttributeSet::Attribute source;
        ArrayObject::Pointer output;
    };
    std::vector<OutputAttribute> outputAttributes;
    auto inputAttributes = input->GetAttributeSet();
    auto outputAttributeSet = output->GetAttributeSet();
    if (inputAttributes) {
        outputAttributes.reserve(inputAttributes->GetNumberOfAttributes());
        for (IGsize attributeId = 0; attributeId < inputAttributes->GetNumberOfAttributes(); ++attributeId) {
            auto& attribute = inputAttributes->GetAttribute(attributeId);
            if (attribute.IsDeleted() || attribute.attachmentType != IG_POINT || !attribute.pointer ||
                attribute.pointer->GetNumberOfElements() != sourcePointCount) {
                continue;
            }
            auto array = NewArrayLike(attribute.pointer.get());
            if (!array) continue;
            array->SetName(attribute.pointer->GetName());
            array->SetDimension(attribute.pointer->GetDimension());
            array->Resize(samplePointCount);
            outputAttributes.push_back({attribute, array});
        }
    }

    UnsignedCharArray::Pointer validMask{};
    if (m_NullPointsStrategy == MASK_POINTS) {
        validMask = UnsignedCharArray::New();
        validMask->SetName(m_ValidPointsMaskArrayName);
        validMask->SetDimension(1);
        validMask->Resize(samplePointCount);
    }

    std::vector<std::pair<double, igIndex>> distances;
    distances.reserve(sourcePointCount);
    std::vector<igIndex> basisIds;
    std::vector<double> weights;

    for (IGsize sampleId = 0; sampleId < samplePointCount; ++sampleId) {
        const Point& sample = outputPoints->GetPoint(sampleId);
        basisIds.clear();
        if (m_KernelType == VORONOI) {
            basisIds.push_back(pointFinder->FindClosestPoint(sample));
        } else {
            distances.clear();
            for (IGsize sourceId = 0; sourceId < sourcePointCount; ++sourceId) {
                distances.emplace_back(SquaredDistance(sample, sourcePoints->GetPoint(sourceId)),
                                       static_cast<igIndex>(sourceId));
            }
            std::sort(distances.begin(), distances.end(), [](const auto& lhs, const auto& rhs) {
                if (lhs.first != rhs.first) return lhs.first < rhs.first;
                return lhs.second < rhs.second;
            });
            if (m_KernelFootprint == RADIUS) {
                const double radius2 = m_Radius * m_Radius;
                for (const auto& distance : distances) {
                    if (distance.first > radius2) break;
                    basisIds.push_back(distance.second);
                }
            } else {
                const IGsize count = std::min<IGsize>(m_NumberOfPoints, distances.size());
                for (IGsize i = 0; i < count; ++i) basisIds.push_back(distances[i].second);
            }
        }

        const bool hasNeighborhood = !basisIds.empty();
        if (!hasNeighborhood && m_NullPointsStrategy == CLOSEST_POINT) {
            basisIds.push_back(pointFinder->FindClosestPoint(sample));
        }
        if (validMask) validMask->SetValue(sampleId, hasNeighborhood ? 1.0 : 0.0);

        weights.assign(basisIds.size(), 0.0);
        if (!basisIds.empty()) {
            igIndex exactId = -1;
            for (igIndex sourceId : basisIds) {
                if (SquaredDistance(sample, sourcePoints->GetPoint(sourceId)) <= ExactHitTolerance) {
                    exactId = sourceId;
                    break;
                }
            }
            if (exactId >= 0) {
                basisIds.assign(1, exactId);
                weights.assign(1, 1.0);
            } else if (m_KernelType == VORONOI || (!hasNeighborhood && m_NullPointsStrategy == CLOSEST_POINT)) {
                const igIndex nearestId = basisIds.front();
                basisIds.assign(1, nearestId);
                weights.assign(1, 1.0);
            } else {
                double weightSum = 0.0;
                for (IGsize i = 0; i < basisIds.size(); ++i) {
                    const double distance2 = SquaredDistance(sample, sourcePoints->GetPoint(basisIds[i]));
                    if (m_KernelType == GAUSSIAN) {
                        const double factor = m_Sharpness / m_Radius;
                        weights[i] = std::exp(-(factor * factor) * distance2);
                    } else {
                        weights[i] = 1.0 / std::pow(std::sqrt(distance2), m_PowerParameter);
                    }
                    weightSum += weights[i];
                }
                if (weightSum > 0.0) {
                    for (double& weight : weights) weight /= weightSum;
                }
            }
        }

        for (auto& attribute : outputAttributes) {
            const int dimension = attribute.source.pointer->GetDimension();
            for (int component = 0; component < dimension; ++component) {
                double value = m_NullValue;
                if (!basisIds.empty()) {
                    value = 0.0;
                    for (IGsize basisId = 0; basisId < basisIds.size(); ++basisId) {
                        value += weights[basisId] * attribute.source.pointer->GetValue(
                                                            static_cast<IGsize>(basisIds[basisId]) * dimension + component);
                    }
                }
                attribute.output->SetValue(sampleId * dimension + component, value);
            }
        }
    }

    for (auto& attribute : outputAttributes) {
        outputAttributeSet->AddAttribute(attribute.source.type, IG_POINT, attribute.output);
    }
    if (validMask) outputAttributeSet->AddAttribute(IG_SCALAR, IG_POINT, validMask);

    output->Modified();
    m_Output = output;
    SetOutput(0, output);
    return true;
}

IGAME_NAMESPACE_END
