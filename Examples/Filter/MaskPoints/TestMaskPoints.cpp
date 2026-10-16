// Source: dayuwan77/igamevis at eccac729b57aeacbe9312d7d5189f6990bb4eebd.
// Integration gap: e7ec6571 imported the filter but omitted its example.
// Preserve the numerical/attribute checks below and reject invalid inputs;
// visual examples support --no-render so CI requires a real exit status.
// Integration commit: test: add examples for first-batch standard filters
// Find it: git log --diff-filter=A --format="%h %s" -- Examples/Filter/MaskPoints/TestMaskPoints.cpp
#include <MaskPoints/iGameMaskPointsFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFlatArray.h>
#include <iGameUnstructuredMesh.h>

#include <cmath>
#include <iostream>
#include <vector>

iGame::UnstructuredMesh::Pointer CreateTestMesh() {
    auto mesh = iGame::UnstructuredMesh::New();

    for (int i = 0; i < 10; ++i) { mesh->AddPoint(iGame::Point(static_cast<float>(i), 0.0f, 0.0f)); }

    auto scalarArray = iGame::FloatArray::New();
    scalarArray->SetName("TestScalar");
    scalarArray->SetDimension(1);
    scalarArray->Resize(10);

    for (int i = 0; i < 10; ++i) {
        double value[1] = {100.0 + static_cast<double>(i)};
        scalarArray->SetElement(i, value);
    }

    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, scalarArray);
    return mesh;
}

iGame::UnstructuredMesh::Pointer CreateSpatialTestMesh() {
    auto mesh = iGame::UnstructuredMesh::New();

    auto scalarArray = iGame::FloatArray::New();
    scalarArray->SetName("SpatialScalar");
    scalarArray->SetDimension(1);
    scalarArray->Resize(125);

    int id = 0;

    for (int z = 0; z < 5; ++z) {
        for (int y = 0; y < 5; ++y) {
            for (int x = 0; x < 5; ++x) {
                mesh->AddPoint(iGame::Point(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));

                double value[1] = {1000.0 + static_cast<double>(id)};
                scalarArray->SetElement(id, value);
                ++id;
            }
        }
    }

    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, scalarArray);
    return mesh;
}

iGame::UnstructuredMesh::Pointer CreateSurfaceTestMesh() {
    auto mesh = iGame::UnstructuredMesh::New();

    mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(2.0f, 1.0f, 0.0f));

    mesh->AddPoint(iGame::Point(10.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(11.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(12.0f, 0.0f, 0.0f));

    mesh->AddPoint(iGame::Point(20.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(21.0f, 0.0f, 0.0f));

    igIndex triangle1[3] = {0, 1, 2};
    igIndex triangle2[3] = {3, 4, 5};
    igIndex line[2] = {6, 7};

    mesh->AddCell(triangle1, 3, iGame::IG_TRIANGLE);
    mesh->AddCell(triangle2, 3, iGame::IG_TRIANGLE);
    mesh->AddCell(line, 2, iGame::IG_LINE);

    auto scalarArray = iGame::FloatArray::New();
    scalarArray->SetName("SurfaceScalar");
    scalarArray->SetDimension(1);
    scalarArray->Resize(8);

    for (int i = 0; i < 8; ++i) {
        double value[1] = {2000.0 + static_cast<double>(i)};
        scalarArray->SetElement(i, value);
    }

    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, scalarArray);
    return mesh;
}

iGame::UnstructuredMesh::Pointer CreateVolumeTestMesh() {
    auto mesh = iGame::UnstructuredMesh::New();

    // Valid tetrahedron
    mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
    mesh->AddPoint(iGame::Point(0.0f, 0.0f, 1.0f));

    // Degenerate tetrahedron
    mesh->AddPoint(iGame::Point(10.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(11.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(12.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(13.0f, 0.0f, 0.0f));

    // 2D triangle
    mesh->AddPoint(iGame::Point(20.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(21.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(20.0f, 1.0f, 0.0f));

    igIndex tetra1[4] = {0, 1, 2, 3};
    igIndex tetra2[4] = {4, 5, 6, 7};
    igIndex triangle[3] = {8, 9, 10};

    mesh->AddCell(tetra1, 4, iGame::IG_TETRA);
    mesh->AddCell(tetra2, 4, iGame::IG_TETRA);
    mesh->AddCell(triangle, 3, iGame::IG_TRIANGLE);

    auto scalarArray = iGame::FloatArray::New();
    scalarArray->SetName("VolumeScalar");
    scalarArray->SetDimension(1);
    scalarArray->Resize(11);

    for (int i = 0; i < 11; ++i) {
        double value[1] = {3000.0 + static_cast<double>(i)};
        scalarArray->SetElement(i, value);
    }

    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, scalarArray);
    return mesh;
}

bool CheckCoordinates(iGame::UnstructuredMesh::Pointer output, const std::vector<double>& expected) {
    if (output.IsNull()) { return false; }
    if (output->GetNumberOfPoints() != expected.size()) { return false; }

    for (IGsize i = 0; i < output->GetNumberOfPoints(); ++i) {
        const auto& point = output->GetPoint(i);
        if (point[0] != expected[static_cast<size_t>(i)]) { return false; }
    }

    return true;
}

bool CheckSameCoordinates(iGame::UnstructuredMesh::Pointer output1, iGame::UnstructuredMesh::Pointer output2) {
    if (output1.IsNull() || output2.IsNull()) { return false; }

    if (output1->GetNumberOfPoints() != output2->GetNumberOfPoints()) { return false; }

    for (IGsize i = 0; i < output1->GetNumberOfPoints(); ++i) {
        const auto& p1 = output1->GetPoint(i);
        const auto& p2 = output2->GetPoint(i);

        if (p1[0] != p2[0] || p1[1] != p2[1] || p1[2] != p2[2]) { return false; }
    }

    return true;
}

bool CheckAttributes(iGame::UnstructuredMesh::Pointer output, const std::vector<double>& expected) {
    if (output.IsNull()) { return false; }

    auto attrSet = output->GetAttributeSet();
    if (attrSet == nullptr) { return false; }

    auto allAttrs = attrSet->GetAllAttributes();

    for (IGsize i = 0; i < allAttrs->GetNumberOfElements(); ++i) {
        auto attr = allAttrs->GetElement(i);

        if (attr.attachmentType != IG_POINT) { continue; }

        auto array = attr.pointer;

        if (array->GetName() != "TestScalar") { continue; }
        if (array->GetNumberOfElements() != expected.size()) { return false; }

        double value[IGAME_CELL_MAX_SIZE] = {0};

        for (IGsize j = 0; j < static_cast<IGsize>(expected.size()); ++j) {
            array->GetElement(j, value);

            if (value[0] != expected[static_cast<size_t>(j)]) { return false; }
        }

        return true;
    }

    return false;
}

bool CheckAttributeMapping(iGame::UnstructuredMesh::Pointer output) {
    if (output.IsNull()) { return false; }

    auto attrSet = output->GetAttributeSet();
    if (attrSet == nullptr) { return false; }

    auto allAttrs = attrSet->GetAllAttributes();

    for (IGsize i = 0; i < allAttrs->GetNumberOfElements(); ++i) {
        auto attr = allAttrs->GetElement(i);

        if (attr.attachmentType != IG_POINT) { continue; }

        auto array = attr.pointer;

        if (array->GetName() != "TestScalar") { continue; }

        if (array->GetNumberOfElements() != output->GetNumberOfPoints()) { return false; }

        double value[IGAME_CELL_MAX_SIZE] = {0};

        for (IGsize j = 0; j < output->GetNumberOfPoints(); ++j) {
            array->GetElement(j, value);
            const auto& point = output->GetPoint(j);

            if (value[0] != 100.0 + point[0]) { return false; }
        }

        return true;
    }

    return false;
}

bool CheckSpatialAttributeMapping(iGame::UnstructuredMesh::Pointer output) {
    if (output.IsNull()) { return false; }

    auto attrSet = output->GetAttributeSet();
    if (attrSet == nullptr) { return false; }

    auto allAttrs = attrSet->GetAllAttributes();

    for (IGsize i = 0; i < allAttrs->GetNumberOfElements(); ++i) {
        auto attr = allAttrs->GetElement(i);

        if (attr.attachmentType != IG_POINT) { continue; }

        auto array = attr.pointer;

        if (array->GetName() != "SpatialScalar") { continue; }

        if (array->GetNumberOfElements() != output->GetNumberOfPoints()) { return false; }

        double value[IGAME_CELL_MAX_SIZE] = {0};

        for (IGsize j = 0; j < output->GetNumberOfPoints(); ++j) {
            const auto& point = output->GetPoint(j);

            const int x = static_cast<int>(std::round(point[0]));
            const int y = static_cast<int>(std::round(point[1]));
            const int z = static_cast<int>(std::round(point[2]));
            const int originalId = z * 25 + y * 5 + x;

            array->GetElement(j, value);

            if (value[0] != 1000.0 + originalId) { return false; }
        }

        return true;
    }

    return false;
}

bool TestOnRatio() {
    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, CreateTestMesh());
    filter->SetOnRatio(2);
    filter->SetOffset(0);
    filter->SetMaximumNumberOfPoints(0);
    filter->SetRandomMode(false);

    if (!filter->Execute()) { return false; }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    return CheckCoordinates(output, {0, 2, 4, 6, 8});
}

bool TestMaximumNumberOfPoints() {
    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, CreateTestMesh());
    filter->SetOnRatio(2);
    filter->SetOffset(0);
    filter->SetMaximumNumberOfPoints(3);
    filter->SetRandomMode(false);

    if (!filter->Execute()) { return false; }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    return CheckCoordinates(output, {0, 2, 4});
}

bool TestOffset() {
    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, CreateTestMesh());
    filter->SetOnRatio(3);
    filter->SetOffset(1);
    filter->SetMaximumNumberOfPoints(0);
    filter->SetRandomMode(false);

    if (!filter->Execute()) { return false; }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    return CheckCoordinates(output, {1, 4, 7});
}

bool TestNoVertexCells() {
    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, CreateTestMesh());
    filter->SetOnRatio(2);
    filter->SetGenerateVertices(false);

    if (!filter->Execute()) { return false; }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    if (output.IsNull()) { return false; }

    return output->GetNumberOfPoints() == 5 && output->GetNumberOfCells() == 0;
}

bool TestGenerateVertices() {
    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, CreateTestMesh());
    filter->SetOnRatio(2);
    filter->SetGenerateVertices(true);
    filter->SetSingleVertexPerCell(true);

    if (!filter->Execute()) { return false; }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    if (output.IsNull()) { return false; }

    if (output->GetNumberOfPoints() != 5 || output->GetNumberOfCells() != 5) { return false; }

    for (IGsize i = 0; i < output->GetNumberOfCells(); ++i) {
        if (output->GetCellType(i) != iGame::IG_VERTEX) { return false; }
    }

    return true;
}

bool TestUnsupportedPolyVertex() {
    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, CreateTestMesh());
    filter->SetOnRatio(2);
    filter->SetGenerateVertices(true);
    filter->SetSingleVertexPerCell(false);

    return !filter->Execute();
}

bool TestAttributeTypes() {
    auto input = iGame::UnstructuredMesh::New();

    for (int i = 0; i < 6; ++i) { input->AddPoint(iGame::Point(static_cast<float>(i), 0.0f, 0.0f)); }

    auto doubleArray = iGame::DoubleArray::New();
    doubleArray->SetName("DoubleScalar");
    doubleArray->SetDimension(1);
    doubleArray->Resize(6);

    auto intArray = iGame::IntArray::New();
    intArray->SetName("IntScalar");
    intArray->SetDimension(1);
    intArray->Resize(6);

    for (int i = 0; i < 6; ++i) {
        double d[1] = {1000.5 + static_cast<double>(i)};
        int v[1] = {200 + i};

        doubleArray->SetElement(i, d);
        intArray->SetElement(i, v);
    }

    input->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, doubleArray);
    input->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, intArray);

    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, input);
    filter->SetOnRatio(2);
    filter->SetOffset(0);
    filter->SetMaximumNumberOfPoints(0);
    filter->SetRandomMode(false);

    if (!filter->Execute()) { return false; }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    if (output.IsNull()) { return false; }
    if (output->GetNumberOfPoints() != 3) { return false; }

    auto attrSet = output->GetAttributeSet();
    if (attrSet == nullptr) { return false; }

    auto allAttrs = attrSet->GetAllAttributes();

    bool foundDouble = false;
    bool foundInt = false;

    for (IGsize i = 0; i < allAttrs->GetNumberOfElements(); ++i) {
        auto attr = allAttrs->GetElement(i);

        if (attr.attachmentType != IG_POINT) { continue; }

        auto array = attr.pointer;

        if (array->GetName() == "DoubleScalar") {
            if (array->GetArrayType() != IG_DoubleArray) { return false; }

            double value[1] = {0.0};

            array->GetElement(0, value);
            if (value[0] != 1000.5) { return false; }

            array->GetElement(1, value);
            if (value[0] != 1002.5) { return false; }

            array->GetElement(2, value);
            if (value[0] != 1004.5) { return false; }

            foundDouble = true;
        }

        if (array->GetName() == "IntScalar") {
            if (array->GetArrayType() != IG_IntArray) { return false; }

            int value[1] = {0};

            array->GetElement(0, value);
            if (value[0] != 200) { return false; }

            array->GetElement(1, value);
            if (value[0] != 202) { return false; }

            array->GetElement(2, value);
            if (value[0] != 204) { return false; }

            foundInt = true;
        }
    }

    return foundDouble && foundInt;
}

bool TestOffsetOutOfRange() {
    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, CreateTestMesh());
    filter->SetOnRatio(2);
    filter->SetOffset(100);
    filter->SetRandomMode(false);

    if (!filter->Execute()) { return false; }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    if (output.IsNull()) { return false; }

    return output->GetNumberOfPoints() == 0 && output->GetNumberOfCells() == 0;
}

bool TestPointAttributes() {
    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, CreateTestMesh());
    filter->SetOnRatio(3);
    filter->SetOffset(1);
    filter->SetRandomMode(false);

    if (!filter->Execute()) { return false; }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    return CheckAttributes(output, {101, 104, 107});
}

bool TestRandomizedIdStrides() {
    auto input = CreateTestMesh();

    auto filter1 = iGame::MaskPointsFilter::New();

    filter1->SetInput(0, input);
    filter1->SetOnRatio(3);
    filter1->SetOffset(2);
    filter1->SetMaximumNumberOfPoints(4);
    filter1->SetRandomMode(true);
    filter1->SetRandomModeType(iGame::MaskPointsFilter::RANDOMIZED_ID_STRIDES);
    filter1->SetRandomSeed(42);

    if (!filter1->Execute()) { return false; }

    auto filter2 = iGame::MaskPointsFilter::New();

    filter2->SetInput(0, input);
    filter2->SetOnRatio(3);
    filter2->SetOffset(2);
    filter2->SetMaximumNumberOfPoints(4);
    filter2->SetRandomMode(true);
    filter2->SetRandomModeType(iGame::MaskPointsFilter::RANDOMIZED_ID_STRIDES);
    filter2->SetRandomSeed(42);

    if (!filter2->Execute()) { return false; }

    auto output1 = iGame::DynamicCast<iGame::UnstructuredMesh>(filter1->GetOutput());

    auto output2 = iGame::DynamicCast<iGame::UnstructuredMesh>(filter2->GetOutput());

    if (output1.IsNull() || output2.IsNull()) { return false; }

    if (output1->GetNumberOfPoints() == 0 || output1->GetNumberOfPoints() > 4) { return false; }

    if (output1->GetPoint(0)[0] != 2.0) { return false; }

    for (IGsize i = 1; i < output1->GetNumberOfPoints(); ++i) {
        if (output1->GetPoint(i)[0] <= output1->GetPoint(i - 1)[0]) { return false; }
    }

    return CheckSameCoordinates(output1, output2);
}

bool TestRandomSampling() {
    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, CreateTestMesh());
    filter->SetOnRatio(100);
    filter->SetOffset(9);
    filter->SetMaximumNumberOfPoints(4);
    filter->SetRandomMode(true);
    filter->SetRandomModeType(iGame::MaskPointsFilter::RANDOM_SAMPLING);
    filter->SetRandomSeed(42);

    if (!filter->Execute()) { return false; }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    if (output.IsNull()) { return false; }
    if (output->GetNumberOfPoints() != 4) { return false; }

    bool containsPointBeforeOffset = false;

    for (IGsize i = 0; i < output->GetNumberOfPoints(); ++i) {
        if (output->GetPoint(i)[0] < 9.0) { containsPointBeforeOffset = true; }
    }

    if (!containsPointBeforeOffset) { return false; }

    return CheckAttributeMapping(output);
}

bool TestSpatiallyStratified() {
    auto input = CreateTestMesh();

    auto filter1 = iGame::MaskPointsFilter::New();

    filter1->SetInput(0, input);
    filter1->SetOnRatio(100);
    filter1->SetOffset(9);
    filter1->SetMaximumNumberOfPoints(4);
    filter1->SetRandomMode(true);
    filter1->SetRandomModeType(iGame::MaskPointsFilter::SPATIALLY_STRATIFIED);
    filter1->SetRandomSeed(42);

    if (!filter1->Execute()) { return false; }

    auto filter2 = iGame::MaskPointsFilter::New();

    filter2->SetInput(0, input);
    filter2->SetOnRatio(100);
    filter2->SetOffset(9);
    filter2->SetMaximumNumberOfPoints(4);
    filter2->SetRandomMode(true);
    filter2->SetRandomModeType(iGame::MaskPointsFilter::SPATIALLY_STRATIFIED);
    filter2->SetRandomSeed(42);

    if (!filter2->Execute()) { return false; }

    auto output1 = iGame::DynamicCast<iGame::UnstructuredMesh>(filter1->GetOutput());

    auto output2 = iGame::DynamicCast<iGame::UnstructuredMesh>(filter2->GetOutput());

    if (output1.IsNull() || output2.IsNull()) { return false; }
    if (output1->GetNumberOfPoints() != 4) { return false; }

    bool hasLeftHalf = false;
    bool hasRightHalf = false;

    for (IGsize i = 0; i < output1->GetNumberOfPoints(); ++i) {
        if (output1->GetPoint(i)[0] < 5.0) { hasLeftHalf = true; }

        if (output1->GetPoint(i)[0] >= 5.0) { hasRightHalf = true; }
    }

    if (!hasLeftHalf || !hasRightHalf) { return false; }

    if (!CheckSameCoordinates(output1, output2)) { return false; }

    return CheckAttributeMapping(output1);
}

bool TestUniformSpatialBounds() {
    auto input = CreateSpatialTestMesh();

    auto filter1 = iGame::MaskPointsFilter::New();

    filter1->SetInput(0, input);
    filter1->SetOnRatio(1000);
    filter1->SetOffset(124);
    filter1->SetMaximumNumberOfPoints(20);
    filter1->SetRandomMode(true);
    filter1->SetRandomModeType(iGame::MaskPointsFilter::UNIFORM_SPATIAL_BOUNDS);
    filter1->SetRandomSeed(42);

    if (!filter1->Execute()) { return false; }

    auto filter2 = iGame::MaskPointsFilter::New();

    filter2->SetInput(0, input);
    filter2->SetOnRatio(1000);
    filter2->SetOffset(124);
    filter2->SetMaximumNumberOfPoints(20);
    filter2->SetRandomMode(true);
    filter2->SetRandomModeType(iGame::MaskPointsFilter::UNIFORM_SPATIAL_BOUNDS);
    filter2->SetRandomSeed(42);

    if (!filter2->Execute()) { return false; }

    auto output1 = iGame::DynamicCast<iGame::UnstructuredMesh>(filter1->GetOutput());

    auto output2 = iGame::DynamicCast<iGame::UnstructuredMesh>(filter2->GetOutput());

    if (output1.IsNull() || output2.IsNull()) { return false; }

    if (output1->GetNumberOfPoints() == 0 || output1->GetNumberOfPoints() > 20) { return false; }

    if (!CheckSameCoordinates(output1, output2)) { return false; }

    for (IGsize i = 0; i < output1->GetNumberOfPoints(); ++i) {
        const auto& point = output1->GetPoint(i);

        if (point[0] < 0.0 || point[0] > 4.0 || point[1] < 0.0 || point[1] > 4.0 || point[2] < 0.0 || point[2] > 4.0) {
            return false;
        }

        if (std::round(point[0]) != point[0] || std::round(point[1]) != point[1] || std::round(point[2]) != point[2]) {
            return false;
        }
    }

    return CheckSpatialAttributeMapping(output1);
}

bool TestUniformSpatialSurface() {
    auto input = CreateSurfaceTestMesh();

    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, input);
    filter->SetOnRatio(1000);
    filter->SetOffset(7);
    filter->SetMaximumNumberOfPoints(8);
    filter->SetRandomMode(true);
    filter->SetRandomModeType(iGame::MaskPointsFilter::UNIFORM_SPATIAL_SURFACE);
    filter->SetRandomSeed(42);

    if (!filter->Execute()) { return false; }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    if (output.IsNull()) { return false; }

    if (output->GetNumberOfPoints() != 3) { return false; }
    if (output->GetNumberOfCells() != 0) { return false; }

    if (!CheckCoordinates(output, {0, 1, 2})) { return false; }

    auto attrSet = output->GetAttributeSet();
    if (attrSet == nullptr) { return false; }

    auto allAttrs = attrSet->GetAllAttributes();

    for (IGsize i = 0; i < allAttrs->GetNumberOfElements(); ++i) {
        auto attr = allAttrs->GetElement(i);

        if (attr.attachmentType != IG_POINT) { continue; }

        auto array = attr.pointer;

        if (array->GetName() != "SurfaceScalar") { continue; }
        if (array->GetNumberOfElements() != 3) { return false; }

        double value[IGAME_CELL_MAX_SIZE] = {0};

        for (IGsize j = 0; j < 3; ++j) {
            array->GetElement(j, value);

            if (value[0] != 2000.0 + static_cast<double>(j)) { return false; }
        }

        return true;
    }

    return false;
}

bool TestUniformSpatialVolume() {
    auto input = CreateVolumeTestMesh();

    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, input);

    // These must not control Volume Sampling.
    filter->SetOnRatio(1000);
    filter->SetOffset(10);

    filter->SetMaximumNumberOfPoints(11);
    filter->SetRandomMode(true);

    filter->SetRandomModeType(iGame::MaskPointsFilter::UNIFORM_SPATIAL_VOLUME);

    filter->SetRandomSeed(42);

    if (!filter->Execute()) { return false; }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    if (output.IsNull()) { return false; }

    // Only the first tetrahedron has non-zero volume.
    // Therefore only points 0,1,2,3 can be selected.
    if (output->GetNumberOfPoints() != 4) { return false; }

    if (output->GetNumberOfCells() != 0) { return false; }

    const std::vector<iGame::Point> expected = {iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                                iGame::Point(0.0f, 1.0f, 0.0f), iGame::Point(0.0f, 0.0f, 1.0f)};

    for (IGsize i = 0; i < 4; ++i) {
        const auto& actual = output->GetPoint(i);
        const auto& correct = expected[static_cast<size_t>(i)];

        if (actual[0] != correct[0] || actual[1] != correct[1] || actual[2] != correct[2]) { return false; }
    }

    auto attrSet = output->GetAttributeSet();
    if (attrSet == nullptr) { return false; }

    auto allAttrs = attrSet->GetAllAttributes();

    for (IGsize i = 0; i < allAttrs->GetNumberOfElements(); ++i) {
        auto attr = allAttrs->GetElement(i);

        if (attr.attachmentType != IG_POINT) { continue; }

        auto array = attr.pointer;

        if (array->GetName() != "VolumeScalar") { continue; }
        if (array->GetNumberOfElements() != 4) { return false; }

        double value[IGAME_CELL_MAX_SIZE] = {0};

        for (IGsize j = 0; j < 4; ++j) {
            array->GetElement(j, value);

            if (value[0] != 3000.0 + static_cast<double>(j)) { return false; }
        }

        return true;
    }

    return false;
}

bool TestRandomSeed() {
    auto input = CreateTestMesh();

    auto filter1 = iGame::MaskPointsFilter::New();

    filter1->SetInput(0, input);
    filter1->SetMaximumNumberOfPoints(5);
    filter1->SetRandomMode(true);
    filter1->SetRandomModeType(iGame::MaskPointsFilter::RANDOM_SAMPLING);
    filter1->SetRandomSeed(123);

    if (!filter1->Execute()) { return false; }

    auto filter2 = iGame::MaskPointsFilter::New();

    filter2->SetInput(0, input);
    filter2->SetMaximumNumberOfPoints(5);
    filter2->SetRandomMode(true);
    filter2->SetRandomModeType(iGame::MaskPointsFilter::RANDOM_SAMPLING);
    filter2->SetRandomSeed(123);

    if (!filter2->Execute()) { return false; }

    auto output1 = iGame::DynamicCast<iGame::UnstructuredMesh>(filter1->GetOutput());

    auto output2 = iGame::DynamicCast<iGame::UnstructuredMesh>(filter2->GetOutput());

    return CheckSameCoordinates(output1, output2);
}

bool TestEmptyInput() {
    auto input = iGame::UnstructuredMesh::New();

    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, input);
    filter->SetOnRatio(2);

    if (!filter->Execute()) { return false; }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    if (output.IsNull()) { return false; }

    return output->GetNumberOfPoints() == 0 && output->GetNumberOfCells() == 0;
}

bool TestInvalidOnRatio() {
    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, CreateTestMesh());
    filter->SetOnRatio(0);

    return !filter->Execute();
}

bool RunTest(const char* name, bool (*test)()) {
    const bool result = test();

    if (result) {
        std::cout << "Success: " << name << std::endl;
    } else {
        std::cout << "Failed: " << name << std::endl;
    }

    return result;
}

int main() {
    bool allPassed = true;

    allPassed = RunTest("OnRatio", TestOnRatio) && allPassed;
    allPassed = RunTest("MaximumNumberOfPoints", TestMaximumNumberOfPoints) && allPassed;
    allPassed = RunTest("Offset", TestOffset) && allPassed;
    allPassed = RunTest("No Vertex Cells", TestNoVertexCells) && allPassed;
    allPassed = RunTest("Generate Vertices", TestGenerateVertices) && allPassed;
    allPassed = RunTest("Unsupported PolyVertex", TestUnsupportedPolyVertex) && allPassed;
    allPassed = RunTest("Point Attributes", TestPointAttributes) && allPassed;
    allPassed = RunTest("Attribute Types", TestAttributeTypes) && allPassed;
    allPassed = RunTest("Randomized Id Strides", TestRandomizedIdStrides) && allPassed;
    allPassed = RunTest("Random Sampling", TestRandomSampling) && allPassed;
    allPassed = RunTest("Spatially Stratified", TestSpatiallyStratified) && allPassed;
    allPassed = RunTest("Uniform Spatial Bounds", TestUniformSpatialBounds) && allPassed;
    allPassed = RunTest("Uniform Spatial Surface", TestUniformSpatialSurface) && allPassed;
    allPassed = RunTest("Uniform Spatial Volume", TestUniformSpatialVolume) && allPassed;
    allPassed = RunTest("Random Seed", TestRandomSeed) && allPassed;
    allPassed = RunTest("Empty Input", TestEmptyInput) && allPassed;
    allPassed = RunTest("Offset Out Of Range", TestOffsetOutOfRange) && allPassed;
    allPassed = RunTest("Invalid OnRatio", TestInvalidOnRatio) && allPassed;

    if (!allPassed) {
        std::cout << "MaskPoints tests failed." << std::endl;
        return 1;
    }

    std::cout << "All MaskPoints core tests passed." << std::endl;
    return 0;
}
