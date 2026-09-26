#include <iGameFileIO.h>
#include <iGameFileSystem.h>
#include <VTK XML/iGameVTMReader.h>
#include <iGameDrawObject.h>
#include <iGameFlatArray.h>
#include <iGameUnstructuredMesh.h>

#include <chrono>
#include <algorithm>
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

std::string ForwardSlashUtf8Path(const std::filesystem::path& path) {
    auto result = iGame::FileSystem::PathToUtf8(path);
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
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
    const auto output = iGame::FileIO::ReadFile(iGame::FileSystem::PathToUtf8(path));
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
    const auto output = iGame::FileIO::ReadFile(iGame::FileSystem::PathToUtf8(path));
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

bool HasOneReferencedDataSet(const std::string& utf8Path) {
    const auto output = iGame::FileIO::ReadFile(utf8Path);
    return output != nullptr && output->GetNumberOfSubDataObjects() == 1;
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

    // Use UTF-8 in both slash forms; never convert through the Windows ACP.
    if (!HasOneReferencedDataSet(ForwardSlashUtf8Path(relativePathVtm))) {
        std::cerr << "Forward-slash VTM path with a relative child failed\n";
        passed = false;
    }

    // PathToUtf8 preserves native backslashes on Windows while encoding names
    // correctly for the public FileIO and XML reader entry points.
    if (!HasOneReferencedDataSet(iGame::FileSystem::PathToUtf8(nativePathVtm))) {
        std::cerr << "Native VTM path with a relative child failed\n";
        passed = false;
    }

    if (iGame::FileIO::ReadFile(iGame::FileSystem::PathToUtf8(missingPathVtm)) != nullptr) {
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

    // Regression: the XML base already opens UTF-8 paths via _wfopen on
    // Windows, but VTM used to reinterpret both the manifest directory and
    // XML child filename as ACP and then convert the resolved path back to ACP.
    const fs::path unicodeParent = testRoot / iGame::FileSystem::PathFromUtf8("中文父目录");
    const fs::path unicodePieces = unicodeParent / iGame::FileSystem::PathFromUtf8("子分块");
    const fs::path unicodePiece = unicodePieces / iGame::FileSystem::PathFromUtf8("车身网格.vtu");
    const fs::path unicodeManifest = unicodeParent / iGame::FileSystem::PathFromUtf8("整车模型.vtm");
    const std::string unicodeVtm = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<VTKFile type="vtkMultiBlockDataSet" version="1.0" byte_order="LittleEndian">
  <vtkMultiBlockDataSet>
    <DataSet index="0" file="子分块/车身网格.vtu"/>
  </vtkMultiBlockDataSet>
</VTKFile>
)xml";
    error.clear();
    if (!fs::create_directories(unicodePieces, error) || error ||
        !WriteTextFile(unicodePiece, vtu) || !WriteTextFile(unicodeManifest, unicodeVtm)) {
        std::cerr << "Could not write Unicode VTM regression inputs\n";
        passed = false;
    } else {
        const auto nativeUtf8 = iGame::FileSystem::PathToUtf8(unicodeManifest);
        const auto output = iGame::FileIO::ReadFile(nativeUtf8);
        if (!output || output->GetNumberOfSubDataObjects() != 1 ||
            output->SubDataObjectIteratorBegin()->second->GetName() != "车身网格") {
            std::cerr << "FileIO failed Chinese VTM parent/child names or changed the UTF-8 child name\n";
            passed = false;
        }
        if (!HasOneReferencedDataSet(ForwardSlashUtf8Path(unicodeManifest))) {
            std::cerr << "FileIO failed forward-slash Chinese VTM path\n";
            passed = false;
        }
        auto directReader = iGame::iGameVTMReader::New();
        directReader->SetFilePath(nativeUtf8);
        if (!directReader->Execute() || !directReader->GetOutput() ||
            directReader->GetOutput()->GetNumberOfSubDataObjects() != 1) {
            std::cerr << "Direct XML/VTM reader failed Chinese paths\n";
            passed = false;
        }
        const auto child = iGame::DynamicCast<iGame::UnstructuredMesh>(
                iGame::FileIO::ReadFile(iGame::FileSystem::PathToUtf8(unicodePiece)));
        if (!child || child->GetNumberOfPoints() != 3 || child->GetNumberOfCells() != 1) {
            std::cerr << "Direct FileIO VTU entry failed Chinese directory and filename\n";
            passed = false;
        }
    }

    fs::remove_all(testRoot, error);
    if (error) {
        std::cerr << "Warning: could not remove VTM reader validation directory: " << error.message() << '\n';
    }

    return passed ? 0 : 1;
}
