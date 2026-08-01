#include "VtkUnstructuredGridIO.h"

#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkUnstructuredGridWriter.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <utility>

namespace vtk_datacodec_example {
namespace {

bool Fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

std::string LowercaseExtension(const std::filesystem::path& path) {
    auto extension = path.extension().string();
    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](const unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return extension;
}

} // namespace

vtkSmartPointer<vtkUnstructuredGrid> ReadVtkUnstructuredGrid(
    const std::filesystem::path& path,
    std::string* error) {
    if (!std::filesystem::is_regular_file(path)) {
        Fail(error, "VTK input file is missing: " + path.string());
        return nullptr;
    }

    const auto extension = LowercaseExtension(path);
    vtkSmartPointer<vtkUnstructuredGrid> output;
    if (extension == ".vtk") {
        auto reader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
        reader->SetFileName(path.string().c_str());
        reader->Update();
        output = reader->GetOutput();
    } else if (extension == ".vtu") {
        auto reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        reader->SetFileName(path.string().c_str());
        reader->Update();
        output = reader->GetOutput();
    } else {
        Fail(error, "VTK input extension must be .vtk or .vtu");
        return nullptr;
    }

    if (output == nullptr || output->GetPoints() == nullptr) {
        Fail(error, "VTK reader produced no unstructured grid");
        return nullptr;
    }
    return output;
}

bool WriteVtkUnstructuredGrid(
    const std::filesystem::path& path,
    vtkUnstructuredGrid* grid,
    std::string* error) {
    if (grid == nullptr) {
        return Fail(error, "VTK writer requires an output grid");
    }
    if (!path.parent_path().empty()) {
        std::error_code directoryError;
        std::filesystem::create_directories(path.parent_path(), directoryError);
        if (directoryError) {
            return Fail(error, "failed to create VTK output directory: " + directoryError.message());
        }
    }

    const auto extension = LowercaseExtension(path);
    if (extension == ".vtk") {
        auto writer = vtkSmartPointer<vtkUnstructuredGridWriter>::New();
        writer->SetFileName(path.string().c_str());
        writer->SetFileTypeToBinary();
        writer->SetInputData(grid);
        return writer->Write() == 1
            ? true
            : Fail(error, "failed to write legacy VTK output");
    }
    if (extension == ".vtu") {
        auto writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
        writer->SetFileName(path.string().c_str());
        writer->SetDataModeToBinary();
        writer->SetInputData(grid);
        return writer->Write() == 1
            ? true
            : Fail(error, "failed to write VTU output");
    }
    return Fail(error, "VTK output extension must be .vtk or .vtu");
}

} // namespace vtk_datacodec_example
