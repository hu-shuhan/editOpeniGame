#include "iGameMaskPointsFilter.h"
#include "Quadratic/Base/iGameQuadraticVolume.h"
#include "iGameCell.h"
#include "iGameFlatArray.h"
#include "iGamePointFinder.h"
#include "iGameVolume.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace
{

template<typename ArrayType>
ArrayObject::Pointer CopySelectedArray(const ArrayObject::Pointer& inputArray,
                                       const std::vector<igIndex>& selectedIds) {
    auto inArray = DynamicCast<ArrayType>(inputArray);
    if (inArray.IsNull()) { return nullptr; }

    auto outArray = ArrayType::New();
    outArray->SetName(inArray->GetName());

    const int dimension = inArray->GetDimension();
    outArray->SetDimension(dimension);
    outArray->Resize(static_cast<IGsize>(selectedIds.size()));

    for (IGsize newId = 0; newId < static_cast<IGsize>(selectedIds.size()); ++newId) {
        const igIndex oldId = selectedIds[static_cast<size_t>(newId)];
        const auto* src = inArray->RawPointer(oldId);
        auto* dst = outArray->RawPointer(newId);
        std::copy(src, src + dimension, dst);
    }

    return outArray;
}

void SpatiallyStratifiedSample(const UnstructuredMesh::Pointer& input, std::vector<igIndex>& ids, size_t start,
                               size_t end, size_t sampleSize, int depth, std::mt19937& generator) {
    if (end <= start || sampleSize == 0) { return; }

    const size_t count = end - start;
    if (sampleSize >= count) { return; }

    if (sampleSize == 1) {
        std::uniform_int_distribution<size_t> distribution(start, end - 1);
        std::swap(ids[start], ids[distribution(generator)]);
        return;
    }

    size_t half = start + count / 2;
    int bigger = 0;

    if (count % 2 != 0) {
        std::uniform_int_distribution<int> distribution(0, 1);

        if (distribution(generator) != 0) {
            bigger = 1;
            ++half;
        } else {
            bigger = 2;
        }
    }

    const int axis = depth % 3;

    std::nth_element(ids.begin() + static_cast<std::ptrdiff_t>(start), ids.begin() + static_cast<std::ptrdiff_t>(half),
                     ids.begin() + static_cast<std::ptrdiff_t>(end),
                     [&](igIndex a, igIndex b) { return input->GetPoint(a)[axis] < input->GetPoint(b)[axis]; });

    size_t leftSize = 0;
    size_t rightSize = 0;

    if (sampleSize % 2 != 0) {
        if (bigger == 1) {
            leftSize = sampleSize / 2 + 1;
            rightSize = sampleSize / 2;
        } else if (bigger == 2) {
            leftSize = sampleSize / 2;
            rightSize = sampleSize / 2 + 1;
        } else {
            std::uniform_int_distribution<int> distribution(0, 1);

            if (distribution(generator) != 0) {
                leftSize = sampleSize / 2 + 1;
                rightSize = sampleSize / 2;
            } else {
                leftSize = sampleSize / 2;
                rightSize = sampleSize / 2 + 1;
            }
        }
    } else {
        leftSize = sampleSize / 2;
        rightSize = sampleSize / 2;
    }

    SpatiallyStratifiedSample(input, ids, start, half, leftSize, depth + 1, generator);
    SpatiallyStratifiedSample(input, ids, half, end, rightSize, depth + 1, generator);

    for (size_t i = 0; i < rightSize; ++i) { std::swap(ids[start + leftSize + i], ids[half + i]); }
}

double GetNearestPointRadius(const BoundingBox& bounds, IGsize maximumNumberOfPoints) {
    const double dx = bounds.max[0] - bounds.min[0];
    const double dy = bounds.max[1] - bounds.min[1];
    const double dz = bounds.max[2] - bounds.min[2];

    const int dimension = (dx > 0.0 && dy > 0.0 && dz > 0.0) ? 3 : 2;
    const double diagonal = std::sqrt(dx * dx + dy * dy + dz * dz);
    const double volume = std::pow(diagonal, dimension);

    if (volume > 0.0 && maximumNumberOfPoints > 0) {
        const double volumePerPoint = volume / static_cast<double>(maximumNumberOfPoints);
        const double delta = std::pow(volumePerPoint, 1.0 / dimension);
        return delta * 0.5;
    }

    return 0.0001;
}

double TriangleArea(const Point& a, const Point& b, const Point& c) {
    const double ux = b[0] - a[0];
    const double uy = b[1] - a[1];
    const double uz = b[2] - a[2];

    const double vx = c[0] - a[0];
    const double vy = c[1] - a[1];
    const double vz = c[2] - a[2];

    const double cx = uy * vz - uz * vy;
    const double cy = uz * vx - ux * vz;
    const double cz = ux * vy - uy * vx;

    return 0.5 * std::sqrt(cx * cx + cy * cy + cz * cz);
}

double TetraVolume(const Point& a, const Point& b, const Point& c, const Point& d) {
    const double ax = a[0] - d[0];
    const double ay = a[1] - d[1];
    const double az = a[2] - d[2];

    const double bx = b[0] - d[0];
    const double by = b[1] - d[1];
    const double bz = b[2] - d[2];

    const double cx = c[0] - d[0];
    const double cy = c[1] - d[1];
    const double cz = c[2] - d[2];

    const double crossX = by * cz - bz * cy;
    const double crossY = bz * cx - bx * cz;
    const double crossZ = bx * cy - by * cx;

    const double triple = ax * crossX + ay * crossY + az * crossZ;

    return std::abs(triple) / 6.0;
}

double GetSurfaceCellArea(const UnstructuredMesh::Pointer& input, IGsize cellId) {
    const IGenum cellType = input->GetCellType(cellId);

    if (Cell::GetCellDimension(cellType) != 2) { return 0.0; }

    auto cell = input->GetCell(cellId);
    if (cell == nullptr) { return 0.0; }

    const int sizeHint = cell->GetNumberOfPoints();
    if (sizeHint < 3) { return 0.0; }

    std::vector<igIndex> ids(static_cast<size_t>(sizeHint));
    const int size = input->GetCellPointIds(cellId, ids.data());

    if (size < 3) { return 0.0; }

    double area = 0.0;

    if (cellType == IG_QUADRATIC_TRIANGLE || cellType == IG_QUADRATIC_QUAD) {
        const int trueSize = size / 2;

        if (trueSize < 3) { return 0.0; }

        area += TriangleArea(input->GetPoint(ids[0]), input->GetPoint(ids[trueSize]),
                             input->GetPoint(ids[trueSize * 2 - 1]));

        for (int j = 1; j < trueSize; ++j) {
            area += TriangleArea(input->GetPoint(ids[j]), input->GetPoint(ids[j + trueSize]),
                                 input->GetPoint(ids[j + trueSize - 1]));
        }

        for (int j = 2; j < trueSize; ++j) {
            area += TriangleArea(input->GetPoint(ids[trueSize]), input->GetPoint(ids[trueSize + j - 1]),
                                 input->GetPoint(ids[trueSize + j]));
        }

        return area;
    }

    for (int i = 1; i < size - 1; ++i) {
        area += TriangleArea(input->GetPoint(ids[0]), input->GetPoint(ids[i]), input->GetPoint(ids[i + 1]));
    }

    return area;
}

Point GetCellCenter(const UnstructuredMesh::Pointer& input, const std::vector<igIndex>& cellIds,
                    int numberOfCornerPoints) {
    Point center(0.0f, 0.0f, 0.0f);

    if (numberOfCornerPoints <= 0) { return center; }

    for (int i = 0; i < numberOfCornerPoints; ++i) {
        const Point& p = input->GetPoint(cellIds[static_cast<size_t>(i)]);

        center[0] += p[0];
        center[1] += p[1];
        center[2] += p[2];
    }

    center[0] /= numberOfCornerPoints;
    center[1] /= numberOfCornerPoints;
    center[2] /= numberOfCornerPoints;

    return center;
}

int GetNumberOfCornerPoints(IGenum cellType, int totalNumberOfPoints) {
    switch (cellType) {
        case IG_QUADRATIC_TETRA:
            return 4;

        case IG_QUADRATIC_HEXAHEDRON:
            return 8;

        case IG_QUADRATIC_PRISM:
            return 6;

        case IG_QUADRATIC_PYRAMID:
            return 5;

        default:
            return totalNumberOfPoints;
    }
}

double AddLinearFaceVolume(const UnstructuredMesh::Pointer& input, const std::vector<igIndex>& cellIds,
                           const igIndex* faceIds, int faceSize, const Point& center) {
    if (faceIds == nullptr || faceSize < 3) { return 0.0; }

    double volume = 0.0;

    for (int i = 1; i < faceSize - 1; ++i) {
        const igIndex id0 = cellIds[static_cast<size_t>(faceIds[0])];
        const igIndex id1 = cellIds[static_cast<size_t>(faceIds[i])];
        const igIndex id2 = cellIds[static_cast<size_t>(faceIds[i + 1])];

        volume += TetraVolume(input->GetPoint(id0), input->GetPoint(id1), input->GetPoint(id2), center);
    }

    return volume;
}

double AddQuadraticFaceVolume(const UnstructuredMesh::Pointer& input, const std::vector<igIndex>& cellIds,
                              const igIndex* faceIds, int faceSize, const Point& center) {
    if (faceIds == nullptr || faceSize < 6) { return 0.0; }

    const int trueSize = faceSize / 2;

    if (trueSize < 3) { return 0.0; }

    double volume = 0.0;

    auto AddTriangle = [&](int a, int b, int c) {
        const igIndex id0 = cellIds[static_cast<size_t>(faceIds[a])];
        const igIndex id1 = cellIds[static_cast<size_t>(faceIds[b])];
        const igIndex id2 = cellIds[static_cast<size_t>(faceIds[c])];

        volume += TetraVolume(input->GetPoint(id0), input->GetPoint(id1), input->GetPoint(id2), center);
    };

    AddTriangle(0, trueSize, trueSize * 2 - 1);

    for (int j = 1; j < trueSize; ++j) { AddTriangle(j, j + trueSize, j + trueSize - 1); }

    for (int j = 2; j < trueSize; ++j) { AddTriangle(trueSize, trueSize + j - 1, trueSize + j); }

    return volume;
}

double GetVolumeCellVolume(const UnstructuredMesh::Pointer& input, IGsize cellId) {
    const IGenum cellType = input->GetCellType(cellId);

    if (Cell::GetCellDimension(cellType) != 3) { return 0.0; }

    auto cell = input->GetCell(cellId);
    if (cell == nullptr) { return 0.0; }

    const int sizeHint = cell->GetNumberOfPoints();
    if (sizeHint < 4) { return 0.0; }

    std::vector<igIndex> cellIds(static_cast<size_t>(sizeHint));

    const int numberOfCellPoints = input->GetCellPointIds(cellId, cellIds.data());

    if (numberOfCellPoints < 4) { return 0.0; }

    const int numberOfCornerPoints = GetNumberOfCornerPoints(cellType, numberOfCellPoints);
    const Point center = GetCellCenter(input, cellIds, numberOfCornerPoints);

    double volume = 0.0;

    if (cellType == IG_QUADRATIC_TETRA || cellType == IG_QUADRATIC_HEXAHEDRON || cellType == IG_QUADRATIC_PRISM ||
        cellType == IG_QUADRATIC_PYRAMID) {

        auto* quadraticVolume = dynamic_cast<QuadraticVolume*>(cell);

        if (quadraticVolume == nullptr) { return 0.0; }

        const int numberOfFaces = quadraticVolume->GetNumberOfFaces();

        for (int faceId = 0; faceId < numberOfFaces; ++faceId) {
            const igIndex* faceIds = nullptr;
            const int faceSize = quadraticVolume->GetFacePointIds(faceId, faceIds);

            volume += AddQuadraticFaceVolume(input, cellIds, faceIds, faceSize, center);
        }

        return volume;
    }

    auto* linearVolume = dynamic_cast<Volume*>(cell);

    if (linearVolume == nullptr) { return 0.0; }

    const int numberOfFaces = linearVolume->GetNumberOfFaces();

    for (int faceId = 0; faceId < numberOfFaces; ++faceId) {
        const igIndex* faceIds = nullptr;
        const int faceSize = linearVolume->GetFacePointIds(faceId, faceIds);

        volume += AddLinearFaceVolume(input, cellIds, faceIds, faceSize, center);
    }

    return volume;
}

} // namespace

MaskPointsFilter::MaskPointsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void MaskPointsFilter::SetOnRatio(int ratio) { m_OnRatio = ratio; }

int MaskPointsFilter::GetOnRatio() const { return m_OnRatio; }

void MaskPointsFilter::SetMaximumNumberOfPoints(IGsize maxPoints) { m_MaximumNumberOfPoints = maxPoints; }

IGsize MaskPointsFilter::GetMaximumNumberOfPoints() const { return m_MaximumNumberOfPoints; }

void MaskPointsFilter::SetProportionalMaximumNumberOfPoints(bool enabled) {
    m_ProportionalMaximumNumberOfPoints = enabled;
}

bool MaskPointsFilter::GetProportionalMaximumNumberOfPoints() const { return m_ProportionalMaximumNumberOfPoints; }

void MaskPointsFilter::SetOffset(IGsize offset) { m_Offset = offset; }

IGsize MaskPointsFilter::GetOffset() const { return m_Offset; }

void MaskPointsFilter::SetRandomMode(bool enabled) { m_RandomMode = enabled; }

bool MaskPointsFilter::GetRandomMode() const { return m_RandomMode; }

void MaskPointsFilter::SetRandomModeType(int mode) { m_RandomModeType = mode; }

int MaskPointsFilter::GetRandomModeType() const { return m_RandomModeType; }

void MaskPointsFilter::SetRandomSeed(unsigned int seed) { m_RandomSeed = seed; }

unsigned int MaskPointsFilter::GetRandomSeed() const { return m_RandomSeed; }

void MaskPointsFilter::SetGenerateVertices(bool enabled) { m_GenerateVertices = enabled; }

bool MaskPointsFilter::GetGenerateVertices() const { return m_GenerateVertices; }

void MaskPointsFilter::SetSingleVertexPerCell(bool enabled) { m_SingleVertexPerCell = enabled; }

bool MaskPointsFilter::GetSingleVertexPerCell() const { return m_SingleVertexPerCell; }

bool MaskPointsFilter::Execute() {
    auto input = DynamicCast<UnstructuredMesh>(GetInput(0));

    if (input.IsNull()) { return false; }
    if (m_OnRatio <= 0) { return false; }
    if (m_GenerateVertices && !m_SingleVertexPerCell) { return false; }

    const IGsize numberOfPoints = input->GetNumberOfPoints();

    auto output = UnstructuredMesh::New();
    std::vector<igIndex> selectedIds;

    // Regular sampling
    if (!m_RandomMode) {
        for (IGsize oldId = m_Offset; oldId < numberOfPoints; oldId += m_OnRatio) {
            selectedIds.push_back(static_cast<igIndex>(oldId));

            if (m_MaximumNumberOfPoints > 0 && selectedIds.size() >= static_cast<size_t>(m_MaximumNumberOfPoints)) {
                break;
            }
        }
    }

    // Randomized id strides
    else if (m_RandomModeType == RANDOMIZED_ID_STRIDES) {
        if (m_Offset < numberOfPoints) {
            std::mt19937 generator(m_RandomSeed);
            std::uniform_real_distribution<double> distribution(0.0, 1.0);

            double cap = 2.0 * m_OnRatio - 1.0;

            if (m_MaximumNumberOfPoints > 0 &&
                static_cast<double>(numberOfPoints) / m_OnRatio > m_MaximumNumberOfPoints) {
                cap = 2.0 * static_cast<double>(numberOfPoints) / m_MaximumNumberOfPoints - 1.0;
            }

            IGsize oldId = m_Offset;

            while (oldId < numberOfPoints) {
                selectedIds.push_back(static_cast<igIndex>(oldId));

                if (m_MaximumNumberOfPoints > 0 && selectedIds.size() >= static_cast<size_t>(m_MaximumNumberOfPoints)) {
                    break;
                }

                const IGsize stride = 1 + static_cast<IGsize>(distribution(generator) * cap);
                oldId += stride;
            }
        }
    }

    // Random sampling
    else if (m_RandomModeType == RANDOM_SAMPLING) {
        std::vector<igIndex> allIds;
        allIds.reserve(static_cast<size_t>(numberOfPoints));

        for (IGsize oldId = 0; oldId < numberOfPoints; ++oldId) { allIds.push_back(static_cast<igIndex>(oldId)); }

        std::mt19937 generator(m_RandomSeed);
        std::shuffle(allIds.begin(), allIds.end(), generator);

        IGsize targetCount = numberOfPoints;

        if (m_MaximumNumberOfPoints > 0 && targetCount > m_MaximumNumberOfPoints) {
            targetCount = m_MaximumNumberOfPoints;
        }

        selectedIds.assign(allIds.begin(), allIds.begin() + static_cast<size_t>(targetCount));
    }

    // Spatially stratified
    else if (m_RandomModeType == SPATIALLY_STRATIFIED) {
        std::vector<igIndex> allIds;
        allIds.reserve(static_cast<size_t>(numberOfPoints));

        for (IGsize oldId = 0; oldId < numberOfPoints; ++oldId) { allIds.push_back(static_cast<igIndex>(oldId)); }

        IGsize targetCount = numberOfPoints;

        if (m_MaximumNumberOfPoints > 0 && targetCount > m_MaximumNumberOfPoints) {
            targetCount = m_MaximumNumberOfPoints;
        }

        if (targetCount > 0) {
            std::mt19937 generator(m_RandomSeed);

            SpatiallyStratifiedSample(input, allIds, 0, allIds.size(), static_cast<size_t>(targetCount), 0, generator);

            selectedIds.assign(allIds.begin(), allIds.begin() + static_cast<size_t>(targetCount));
        }
    }

    // Uniform spatial bounds
    else if (m_RandomModeType == UNIFORM_SPATIAL_BOUNDS) {
        if (numberOfPoints > 0) {
            IGsize targetCount = numberOfPoints;

            if (m_MaximumNumberOfPoints > 0 && targetCount > m_MaximumNumberOfPoints) {
                targetCount = m_MaximumNumberOfPoints;
            }

            const auto& bounds = input->GetBoundingBox();
            const double nearestPointRadius = GetNearestPointRadius(bounds, targetCount);
            const double radiusSquared = nearestPointRadius * nearestPointRadius;

            auto locatorPoints = Points::New();

            for (IGsize oldId = 0; oldId < numberOfPoints; ++oldId) { locatorPoints->AddPoint(input->GetPoint(oldId)); }

            auto finder = PointFinder::New();
            finder->SetPoints(locatorPoints);
            finder->Initialize();

            std::mt19937 generator(m_RandomSeed);
            std::vector<bool> maskedPoints(static_cast<size_t>(numberOfPoints), false);

            std::uniform_real_distribution<double> distributionX(bounds.min[0], bounds.max[0]);
            std::uniform_real_distribution<double> distributionY(bounds.min[1], bounds.max[1]);
            std::uniform_real_distribution<double> distributionZ(bounds.min[2], bounds.max[2]);

            for (IGsize i = 0; i < targetCount; ++i) {
                Vector3d randomPosition;

                randomPosition[0] = distributionX(generator);
                randomPosition[1] = distributionY(generator);
                randomPosition[2] = distributionZ(generator);

                double minDistSquared = 0.0;

                const igIndex closestId = finder->FindClosestPoint(randomPosition, minDistSquared);

                if (closestId >= 0 && static_cast<IGsize>(closestId) < numberOfPoints &&
                    minDistSquared <= radiusSquared && !maskedPoints[static_cast<size_t>(closestId)]) {
                    selectedIds.push_back(closestId);
                    maskedPoints[static_cast<size_t>(closestId)] = true;
                }
            }
        }
    }

    // Uniform spatial surface
    else if (m_RandomModeType == UNIFORM_SPATIAL_SURFACE) {
        const IGsize numberOfCells = input->GetNumberOfCells();

        IGsize targetCount = numberOfPoints;

        if (m_MaximumNumberOfPoints > 0 && targetCount > m_MaximumNumberOfPoints) {
            targetCount = m_MaximumNumberOfPoints;
        }

        std::vector<double> cellContribs(static_cast<size_t>(numberOfCells), 0.0);

        double totalArea = 0.0;

        for (IGsize cellId = 0; cellId < numberOfCells; ++cellId) {
            const IGenum cellType = input->GetCellType(cellId);

            if (Cell::GetCellDimension(cellType) == 2) { totalArea += GetSurfaceCellArea(input, cellId); }

            cellContribs[static_cast<size_t>(cellId)] = totalArea;
        }

        if (totalArea > 0.0 && targetCount > 0) {
            std::vector<bool> maskedPoints(static_cast<size_t>(numberOfPoints), false);

            std::mt19937 generator(m_RandomSeed);

            std::uniform_real_distribution<double> distribution(0.0, totalArea);

            for (IGsize sampleId = 0; sampleId < targetCount; ++sampleId) {
                const double sample = distribution(generator);

                auto it = std::upper_bound(cellContribs.cbegin(), cellContribs.cend(), sample);

                if (it == cellContribs.cend()) { continue; }

                const IGsize cellId = static_cast<IGsize>(std::distance(cellContribs.cbegin(), it));

                auto cell = input->GetCell(cellId);

                if (cell == nullptr) { continue; }

                const int sizeHint = cell->GetNumberOfPoints();

                if (sizeHint <= 0) { continue; }

                std::vector<igIndex> cellIds(static_cast<size_t>(sizeHint));

                const int size = input->GetCellPointIds(cellId, cellIds.data());

                for (int i = 0; i < size; ++i) {
                    const igIndex pointId = cellIds[static_cast<size_t>(i)];

                    if (pointId < 0 || static_cast<IGsize>(pointId) >= numberOfPoints) { continue; }

                    if (!maskedPoints[static_cast<size_t>(pointId)]) {
                        selectedIds.push_back(pointId);
                        maskedPoints[static_cast<size_t>(pointId)] = true;
                        break;
                    }
                }
            }
        }
    }

    // Uniform spatial volume
    else if (m_RandomModeType == UNIFORM_SPATIAL_VOLUME) {
        const IGsize numberOfCells = input->GetNumberOfCells();

        IGsize targetCount = numberOfPoints;

        if (m_MaximumNumberOfPoints > 0 && targetCount > m_MaximumNumberOfPoints) {
            targetCount = m_MaximumNumberOfPoints;
        }

        std::vector<double> cellContribs(static_cast<size_t>(numberOfCells), 0.0);

        double totalVolume = 0.0;

        for (IGsize cellId = 0; cellId < numberOfCells; ++cellId) {
            const IGenum cellType = input->GetCellType(cellId);

            if (Cell::GetCellDimension(cellType) == 3) { totalVolume += GetVolumeCellVolume(input, cellId); }

            cellContribs[static_cast<size_t>(cellId)] = totalVolume;
        }

        if (totalVolume > 0.0 && targetCount > 0) {
            std::vector<bool> maskedPoints(static_cast<size_t>(numberOfPoints), false);

            std::mt19937 generator(m_RandomSeed);

            std::uniform_real_distribution<double> distribution(0.0, totalVolume);

            for (IGsize sampleId = 0; sampleId < targetCount; ++sampleId) {
                const double sample = distribution(generator);

                auto it = std::upper_bound(cellContribs.cbegin(), cellContribs.cend(), sample);

                if (it == cellContribs.cend()) { continue; }

                const IGsize cellId = static_cast<IGsize>(std::distance(cellContribs.cbegin(), it));

                auto cell = input->GetCell(cellId);

                if (cell == nullptr) { continue; }

                const int sizeHint = cell->GetNumberOfPoints();

                if (sizeHint <= 0) { continue; }

                std::vector<igIndex> cellIds(static_cast<size_t>(sizeHint));

                const int size = input->GetCellPointIds(cellId, cellIds.data());

                for (int i = 0; i < size; ++i) {
                    const igIndex pointId = cellIds[static_cast<size_t>(i)];

                    if (pointId < 0 || static_cast<IGsize>(pointId) >= numberOfPoints) { continue; }

                    if (!maskedPoints[static_cast<size_t>(pointId)]) {
                        selectedIds.push_back(pointId);
                        maskedPoints[static_cast<size_t>(pointId)] = true;
                        break;
                    }
                }
            }
        }
    }

    else {
        return false;
    }

    // Copy points
    for (igIndex oldId: selectedIds) {
        const Point& point = input->GetPoint(oldId);
        output->AddPoint(point);
    }

    // Generate vertex cells
    if (m_GenerateVertices) {
        for (IGsize i = 0; i < output->GetNumberOfPoints(); ++i) {
            igIndex cell[1] = {static_cast<igIndex>(i)};
            output->AddCell(cell, 1, IG_VERTEX);
        }
    }

    output->SetName(input->GetName());

    // Copy point attributes
    auto inData = input->GetAttributeSet();

    if (inData != nullptr) {
        auto outData = AttributeSet::New();
        auto inAllAttr = inData->GetAllAttributes();

        for (IGsize i = 0; i < inAllAttr->GetNumberOfElements(); ++i) {
            auto attr = inAllAttr->GetElement(i);

            if (attr.attachmentType != IG_POINT) { continue; }

            auto inArray = attr.pointer;
            ArrayObject::Pointer outArray;

            switch (inArray->GetArrayType()) {
                case IG_FloatArray:
                    outArray = CopySelectedArray<FloatArray>(inArray, selectedIds);
                    break;

                case IG_DoubleArray:
                    outArray = CopySelectedArray<DoubleArray>(inArray, selectedIds);
                    break;

                case IG_IntArray:
                    outArray = CopySelectedArray<IntArray>(inArray, selectedIds);
                    break;

                case IG_UnsignedIntArray:
                    outArray = CopySelectedArray<UnsignedIntArray>(inArray, selectedIds);
                    break;

                case IG_CharArray:
                    outArray = CopySelectedArray<CharArray>(inArray, selectedIds);
                    break;

                case IG_UnsignedCharArray:
                    outArray = CopySelectedArray<UnsignedCharArray>(inArray, selectedIds);
                    break;

                case IG_ShortArray:
                    outArray = CopySelectedArray<ShortArray>(inArray, selectedIds);
                    break;

                case IG_UnsignedShortArray:
                    outArray = CopySelectedArray<UnsignedShortArray>(inArray, selectedIds);
                    break;

                case IG_LongLongArray:
                    outArray = CopySelectedArray<LongLongArray>(inArray, selectedIds);
                    break;

                case IG_UnsignedLongLongArray:
                    outArray = CopySelectedArray<UnsignedLongLongArray>(inArray, selectedIds);
                    break;

                default:
                    continue;
            }

            if (outArray == nullptr) { continue; }

            outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
        }

        output->SetAttributeSet(outData);
    }

    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END