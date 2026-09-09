#include <iGameFileIO.h>
#include <iGameDrawObject.h>
#include <iGameFlatArray.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>

namespace
{

bool WriteTextFile(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << contents;
    return stream.good();
}

std::string MakePointScalarVtu(const std::string& scalarName,
                               int componentCount,
                               const std::string& scalarValues) {
    std::ostringstream stream;
    stream << R"xml(<?xml version="1.0"?>
<VTKFile type="UnstructuredGrid" version="0.1" byte_order="LittleEndian">
  <UnstructuredGrid>
    <Piece NumberOfPoints="3" NumberOfCells="1">
      <PointData>
        <DataArray type="Float32" Name=")xml"
           << scalarName << R"xml(" NumberOfComponents=")xml" << componentCount
           << R"xml(" format="ascii">)xml" << scalarValues << R"xml(</DataArray>
      </PointData>
      <CellData/>
      <Points>
        <DataArray type="Float32" NumberOfComponents="3" format="ascii">
          0 0 0  1 0 0  0 1 0
        </DataArray>
      </Points>
      <Cells>
        <DataArray type="Int32" Name="connectivity" format="ascii">0 1 2</DataArray>
        <DataArray type="Int32" Name="offsets" format="ascii">3</DataArray>
        <DataArray type="UInt8" Name="types" format="ascii">5</DataArray>
      </Cells>
    </Piece>
  </UnstructuredGrid>
</VTKFile>
)xml";
    return stream.str();
}

bool NearlyEqual(double lhs, double rhs) {
    return std::abs(lhs - rhs) <= 1e-12;
}

bool HasExpectedGlobalRange(const iGame::DoubleArray::Pointer& range) {
    // For a one-component scalar, element 0 is magnitude and element 1 is
    // component 0.  Values {-4,-2,0} and {1,3,5} therefore aggregate to:
    // magnitude [0,5], component [-4,5].
    return range != nullptr && range->GetDimension() == 2 && range->GetNumberOfElements() == 2 &&
           NearlyEqual(range->GetValue(0), 0.0) && NearlyEqual(range->GetValue(1), 5.0) &&
           NearlyEqual(range->GetValue(2), -4.0) && NearlyEqual(range->GetValue(3), 5.0);
}

bool ValidatePublishedCommonScalar(const std::filesystem::path& path) {
    const auto output = iGame::FileIO::ReadFile(path.string());
    const auto root = iGame::DynamicCast<iGame::DrawObject>(output);
    if (root == nullptr || root->GetNumberOfSubDataObjects() != 2) {
        std::cerr << "Common-schema VTM did not produce a two-leaf drawable root\n";
        return false;
    }

    auto* rootAttributes = root->GetAttributeSet();
    if (rootAttributes == nullptr || rootAttributes->GetNumberOfAttributes() != 1 ||
        rootAttributes->GetAttributeIndex("PressureCoefficient") != 0) {
        std::cerr << "Common PressureCoefficient was not published exactly once at the VTM root\n";
        return false;
    }

    auto& rootAttribute = rootAttributes->GetAttribute(0);
    const auto rootProxy = iGame::DynamicCast<iGame::DoubleArray>(rootAttribute.pointer);
    const auto rootRange = rootAttribute.GetDataRange();
    if (rootAttribute.type != IG_SCALAR || rootAttribute.attachmentType != IG_POINT || rootProxy == nullptr ||
        rootProxy->GetName() != "PressureCoefficient" || rootProxy->GetDimension() != 1 ||
        rootProxy->GetNumberOfElements() != 0 || !HasExpectedGlobalRange(rootRange)) {
        std::cerr << "VTM root PressureCoefficient proxy schema or global range is incorrect\n";
        return false;
    }

    // The Core reader publishes metadata only.  Selection/default coloring is
    // a UI policy applied after loading and must not be hidden in FileIO.
    if (root->GetCurrentAttributeIndex() != -1 || root->GetCurrentAttributeDimension() != -1) {
        std::cerr << "Core VTM reader unexpectedly selected the root scalar\n";
        return false;
    }

    const auto rootMapper = root->GetColorMapper();
    int leafCount = 0;
    for (auto it = root->SubDataObjectIteratorBegin(); it != root->SubDataObjectIteratorEnd(); ++it) {
        const auto leaf = iGame::DynamicCast<iGame::DrawObject>(it->second);
        if (leaf == nullptr || leaf->GetAttributeSet() == nullptr ||
            leaf->GetAttributeSet()->GetNumberOfAttributes() != 1) {
            std::cerr << "Common-schema VTM contains an invalid leaf\n";
            return false;
        }

        auto& leafAttribute = leaf->GetAttributeSet()->GetAttribute(0);
        if (leafAttribute.pointer == nullptr || leafAttribute.pointer->GetName() != "PressureCoefficient" ||
            leafAttribute.pointer->GetDimension() != 1 || leafAttribute.GetDataRange().get() != rootRange.get() ||
            leaf->GetColorMapper().get() != rootMapper.get()) {
            std::cerr << "Leaf scalar does not share the root range or color mapper\n";
            return false;
        }

        // AddSubDataObject has already created each leaf's renderable surface.
        // It must see the same range and mapper that actual rendering will use.
        const auto renderable = leaf->GetRenderableObject(false);
        if (renderable == nullptr || renderable->GetAttributeSet() == nullptr ||
            renderable->GetAttributeSet()->GetNumberOfAttributes() != 1 ||
            renderable->GetAttributeSet()->GetAttribute(0).GetDataRange().get() != rootRange.get() ||
            renderable->GetColorMapper().get() != rootMapper.get()) {
            std::cerr << "Renderable leaf does not share the root range or color mapper\n";
            return false;
        }
        ++leafCount;
    }

    return leafCount == 2;
}

bool ValidateHeterogeneousSchemaIsNotPublished(const std::filesystem::path& path) {
    const auto output = iGame::FileIO::ReadFile(path.string());
    const auto root = iGame::DynamicCast<iGame::DrawObject>(output);
    if (root == nullptr || root->GetNumberOfSubDataObjects() != 2) {
        std::cerr << "Heterogeneous-schema VTM did not produce a two-leaf drawable root\n";
        return false;
    }
    if (root->GetAttributeSet() == nullptr || root->GetAttributeSet()->GetNumberOfAttributes() != 0) {
        std::cerr << "Heterogeneous leaf schemas were incorrectly published at the VTM root\n";
        return false;
    }
    return root->GetCurrentAttributeIndex() == -1 && root->GetCurrentAttributeDimension() == -1;
}

bool HasOneReferencedDataSet(const std::filesystem::path& path) {
    const auto output = iGame::FileIO::ReadFile(path.string());
    return output != nullptr && output->GetNumberOfSubDataObjects() == 1;
}

bool ValidateSurfaceDrawableTransitions(const std::filesystem::path& path) {
    const auto output = iGame::FileIO::ReadFile(path.string());
    if (output == nullptr || output->GetNumberOfSubDataObjects() != 1) { return false; }

    const auto child = output->SubDataObjectIteratorBegin()->second;
    const auto source = iGame::DynamicCast<iGame::UnstructuredMesh>(child);
    if (source == nullptr) { return false; }

    const auto surface = iGame::DynamicCast<iGame::SurfaceMesh>(source->GetRenderableObject(false));
    if (surface == nullptr || source->GetPoints() != surface->GetPoints() ||
        source->GetCells() != surface->GetFaces() || source->GetAttributeSet() == surface->GetAttributeSet()) {
        return false;
    }

    // Opaque desktop Surface+Wireframe uses triangle edge masks and must not
    // allocate an explicit edge list.
    source->AddViewStyle(IG_WIREFRAME);
    surface->ConvertToDrawableData();
#ifndef __EMSCRIPTEN__
    if (surface->GetEdges() != nullptr) { return false; }
#endif

    // Removing Surface changes the same object to pure wireframe. This must
    // mark only the renderable leaf dirty and build its edge list lazily.
    source->RemoveViewStyle(IG_SURFACE);
    surface->ConvertToDrawableData();
    return surface->GetEdges() != nullptr && surface->GetNumberOfEdges() == 3;
}

} // namespace

int main() {
    namespace fs = std::filesystem;

    const auto uniqueSuffix = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path testRoot = fs::temp_directory_path() / ("igame-vtm-path-validation-" + std::to_string(uniqueSuffix));
    const fs::path piecesDirectory = testRoot / "pieces";

    std::error_code error;
    if (!fs::create_directories(piecesDirectory, error) || error) {
        std::cerr << "Could not create VTM reader validation directory: " << error.message() << '\n';
        return 1;
    }

    const std::string vtu = R"xml(<?xml version="1.0"?>
<VTKFile type="UnstructuredGrid" version="0.1" byte_order="LittleEndian">
  <UnstructuredGrid>
    <Piece NumberOfPoints="3" NumberOfCells="1">
      <PointData/>
      <CellData/>
      <Points>
        <DataArray type="Float32" NumberOfComponents="3" format="ascii">
          0 0 0  1 0 0  0 1 0
        </DataArray>
      </Points>
      <Cells>
        <DataArray type="Int32" Name="connectivity" format="ascii">0 1 2</DataArray>
        <DataArray type="Int32" Name="offsets" format="ascii">3</DataArray>
        <DataArray type="UInt8" Name="types" format="ascii">5</DataArray>
      </Cells>
    </Piece>
  </UnstructuredGrid>
</VTKFile>
)xml";

    const std::string validVtm = R"xml(<?xml version="1.0"?>
<VTKFile type="vtkMultiBlockDataSet" version="1.0" byte_order="LittleEndian">
  <vtkMultiBlockDataSet>
    <DataSet index="0" file="pieces/piece.vtu"/>
  </vtkMultiBlockDataSet>
</VTKFile>
)xml";

    const std::string missingVtm = R"xml(<?xml version="1.0"?>
<VTKFile type="vtkMultiBlockDataSet" version="1.0" byte_order="LittleEndian">
  <vtkMultiBlockDataSet>
    <DataSet index="0" file="pieces/does-not-exist.vtu"/>
  </vtkMultiBlockDataSet>
</VTKFile>
)xml";

    const std::string scalarVtm = R"xml(<?xml version="1.0"?>
<VTKFile type="vtkMultiBlockDataSet" version="1.0" byte_order="LittleEndian">
  <vtkMultiBlockDataSet>
    <DataSet index="0" file="pieces/scalar-negative.vtu"/>
    <DataSet index="1" file="pieces/scalar-positive.vtu"/>
  </vtkMultiBlockDataSet>
</VTKFile>
)xml";

    const fs::path piecePath = piecesDirectory / "piece.vtu";
    const fs::path relativePathVtm = testRoot / "relative.vtm";
    const fs::path nativePathVtm = testRoot / "native-path.vtm";
    const fs::path missingPathVtm = testRoot / "missing.vtm";
    const fs::path scalarPathVtm = testRoot / "scalar.vtm";
    const fs::path negativePiecePath = piecesDirectory / "scalar-negative.vtu";
    const fs::path positivePiecePath = piecesDirectory / "scalar-positive.vtu";

    const bool filesWritten = WriteTextFile(piecePath, vtu) && WriteTextFile(relativePathVtm, validVtm) &&
                              WriteTextFile(nativePathVtm, validVtm) && WriteTextFile(missingPathVtm, missingVtm) &&
                              WriteTextFile(scalarPathVtm, scalarVtm) &&
                              WriteTextFile(negativePiecePath,
                                            MakePointScalarVtu("PressureCoefficient", 1, "-4 -2 0")) &&
                              WriteTextFile(positivePiecePath,
                                            MakePointScalarVtu("PressureCoefficient", 1, "1 3 5"));
    if (!filesWritten) {
        std::cerr << "Could not write VTM reader validation inputs\n";
        fs::remove_all(testRoot, error);
        return 1;
    }

    bool passed = true;

    // generic_string() uses forward slashes while the manifest still contains
    // a relative pieces/... reference.
    if (!HasOneReferencedDataSet(fs::path(relativePathVtm.generic_string()))) {
        std::cerr << "Forward-slash VTM path with a relative child failed\n";
        passed = false;
    }
    if (!ValidateSurfaceDrawableTransitions(fs::path(relativePathVtm.generic_string()))) {
        std::cerr << "Surface drawable sharing or lazy wireframe transition failed\n";
        passed = false;
    }

    // On Windows, path::string() uses native backslashes.  This is the exact
    // entry-path form used by the desktop file dialog and --filepath option.
    if (!HasOneReferencedDataSet(fs::path(nativePathVtm.string()))) {
        std::cerr << "Native VTM path with a relative child failed\n";
        passed = false;
    }

    if (iGame::FileIO::ReadFile(missingPathVtm.string()) != nullptr) {
        std::cerr << "VTM reader accepted a missing referenced file\n";
        passed = false;
    }

    if (!ValidatePublishedCommonScalar(scalarPathVtm)) {
        passed = false;
    }

    // Reuse the same two-piece flat manifest, but make the second leaf's
    // same-named array two-component.  A mixed schema must remain leaf-only.
    if (!WriteTextFile(positivePiecePath,
                       MakePointScalarVtu("PressureCoefficient", 2, "1 10 3 30 5 50"))) {
        std::cerr << "Could not write heterogeneous VTM reader validation input\n";
        passed = false;
    } else if (!ValidateHeterogeneousSchemaIsNotPublished(scalarPathVtm)) {
        passed = false;
    }

    fs::remove_all(testRoot, error);
    if (error) {
        std::cerr << "Warning: could not remove VTM reader validation directory: " << error.message() << '\n';
    }

    return passed ? 0 : 1;
}
