#ifndef IGAME_EXAMPLES_VTK_UNSTRUCTURED_GRID_IO_H
#define IGAME_EXAMPLES_VTK_UNSTRUCTURED_GRID_IO_H

#include <vtkSmartPointer.h>

#include <filesystem>
#include <string>

class vtkUnstructuredGrid;

namespace vtk_datacodec_example {

vtkSmartPointer<vtkUnstructuredGrid> ReadVtkUnstructuredGrid(
    const std::filesystem::path& path,
    std::string* error = nullptr);

bool WriteVtkUnstructuredGrid(
    const std::filesystem::path& path,
    vtkUnstructuredGrid* grid,
    std::string* error = nullptr);

} // namespace vtk_datacodec_example

#endif
