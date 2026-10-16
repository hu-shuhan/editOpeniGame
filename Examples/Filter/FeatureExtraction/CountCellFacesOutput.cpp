// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/FeatureExtraction/CountCellFacesOutput.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include "FeatureExtraction/iGameCountCellFacesFilter.h"
#include "iGameFileIO.h"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {
using namespace iGame;

void Require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

const AttributeSet::Attribute* Find(AttributeSet* attributes,
                                    const std::string& name, IGenum attachment) {
    for (IGsize id = 0; id < attributes->GetNumberOfAttributes(); ++id) {
        const auto& attribute = attributes->GetAttribute(id);
        if (!attribute.isDeleted && attribute.pointer
            && attribute.pointer->GetName() == name
            && attribute.attachmentType == attachment) return &attribute;
    }
    return nullptr;
}

DataObject::Pointer CheckOutput(DataObject::Pointer input, const char* countsFile = nullptr) {
    Require(!input.IsNull(), "Missing input.");
    auto* attributes = input->GetAttributeSet();
    const auto attributeCount = attributes->GetNumberOfAttributes();
    auto filter = CountCellFacesFilter::New();
    filter->SetInput(input);
    Require(filter->Execute(), "Execute failed.");
    auto output = filter->GetOutput();
    Require(output && output != input && output->GetDataObjectId() != input->GetDataObjectId(),
            "Output must be a separate data object.");
    Require(output->GetAttributeSet() != attributes, "Attribute set is shared with input.");
    Require(input->GetAttributeSet() == attributes
            && attributes->GetNumberOfAttributes() == attributeCount, "Input was changed.");
    auto points = input->GetPoints();
    auto copiedPoints = output->GetPoints();
    Require(points && copiedPoints && points != copiedPoints
            && points->GetNumberOfPoints() == copiedPoints->GetNumberOfPoints(),
            "Output points must be independent.");
    for (IGsize id = 0; id < points->GetNumberOfPoints(); ++id)
        for (int axis = 0; axis < 3; ++axis)
            Require(points->GetPoint(id)[axis] == copiedPoints->GetPoint(id)[axis],
                    "Point coordinates changed.");
    for (IGsize id = 0; id < attributeCount; ++id) {
        const auto& before = attributes->GetAttribute(id);
        if (before.isDeleted || !before.pointer) continue;
        if (before.attachmentType == IG_CELL
            && before.pointer->GetName() == "cellFaceCounts") continue;
        const auto* after = Find(output->GetAttributeSet(), before.pointer->GetName(),
                                 before.attachmentType);
        Require(after && after->pointer != before.pointer && after->type == before.type
                && after->pointer->GetArrayType() == before.pointer->GetArrayType()
                && after->pointer->GetDimension() == before.pointer->GetDimension()
                && after->pointer->GetNumberOfValues() == before.pointer->GetNumberOfValues(),
                "An existing attribute was lost or shared.");
        for (IGsize i = 0; i < before.pointer->GetNumberOfValues(); ++i)
            Require(after->pointer->GetValue(i) == before.pointer->GetValue(i),
                    "An attribute value changed.");
        if (before.dataRange) {
            Require(after->dataRange && after->dataRange != before.dataRange
                    && after->dataRange->GetNumberOfValues() == before.dataRange->GetNumberOfValues(),
                    "Attribute range was lost or shared.");
            for (IGsize i = 0; i < before.dataRange->GetNumberOfValues(); ++i)
                Require(after->dataRange->GetValue(i) == before.dataRange->GetValue(i),
                        "Attribute range changed.");
        }
    }
    const auto* result = Find(output->GetAttributeSet(), "cellFaceCounts", IG_CELL);
    Require(result && result->pointer == filter->GetResult()
            && result->type == IG_SCALAR, "Missing cell result attribute.");
    if (countsFile) {
        std::ofstream stream(countsFile);
        Require(stream.is_open(), "Cannot open counts output file.");
        for (IGsize i = 0; i < filter->GetResult()->GetNumberOfValues(); ++i)
            stream << filter->GetResult()->GetValue(i) << '\n';
        stream.close();
        Require(!stream.fail(), "Failed to write counts output file.");
    }
    filter->SetInput(output);
    Require(filter->Execute() && filter->GetOutput() != output,
            "Repeated execution reused its input.");
    Require(filter->GetOutput()->GetAttributeSet()->GetNumberOfAttributes()
            == output->GetAttributeSet()->GetNumberOfAttributes(), "Duplicate result attribute.");
    filter->SetInput(nullptr);
    Require(!filter->Execute() && !filter->GetOutput() && !filter->GetResult(),
            "Failed execution retained an old output.");
    return output;
}

void CheckTensorQuad() {
    auto input = UnstructuredMesh::New();
    input->AddPoint(Point(0, 0, 0));
    input->AddPoint(Point(1, 0, 0));
    input->AddPoint(Point(1, 1, 0));
    input->AddPoint(Point(0, 1, 0));
    igIndex ids[4] = {0, 1, 2, 3};
    input->AddCell(ids, 4, IG_QUAD);
    auto pointCounts = IntArray::New();
    pointCounts->SetName("cellFaceCounts");
    pointCounts->Resize(4);
    for (int i = 0; i < 4; ++i) pointCounts->SetValue(i, 10 + i);
    auto range = DoubleArray::New();
    range->SetDimension(2);
    range->Resize(1);
    range->SetValue(0, 0);
    range->SetValue(1, 20);
    input->GetAttributeSet()->AddScalar(IG_POINT, pointCounts, range);
    auto oldCounts = UnsignedIntArray::New();
    oldCounts->SetName("cellFaceCounts");
    oldCounts->Resize(1);
    oldCounts->SetValue(0, 99);
    input->GetAttributeSet()->AddScalar(IG_CELL, oldCounts);
    auto tensor = DoubleArray::New();
    tensor->SetName("stress");
    tensor->SetDimension(9);
    tensor->Resize(4);
    for (IGsize i = 0; i < 36; ++i) tensor->SetValue(i, i);
    input->GetAttributeSet()->AddAttribute(IG_TENSOR, IG_POINT, tensor);
    auto output = CheckOutput(input);
    Require(Find(output->GetAttributeSet(), "cellFaceCounts", IG_CELL)->pointer->GetValue(0) == 0,
            "A quad has zero three-dimensional faces.");
    Require(oldCounts->GetValue(0) == 99 && pointCounts->GetValue(0) == 10,
            "Input values changed.");
    output->GetPoints()->SetPoint(0, Point(5, 5, 5));
    auto copiedPointCounts = DynamicCast<IntArray>(
        Find(output->GetAttributeSet(), "cellFaceCounts", IG_POINT)->pointer);
    copiedPointCounts->SetValue(0, 100);
    Require(input->GetPoint(0)[0] == 0 && pointCounts->GetValue(0) == 10,
            "Editing output affected input.");

    // The input's attribute owner need not be a renderable object.
    auto owner = DataObject::New();
    owner->SetAttributeSet(input->GetAttributeSet());
    CheckOutput(input);
    std::cout << "Tensor quad, same-name point data, ranges, repeated execution: PASS\n";
}
} // namespace

int main(int argc, char* argv[]) {
    try {
        CheckTensorQuad();
        if (argc == 2 || argc == 3)
            CheckOutput(iGame::FileIO::ReadFile(argv[1]), argc == 3 ? argv[2] : nullptr);
        else Require(argc == 1, "Usage: testCountCellFacesOutput [input-model [counts-file]]");
        std::cout << "Independent output regression: PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
