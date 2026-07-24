#ifndef iGameDataCodecFeaturePreparedSurfaceAttributes_h
#define iGameDataCodecFeaturePreparedSurfaceAttributes_h

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"
#include "iGameScalarsToColors.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <memory>
#include <vector>

namespace iGame::datacodec_test {

[[nodiscard]] inline bool NearlyEqual(
    const double left,
    const double right,
    const double tolerance = 1e-9) {
    return std::abs(left - right) <= tolerance;
}

[[nodiscard]] inline FloatArray::Pointer BuildScalarArray(
    const char* name,
    const std::vector<float>& values) {
    auto array = FloatArray::New();
    array->SetName(name);
    array->SetDimension(1);
    array->Reserve(static_cast<IGsize>(values.size()));
    for (const auto value : values) { array->AddValue(value); }
    return array;
}

[[nodiscard]] inline bool TestRobustRangeCollapseRecovery() {
    std::vector<float> sparseValues(995u, 0.0f);
    sparseValues.insert(sparseValues.end(), {1.0f, 2.0f, 3.0f, 4.0f, 5.0f});

    auto mapper = ScalarsToColors::New();
    mapper->InitRangeRobust(BuildScalarArray("sparse_non_zero", sparseValues), 0);
    const auto sparseRange = mapper->GetRange();
    if (!NearlyEqual(sparseRange[0], 1.0) || !NearlyEqual(sparseRange[1], 5.0)) {
        std::cerr << "robust range did not recover the non-zero distribution\n";
        return false;
    }

    mapper->InitRangeRobust(BuildScalarArray("constant", std::vector<float>(128u, 42.0f)), 0);
    const auto constantRange = mapper->GetRange();
    if (!(constantRange[0] < 42.0 && constantRange[1] > 42.0)) {
        std::cerr << "constant robust range did not receive symmetric padding\n";
        return false;
    }
    return true;
}

[[nodiscard]] inline bool TestPreparedSurfacePointAndCellAttributes() {
    auto source = UnstructuredMesh::New();
    auto sourcePoints = Points::New();
    sourcePoints->AddPoint(0.0f, 0.0f, 0.0f);
    sourcePoints->AddPoint(1.0f, 0.0f, 0.0f);
    sourcePoints->AddPoint(0.0f, 1.0f, 0.0f);
    sourcePoints->AddPoint(0.0f, 0.0f, 1.0f);
    source->SetPoints(sourcePoints);

    auto sourceCells = CellArray::New();
    const igIndex tetra[] = {0, 1, 2, 3};
    sourceCells->AddCellIds(tetra, 4);
    sourceCells->AddCellIds(tetra, 4);
    auto sourceTypes = UnsignedIntArray::New();
    sourceTypes->AddValue(IG_TETRA);
    sourceTypes->AddValue(IG_TETRA);
    source->SetCells(sourceCells, sourceTypes);

    auto surface = SurfaceMesh::New();
    auto surfacePoints = Points::New();
    surfacePoints->AddPoint(1.0f, 0.0f, 0.0f);
    surfacePoints->AddPoint(0.0f, 1.0f, 0.0f);
    surfacePoints->AddPoint(0.0f, 0.0f, 0.0f);
    surface->SetPoints(surfacePoints);
    auto surfaceFaces = CellArray::New();
    const igIndex face0[] = {0, 1, 2};
    const igIndex face1[] = {2, 1, 0};
    surfaceFaces->AddCellIds(face0, 3);
    surfaceFaces->AddCellIds(face1, 3);
    surface->SetFaces(surfaceFaces);
    surface->SetAttributeSet(AttributeSet::New());

    auto pointMap = FlatArray<igIndex>::New();
    pointMap->SetDimension(1);
    pointMap->Resize(4u);
    pointMap->SetValue(0u, 2.0);
    pointMap->SetValue(1u, 0.0);
    pointMap->SetValue(2u, 1.0);
    pointMap->SetValue(3u, -1.0);
    auto faceToCellMap = std::make_shared<std::vector<igIndex>>(
        std::initializer_list<igIndex>{1, 0});
    source->SetPreparedSurfaceMesh(surface, pointMap, faceToCellMap);

    auto lateAttributes = AttributeSet::New();
    lateAttributes->AddAttribute(
        IG_SCALAR,
        IG_POINT,
        BuildScalarArray("late_point", {11.0f, 22.0f, 33.0f, 44.0f}));
    lateAttributes->AddAttribute(
        IG_SCALAR,
        IG_CELL,
        BuildScalarArray("late_cell", {100.0f, 200.0f}));
    source->SetAttributeSet(lateAttributes);

    if (!source->ViewCloudPicture(nullptr, 0, 0, false)) {
        std::cerr << "prepared surface point attribute could not be selected\n";
        return false;
    }
    source->ConvertToDrawableData();

    auto* mappedAttributes = surface->GetAttributeSet();
    if (mappedAttributes == nullptr || mappedAttributes->GetNumberOfAttributes() != 2u) {
        std::cerr << "prepared surface did not receive both late attributes\n";
        return false;
    }
    const auto& mappedPoint = mappedAttributes->GetAttribute(0);
    const auto& mappedCell = mappedAttributes->GetAttribute(1);
    if (mappedPoint.attachmentType != IG_POINT ||
        mappedPoint.pointer == nullptr ||
        mappedPoint.pointer->GetNumberOfElements() != 3u ||
        !NearlyEqual(mappedPoint.pointer->GetValue(0u), 22.0) ||
        !NearlyEqual(mappedPoint.pointer->GetValue(1u), 33.0) ||
        !NearlyEqual(mappedPoint.pointer->GetValue(2u), 11.0)) {
        std::cerr << "prepared surface point attribute mapping is incorrect\n";
        return false;
    }
    if (mappedCell.attachmentType != IG_CELL ||
        mappedCell.pointer == nullptr ||
        mappedCell.pointer->GetNumberOfElements() != 2u ||
        !NearlyEqual(mappedCell.pointer->GetValue(0u), 200.0) ||
        !NearlyEqual(mappedCell.pointer->GetValue(1u), 100.0)) {
        std::cerr << "prepared surface cell attribute mapping is incorrect\n";
        return false;
    }

    surface->ConvertToDrawableData();
    if (surface->IsActiveColorBufferCellBased() ||
        surface->GetActiveColorBufferElementCount() == 0u ||
        surface->GetActiveColorBufferUpdateId() == 0u) {
        std::cerr << "prepared surface point color buffer diagnostics are invalid\n";
        return false;
    }

    if (!source->ViewCloudPicture(nullptr, 1, 0, false)) {
        std::cerr << "prepared surface cell attribute could not be selected\n";
        return false;
    }
    source->ConvertToDrawableData();
    surface->ConvertToDrawableData();
    if (!surface->IsActiveColorBufferCellBased() ||
        surface->GetActiveColorBufferElementCount() == 0u ||
        surface->GetActiveColorBufferUpdateId() == 0u) {
        std::cerr << "prepared surface cell color buffer diagnostics are invalid\n";
        return false;
    }
    return true;
}

[[nodiscard]] inline int RunDataCodecFeaturePreparedSurfaceAttributes() {
    if (!TestRobustRangeCollapseRecovery()) { return 1; }
    if (!TestPreparedSurfacePointAndCellAttributes()) { return 1; }
    return 0;
}

}

#endif
