// BUG (2026-09-19): Bird_impact_37.vtu contains only VTK_VERTEX cells. The reader
// discarded those cells, and surface/shell defaults made the point cloud invisible.
// Verify cell/attribute identity, both cell accessors, and direct point rendering;
// ordinary and mixed meshes must keep their existing defaults.
// 修复提交：与本测试首次加入的提交相同，主题为：
// fix(io): preserve VTK_VERTEX cells and render point grids
// 查询提交号：git log --diff-filter=A --format="%h %s" -- Examples/IO/VertexReaderValidation.cpp
#include <VTK/iGameVTKReader.h>
#include <VTK XML/iGameVTUReader.h>
#include <iGameUnstructuredMesh.h>
#include <zlib.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace iGame;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::string Base64(const unsigned char* bytes, std::size_t size) {
    static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    for (std::size_t i = 0; i < size; i += 3) {
        const uint32_t value = (uint32_t(bytes[i]) << 16) |
                               (i + 1 < size ? uint32_t(bytes[i + 1]) << 8 : 0) |
                               (i + 2 < size ? uint32_t(bytes[i + 2]) : 0);
        result += alphabet[(value >> 18) & 63];
        result += alphabet[(value >> 12) & 63];
        result += i + 1 < size ? alphabet[(value >> 6) & 63] : '=';
        result += i + 2 < size ? alphabet[value & 63] : '=';
    }
    return result;
}

template<typename T>
std::string DataArray(const char* name, const char* type, int components, const std::vector<T>& values,
                      bool compressed, std::string& appended) {
    std::ostringstream xml;
    xml << "<DataArray Name=\"" << name << "\" type=\"" << type << "\" NumberOfComponents=\""
        << components << "\"";
    if (compressed) {
        xml << " format=\"appended\" offset=\"" << appended.size() << "\"/>";
        const auto rawSize = static_cast<uLong>(values.size() * sizeof(T));
        uLongf compressedSize = compressBound(rawSize);
        std::vector<unsigned char> bytes(compressedSize);
        Require(compress2(bytes.data(), &compressedSize, reinterpret_cast<const Bytef*>(values.data()), rawSize,
                          Z_DEFAULT_COMPRESSION) == Z_OK, "fixture compression failed");
        const uint32_t header[] = {1, 32768, static_cast<uint32_t>(rawSize), static_cast<uint32_t>(compressedSize)};
        // VTK encodes the compression header and compressed payload separately.
        appended += Base64(reinterpret_cast<const unsigned char*>(header), sizeof(header));
        appended += Base64(bytes.data(), compressedSize);
    } else {
        xml << " format=\"ascii\">";
        for (const auto value : values) xml << +value << ' ';
        xml << "</DataArray>";
    }
    return xml.str();
}

std::string Vtu(const std::vector<uint8_t>& types, const std::vector<int64_t>& connectivity,
                const std::vector<int64_t>& offsets, bool compressed) {
    std::string appended;
    std::ostringstream xml;
    xml << "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\" header_type=\"UInt32\"";
    if (compressed) xml << " compressor=\"vtkZLibDataCompressor\"";
    xml << "><UnstructuredGrid><Piece NumberOfPoints=\"4\" NumberOfCells=\"" << types.size() << "\">";
    xml << "<PointData>" << DataArray<double>("temperature", "Float64", 1, {10, 20, 30, 40}, compressed, appended)
        << "</PointData><CellData>";
    std::vector<double> cellValues;
    for (std::size_t i = 0; i < types.size(); ++i) cellValues.push_back(100.0 + i);
    xml << DataArray("cellValue", "Float64", 1, cellValues, compressed, appended) << "</CellData><Points>"
        << DataArray<float>("Points", "Float32", 3, {0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 4}, compressed, appended)
        << "</Points><Cells>"
        << DataArray("connectivity", "Int64", 1, connectivity, compressed, appended)
        << DataArray("offsets", "Int64", 1, offsets, compressed, appended)
        << DataArray("types", "UInt8", 1, types, compressed, appended)
        << "</Cells></Piece></UnstructuredGrid>";
    if (compressed) xml << "<AppendedData encoding=\"base64\">_" << appended << "</AppendedData>";
    xml << "</VTKFile>";
    return xml.str();
}

UnstructuredMesh::Pointer ReadVtu(const std::string& contents) {
    auto reader = iGameVTUReader::New();
    reader->SetMemoryBuffer(contents.data(), contents.size());
    Require(reader->Execute(), "VTU parsing failed");
    auto mesh = DynamicCast<UnstructuredMesh>(reader->GetOutput());
    Require(mesh != nullptr, "VTU output lost its unstructured mesh type");
    return mesh;
}

class RenderProbe : public UnstructuredMesh {
public:
    I_OBJECT(RenderProbe);
    static Pointer New() { return new RenderProbe; }

    void SelectPointScalar() {
        m_AttributeIndex = 0;
        m_AttributeDimension = 0;
        m_UseColor = true;
        m_AttributeChanged = true;
    }
    UnsignedIntArray::Pointer PointIndices() { return m_PointIndices; }
    FloatArray::Pointer Colors() { return m_Colors; }
    IGsize TriangleCount() { return m_TriangleIndices->GetNumberOfElements(); }

protected:
    RenderProbe() { m_IsMainRenderableObject = false; }
};

void CheckVertices(UnstructuredMesh::Pointer mesh) {
    Require(mesh->GetNumberOfPoints() == 4, "point count changed");
    Require(mesh->GetNumberOfCells() == 3, "vertex cells were discarded");
    Require(mesh->GetViewStyle() == IG_POINTS, "pure vertex grid did not default to points");
    Require(!mesh->GetShellRenderingOption(), "pure vertex grid still uses surface extraction");
    // Non-identity connectivity plus an unused point checks that cell IDs and
    // point IDs remain distinct; CellData must not be reinterpreted as PointData.
    const igIndex expected[] = {2, 0, 1};
    Cell::Pointer scratch = Triangle::New();
    for (int i = 0; i < 3; ++i) {
        Require(mesh->GetCellType(i) == IG_VERTEX, "wrong stored cell type");
        auto* cached = mesh->GetCell(i);
        Require(cached && cached->GetCellType() == IG_VERTEX, "cached GetCell did not return Vertex");
        Require(cached->GetCellSize() == 1 && cached->GetPointId(0) == expected[i], "cached vertex has wrong point ID");
        Require(mesh->GetCell(i, scratch) && scratch->GetCellType() == IG_VERTEX, "scratch GetCell did not return Vertex");
        Require(scratch->GetPointId(0) == expected[i], "scratch vertex has wrong point ID");
        for (int axis = 0; axis < 3; ++axis) {
            Require(scratch->GetPoint(0)[axis] == mesh->GetPoint(expected[i])[axis], "vertex coordinates changed");
        }
    }
    auto& pointAttribute = mesh->GetAttributeSet()->GetAttribute("temperature");
    auto& cellAttribute = mesh->GetAttributeSet()->GetAttribute("cellValue");
    Require(pointAttribute.attachmentType == IG_POINT && pointAttribute.pointer->GetNumberOfElements() == 4,
            "point attributes changed attachment or size");
    Require(cellAttribute.attachmentType == IG_CELL && cellAttribute.pointer->GetNumberOfElements() == 3,
            "cell attributes changed attachment or size");
    Require(pointAttribute.pointer->GetValue(2) == 30 && cellAttribute.pointer->GetValue(0) == 100,
            "point/cell attributes no longer match their original IDs");

    auto probe = RenderProbe::New();
    probe->SetPoints(mesh->GetPoints());
    probe->SetCells(mesh->GetCells(), mesh->GetCellTypes());
    probe->SetAttributeSet(mesh->GetAttributeSet());
    probe->SetViewStyle(mesh->GetViewStyle());
    probe->SetShellRenderingOption(mesh->GetShellRenderingOption());
    probe->SelectPointScalar();
    probe->ConvertToDrawableData();
    Require(probe->GetRenderPoints()->GetNumberOfElements() == 4, "render coordinates are empty");
    Require(probe->PointIndices()->GetNumberOfValues() == 3 && probe->TriangleCount() == 0,
            "incorrect point/triangle render indices");
    for (int i = 0; i < 3; ++i) Require(probe->PointIndices()->GetValue(i) == expected[i], "render point order changed");
    auto colors = probe->Colors();
    Require(colors && colors->GetNumberOfElements() == 4, "point scalar colors were not generated");
    bool distinct = false;
    for (int component = 0; component < 3; ++component) {
        distinct |= std::abs(colors->GetValue(component) - colors->GetValue(3 * 4 + component)) > 1e-5;
    }
    Require(distinct, "different scalar values produced identical colors");
    probe->ConvertToDrawableData();
    Require(probe->Colors() == colors, "unchanged point colors were unnecessarily regenerated");
}

void CheckLegacyVtk() {
    const std::string contents =
            "# vtk DataFile Version 3.0\nvertex regression\nASCII\nDATASET UNSTRUCTURED_GRID\n"
            "POINTS 4 float\n0 0 0  2 0 0  0 3 0  0 0 4\n"
            "CELLS 3 6\n1 2\n1 0\n1 1\nCELL_TYPES 3\n1 1 1\n"
            "POINT_DATA 4\nSCALARS temperature double 1\nLOOKUP_TABLE default\n10 20 30 40\n"
            "CELL_DATA 3\nSCALARS cellValue double 1\nLOOKUP_TABLE default\n100 101 102\n";
    auto reader = VTKReader::New();
    reader->SetMemoryBuffer(contents.data(), contents.size());
    Require(reader->Execute(), "legacy VTK parsing failed");
    auto mesh = DynamicCast<UnstructuredMesh>(reader->GetOutput());
    Require(mesh != nullptr, "legacy VTK output is not an unstructured mesh");
    CheckVertices(mesh);
}

void CheckNonVertexGrids() {
    auto triangle = ReadVtu(Vtu({5}, {0, 1, 2}, {3}, false));
    Require(triangle->GetNumberOfCells() == 1 && triangle->GetCellType(0) == IG_TRIANGLE, "triangle topology changed");
    Require(triangle->GetViewStyle() == IG_SURFACE && triangle->GetShellRenderingOption(), "triangle defaults changed");
    auto mixed = ReadVtu(Vtu({1, 5}, {2, 0, 1, 2}, {1, 4}, true));
    Require(mixed->GetNumberOfCells() == 2 && mixed->GetCellType(0) == IG_VERTEX && mixed->GetCellType(1) == IG_TRIANGLE,
            "mixed grid cell order changed");
    Require(mixed->GetViewStyle() == IG_SURFACE && mixed->GetShellRenderingOption(), "mixed grid forced into point mode");
    Cell::Pointer scratch = Vertex::New();
    Require(mixed->GetCell(1, scratch) && scratch->GetCellType() == IG_TRIANGLE, "scratch cell did not switch back to triangle");
    auto invalid = ReadVtu(Vtu({1}, {0, 1}, {2}, false));
    Require(invalid->GetNumberOfCells() == 0 && invalid->GetViewStyle() == IG_SURFACE,
            "invalid multi-point VERTEX accepted or mistaken for a pure vertex grid");

    DataObject::Pointer empty;
    auto offsets = IntArray::New();
    offsets->AddValue(0);
    VTKAbstractReader::TransferVtkCellToiGameCell(empty, offsets, IntArray::New(), IntArray::New());
    auto emptyMesh = DynamicCast<UnstructuredMesh>(empty);
    Require(emptyMesh && emptyMesh->GetNumberOfCells() == 0 && emptyMesh->GetViewStyle() == IG_SURFACE,
            "empty grid mistaken for a pure vertex grid");
}

void CheckFile(const char* path) {
    const auto start = std::chrono::steady_clock::now();
    auto reader = iGameVTUReader::New();
    reader->SetFilePath(path);
    Require(reader->Execute(), "file parsing failed");
    auto mesh = DynamicCast<UnstructuredMesh>(reader->GetOutput());
    Require(mesh && mesh->GetNumberOfCells() > 0, "file has no cells");
    Require(mesh->GetViewStyle() == IG_POINTS && !mesh->GetShellRenderingOption(), "file did not select point rendering");
    for (IGsize i = 0; i < mesh->GetNumberOfCells(); ++i) Require(mesh->GetCellType(i) == IG_VERTEX, "file has non-vertex cell");
    mesh->ConvertToDrawableData();
    Require(mesh->GetRenderPoints()->GetNumberOfElements() == mesh->GetNumberOfPoints(), "file render points lost");
    std::cout << "FILE points=" << mesh->GetNumberOfPoints() << " cells=" << mesh->GetNumberOfCells()
              << " attributes=" << mesh->GetAttributeSet()->GetNumberOfAttributes()
              << " render_points=" << mesh->GetRenderPoints()->GetNumberOfElements()
              << " elapsed_seconds=" << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
              << '\n';
}
} // namespace

int main(int argc, char** argv) {
    try {
        CheckVertices(ReadVtu(Vtu({1, 1, 1}, {2, 0, 1}, {1, 2, 3}, false)));
        std::cout << "PASS VTU ASCII vertices, attributes, cell access and render data\n";
        CheckVertices(ReadVtu(Vtu({1, 1, 1}, {2, 0, 1}, {1, 2, 3}, true)));
        std::cout << "PASS VTU appended Base64/zlib vertices\n";
        CheckLegacyVtk();
        std::cout << "PASS legacy VTK vertices\n";
        CheckNonVertexGrids();
        std::cout << "PASS triangle, mixed, invalid and empty grids\n";
        if (argc == 2) CheckFile(argv[1]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Vertex reader validation failed: " << error.what() << '\n';
        return 1;
    }
}
