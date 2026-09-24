// Source: dayuwan77/igamevis at eccac729b57aeacbe9312d7d5189f6990bb4eebd.
// Integration gap: e7ec6571 imported the filter but omitted its example.
// Preserve the numerical/attribute checks below and reject invalid inputs;
// visual examples support --no-render so CI requires a real exit status.
// Integration commit: test: add examples for first-batch standard filters
// Find it: git log --diff-filter=A --format="%h %s" -- Examples/Filter/PointCoordinates/TestPointCoordinates.cpp
#include <PointCoordinates/iGamePointCoordinatesFilter.h>
#include <iGameDataObject.h>
#include <iGameUnstructuredMesh.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {

bool Check(bool condition, const std::string& message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; }
    return condition;
}

bool NearlyEqual(float lhs, float rhs) { return std::abs(lhs - rhs) < 1.0e-6f; }

bool TestInvalidInput() {
    auto filter = iGame::PointCoordinatesFilter::New();
    if (!Check(!filter->Execute(), "null input must be rejected")) { return false; }

    filter->SetInput(iGame::DataObject::New());
    return Check(!filter->Execute(), "input without points must be rejected");
}

bool TestEmptyPointSet() {
    auto mesh = iGame::UnstructuredMesh::New();
    auto filter = iGame::PointCoordinatesFilter::New();
    filter->SetInput(mesh);

    if (!Check(filter->Execute(), "an empty point set should produce an empty coordinate array")) { return false; }

    auto coordinates = filter->GetCoordinatesArray();
    return Check(coordinates && coordinates->GetDimension() == 3 && coordinates->GetNumberOfElements() == 0,
                 "empty coordinate array must have zero tuples and three components");
}

bool TestCoordinatesArray() {
    auto mesh = iGame::UnstructuredMesh::New();
    mesh->AddPoint(iGame::Point(1.0f, 2.0f, 3.0f));
    mesh->AddPoint(iGame::Point(-4.0f, 5.5f, 6.0f));
    mesh->AddPoint(iGame::Point(7.0f, 8.0f, -9.0f));

    igIndex triangle[3]{0, 1, 2};
    mesh->AddCell(triangle, 3, iGame::IG_TRIANGLE);
    igIndex line[2]{1, 2};
    mesh->AddCell(line, 2, iGame::IG_LINE);
    auto originalCells = mesh->GetCells();

    auto filter = iGame::PointCoordinatesFilter::New();
    filter->SetInput(mesh);
    if (!Check(filter->Execute(), "valid mesh must be processed")) { return false; }
    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (!Check(output && output.get() != mesh.get(), "filter must produce an independent output")) { return false; }
    if (!Check(mesh->GetCells().get() == originalCells.get(), "filter must preserve mesh topology")) { return false; }
    // Mixed cell sizes exercise offsets, unlike a single fixed-size triangle.
    // A seeded zero offset in a deep copy previously shifted connectivity.
    // Fix commit: test: add examples for first-batch standard filters (lookup above).
    if (!Check(output->GetNumberOfCells() == 2, "cell count must survive the copy")) return false;
    for (IGsize i = 0; i < 2; ++i) {
        const igIndex* inputIds = nullptr;
        const igIndex* outputIds = nullptr;
        const int count = mesh->GetCells()->GetCellIds(i, inputIds);
        if (!Check(output->GetCells()->GetCellIds(i, outputIds) == count,
                   "mixed cell size must survive the copy")) return false;
        for (int j = 0; j < count; ++j) {
            if (!Check(inputIds[j] == outputIds[j], "cell connectivity must match")) return false;
        }
        if (!Check(output->GetCellTypes()->GetValue(i) == mesh->GetCellTypes()->GetValue(i),
                   "cell types must match")) return false;
    }
    if (!Check(mesh->GetAttributeSet()->GetAttributeIndex("Coordinates") < 0,
               "input must not acquire the output attribute")) return false;

    auto attributes = output->GetAttributeSet();
    const int coordinateIndex = attributes->GetAttributeIndex("Coordinates");
    if (!Check(coordinateIndex >= 0, "Coordinates attribute must be present")) { return false; }

    auto& attribute = attributes->GetAttribute(coordinateIndex);
    if (!Check(attribute.GetType() == IG_VECTOR, "Coordinates must be a vector attribute")) { return false; }
    if (!Check(attribute.GetAttachmentType() == IG_POINT, "Coordinates must be attached to points")) { return false; }

    auto coordinates = iGame::DynamicCast<iGame::FloatArray>(attribute.GetPointer());
    if (!Check(coordinates && coordinates->GetDimension() == 3, "Coordinates must be a three-component FloatArray")) {
        return false;
    }
    if (!Check(coordinates->GetNumberOfElements() == mesh->GetNumberOfPoints(),
               "coordinate tuple count must equal point count")) {
        return false;
    }

    const float expected[9]{1.0f, 2.0f, 3.0f, -4.0f, 5.5f, 6.0f, 7.0f, 8.0f, -9.0f};
    for (IGsize i = 0; i < 9; ++i) {
        if (!Check(NearlyEqual(coordinates->GetValue(i), expected[i]), "coordinate values must match mesh points")) {
            return false;
        }
    }

    const auto attributeCount = attributes->GetNumberOfAttributes();
    mesh->GetPoints()->SetPoint(0, 10.0f, 20.0f, 30.0f);
    if (!Check(filter->Execute(), "repeated execution after point edits must succeed")) { return false; }
    auto refreshedOutput = filter->GetOutput();
    if (!Check(refreshedOutput.get() != output.get() &&
                       NearlyEqual(coordinates->GetValue(0), 1.0f),
               "a prior output must stay independent of later executions")) return false;
    attributes = refreshedOutput->GetAttributeSet();
    coordinates = filter->GetCoordinatesArray();
    if (!Check(attributes->GetNumberOfAttributes() == attributeCount,
               "repeated execution must not create duplicate attributes")) {
        return false;
    }
    if (!Check(NearlyEqual(coordinates->GetValue(0), 10.0f) && NearlyEqual(coordinates->GetValue(1), 20.0f) &&
                       NearlyEqual(coordinates->GetValue(2), 30.0f),
               "coordinate array must stay synchronized with point edits")) {
        return false;
    }

    auto refreshedRange = attributes->GetAttribute(attributes->GetAttributeIndex("Coordinates")).GetDataRange();
    const float expectedMagnitude = std::sqrt(10.0f * 10.0f + 20.0f * 20.0f + 30.0f * 30.0f);
    return Check(refreshedRange && NearlyEqual(refreshedRange->GetValue(1), expectedMagnitude) &&
                         NearlyEqual(refreshedRange->GetValue(3), 10.0f) &&
                         NearlyEqual(refreshedRange->GetValue(5), 20.0f) &&
                         NearlyEqual(refreshedRange->GetValue(7), 30.0f),
                 "repeated execution must refresh magnitude and component ranges");
}

bool TestNameCollision() {
    auto mesh = iGame::UnstructuredMesh::New();
    mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));

    auto conflictingArray = iGame::FloatArray::New();
    conflictingArray->SetName("Coordinates");
    conflictingArray->SetDimension(1);
    conflictingArray->AddValue(1.0f);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, conflictingArray);

    auto filter = iGame::PointCoordinatesFilter::New();
    filter->SetInput(mesh);
    return Check(!filter->Execute() && mesh->GetAttributeSet()->GetNumberOfAttributes() == 1,
                 "an existing unrelated array with the same name must be preserved and reported");
}

} // namespace

int main() {
    const bool passed = TestInvalidInput() && TestEmptyPointSet() && TestCoordinatesArray() && TestNameCollision();
    if (!passed) { return 1; }

    std::cout << "PointCoordinatesFilter tests passed.\n";
    return 0;
}
