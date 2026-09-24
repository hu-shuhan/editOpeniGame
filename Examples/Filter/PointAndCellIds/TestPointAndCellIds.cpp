// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/PointAndCellIds/TestPointAndCellIds.cpp
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#include <PointAndCellIds/iGamePointAndCellIdsFilter.h>

#include "iGameAttributeSet.h"
#include "iGameDataObject.h"
#include "iGameFileIO.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGameUnstructuredMesh.h"

#include <iostream>
#include <string>

namespace
{

bool Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
    }
    return condition;
}

iGame::LongLongArray::Pointer FindIds(iGame::DataObject::Pointer object,
                                      const std::string& name,
                                      IGenum attachmentType) {
    if (!object || !object->GetAttributeSet()) {
        return nullptr;
    }

    auto attributes = object->GetAttributeSet()->GetAllAttributes();
    for (IGsize i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto& attribute = attributes->GetElement(i);

        if (attribute.IsNone() ||
            attribute.GetType() != IG_SCALAR ||
            attribute.GetAttachmentType() != attachmentType ||
            attribute.GetPointer()->GetName() != name) {
            continue;
        }

        return iGame::DynamicCast<iGame::LongLongArray>(attribute.GetPointer());
    }

    return nullptr;
}

bool CheckIds(const iGame::LongLongArray::Pointer& ids, IGsize count) {
    if (!Check(ids != nullptr, "ID array must exist")) {
        return false;
    }

    if (!Check(ids->GetDimension() == 1, "ID array must have one component") ||
        !Check(ids->GetNumberOfElements() == count, "ID array size is incorrect")) {
        return false;
    }

    for (IGsize i = 0; i < count; ++i) {
        if (!Check(ids->GetValue(i) == static_cast<long long>(i),
                   "ID value is incorrect")) {
            return false;
        }
    }

    return true;
}

iGame::UnstructuredMesh::Pointer CreateMesh() {
    auto mesh = iGame::UnstructuredMesh::New();

    mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
    mesh->AddPoint(iGame::Point(0.0f, 0.0f, 1.0f));

    igIndex cell[4]{0, 1, 2, 3};
    mesh->AddCell(cell, 4, iGame::IG_TETRA);

    return mesh;
}

bool TestInvalidInput() {
    auto filter = iGame::PointAndCellIdsFilter::New();

    if (!Check(!filter->Execute(), "null input must fail")) {
        return false;
    }

    filter->SetInput(iGame::DataObject::New());
    return Check(!filter->Execute(), "non-PointSet input must fail");
}

bool TestDefaultGeneration() {
    auto input = CreateMesh();
    auto inputCells = input->GetCells();

    auto filter = iGame::PointAndCellIdsFilter::New();
    filter->SetInput(input);

    if (!Check(filter->Execute(), "default generation must succeed")) {
        return false;
    }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (!Check(output != nullptr, "output must exist") ||
        !Check(output.get() != input.get(), "output must be an independent object") ||
        !Check(input->GetCells().get() == inputCells.get(), "input topology must remain unchanged")) {
        return false;
    }

    // 输入模型不能被追加 ID 属性
    if (!Check(FindIds(input, "vtkPointIds", IG_POINT) == nullptr,
               "input point IDs must not be created") ||
        !Check(FindIds(input, "vtkCellIds", IG_CELL) == nullptr,
               "input cell IDs must not be created")) {
        return false;
    }

    auto pointIds = FindIds(output, "vtkPointIds", IG_POINT);
    auto cellIds = FindIds(output, "vtkCellIds", IG_CELL);

    return CheckIds(pointIds, output->GetNumberOfPoints()) &&
           CheckIds(cellIds, output->GetNumberOfCells());
}

bool TestGenerationOptions() {
    {
        auto input = CreateMesh();

        auto filter = iGame::PointAndCellIdsFilter::New();
        filter->SetInput(input);
        filter->SetGenerateCellIds(false);

        if (!Check(filter->Execute(), "point-only generation must succeed")) {
            return false;
        }

        auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
        if (!Check(output != nullptr, "point-only output must exist")) {
            return false;
        }

        if (!Check(FindIds(output, "vtkPointIds", IG_POINT) != nullptr,
                   "point IDs must exist") ||
            !Check(FindIds(output, "vtkCellIds", IG_CELL) == nullptr,
                   "cell IDs must not exist")) {
            return false;
        }

        if (!Check(FindIds(input, "vtkPointIds", IG_POINT) == nullptr,
                   "input point IDs must not be created")) {
            return false;
        }
    }

    {
        auto input = CreateMesh();

        auto filter = iGame::PointAndCellIdsFilter::New();
        filter->SetInput(input);
        filter->SetGeneratePointIds(false);

        if (!Check(filter->Execute(), "cell-only generation must succeed")) {
            return false;
        }

        auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
        if (!Check(output != nullptr, "cell-only output must exist")) {
            return false;
        }

        if (!Check(FindIds(output, "vtkPointIds", IG_POINT) == nullptr,
                   "point IDs must not exist") ||
            !Check(FindIds(output, "vtkCellIds", IG_CELL) != nullptr,
                   "cell IDs must exist")) {
            return false;
        }

        if (!Check(FindIds(input, "vtkCellIds", IG_CELL) == nullptr,
                   "input cell IDs must not be created")) {
            return false;
        }
    }

    return true;
}

bool TestCustomNames() {
    auto input = CreateMesh();

    auto filter = iGame::PointAndCellIdsFilter::New();
    filter->SetInput(input);
    filter->SetPointIdsArrayName("PointIds");
    filter->SetCellIdsArrayName("CellIds");

    if (!Check(filter->Execute(), "custom names must succeed")) {
        return false;
    }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (!Check(output != nullptr, "custom-name output must exist")) {
        return false;
    }

    return CheckIds(FindIds(output, "PointIds", IG_POINT), output->GetNumberOfPoints()) &&
           CheckIds(FindIds(output, "CellIds", IG_CELL), output->GetNumberOfCells()) &&
           Check(FindIds(input, "PointIds", IG_POINT) == nullptr, "input custom point IDs must not be created");
}

bool TestRepeatedExecution() {
    auto input = CreateMesh();

    auto filter = iGame::PointAndCellIdsFilter::New();
    filter->SetInput(input);

    if (!Check(filter->Execute(), "first execution must succeed")) {
        return false;
    }

    auto firstOutput = filter->GetOutput();
    if (!Check(firstOutput != nullptr, "first output must exist")) {
        return false;
    }
    const auto attributeCount = firstOutput->GetAttributeSet()->GetNumberOfAttributes();

    if (!Check(filter->Execute(), "second execution must succeed")) {
        return false;
    }

    auto secondOutput = filter->GetOutput();
    if (!Check(secondOutput != nullptr, "second output must exist") ||
        !Check(secondOutput.get() != firstOutput.get(), "each execution must create a new output object") ||
        !Check(secondOutput->GetAttributeSet()->GetNumberOfAttributes() == attributeCount,
               "repeated execution must not create duplicate attributes")) {
        return false;
    }

    // 重复执行不能修改输入模型
    return Check(FindIds(input, "vtkPointIds", IG_POINT) == nullptr,
                 "repeated execution must not modify input");
}

bool TestNameCollision() {
    auto input = CreateMesh();

    auto array = iGame::FloatArray::New();
    array->SetName("vtkPointIds");
    array->SetDimension(1);
    array->Resize(input->GetNumberOfPoints());

    input->GetAttributeSet()->AddScalar(IG_POINT, array);

    auto filter = iGame::PointAndCellIdsFilter::New();
    filter->SetInput(input);

    return Check(!filter->Execute(),
                 "conflicting attribute type must be rejected");
}

bool TestPointSetCellIds() {
    auto points = iGame::PointSet::New();
    points->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));

    auto filter = iGame::PointAndCellIdsFilter::New();
    filter->SetInput(points);
    filter->SetGeneratePointIds(false);
    filter->SetGenerateCellIds(true);

    return Check(!filter->Execute(),
                 "cell IDs on plain PointSet must fail");
}

bool TestRealModel() {
    auto object = iGame::FileIO::ReadFile(
            "Models/ClipTest_Plane_UnstructuredGrid.vtk");

    auto input = iGame::DynamicCast<iGame::UnstructuredMesh>(object);
    if (!Check(input != nullptr, "real test model must load")) {
        return false;
    }

    auto filter = iGame::PointAndCellIdsFilter::New();
    filter->SetInput(input);

    if (!Check(filter->Execute(), "real model generation must succeed")) {
        return false;
    }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (!Check(output != nullptr, "real model output must exist") ||
        !Check(output.get() != input.get(), "real model output must be independent")) {
        return false;
    }

    auto pointIds = FindIds(output, "vtkPointIds", IG_POINT);
    auto cellIds = FindIds(output, "vtkCellIds", IG_CELL);

    return CheckIds(pointIds, output->GetNumberOfPoints()) &&
           CheckIds(cellIds, output->GetNumberOfCells()) &&
           Check(FindIds(input, "vtkPointIds", IG_POINT) == nullptr,
                 "real model input must remain unchanged");
}

}

int main() {
    const bool passed =
            TestInvalidInput() &&
            TestDefaultGeneration() &&
            TestGenerationOptions() &&
            TestCustomNames() &&
            TestRepeatedExecution() &&
            TestNameCollision() &&
            TestPointSetCellIds() &&
            TestRealModel();

    if (!passed) {
        return 1;
    }

    std::cout << "PointAndCellIdsFilter tests passed.\n";
    return 0;
}
