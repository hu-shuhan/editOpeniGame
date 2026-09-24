// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TriangleStrip/TestTriangleStrip.cpp
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#include <Convert/iGameConvertToSurfaceMeshFilter.h>
#include <DataProcessing/iGameMeshTriangulationFilter.h>
#include <TriangleStrip/iGameTriangleStripFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace
{

using namespace iGame;

constexpr const char* ModelFilePath = "Models/TriangleStripTestModel.vtk";
constexpr const char* ParaViewRegressionModelFilePath =
        "Models/SurfaceNormalsFilter_test.vtk";
constexpr const char* PointScalarName = "TriangleStripTestPointScalar";
constexpr const char* PointVectorName = "TriangleStripTestPointVector";
constexpr const char* CellScalarName = "TriangleStripTestCellScalar";
constexpr IGsize ExpectedTriangleCount = 8;
constexpr IGsize ExpectedFullStripCount = 1;
constexpr IGsize ExpectedInputLineSegmentCount = 4;
constexpr IGsize ExpectedJoinedPolylineCount = 1;
constexpr int ExpectedJoinedPointCount = 5;
constexpr IGsize ExpectedParaViewTriangleCount = 2752;
constexpr IGsize ExpectedParaViewOutputCellCount = 381;

void Check(bool condition, const std::string& message) {
    if (!condition) { throw std::runtime_error(message); }
}

void CheckCellArraysEqual(const CellArray* actual, const CellArray* expected,
                          const std::string& label) {
    Check(actual != nullptr && expected != nullptr,
          label + " contains a null CellArray.");
    Check(actual->GetNumberOfCells() == expected->GetNumberOfCells(),
          label + " has an unexpected cell count.");

    for (IGsize cellId = 0; cellId < expected->GetNumberOfCells(); ++cellId) {
        const igIndex* actualIds = nullptr;
        const igIndex* expectedIds = nullptr;
        const int actualCount = actual->GetCellIds(cellId, actualIds);
        const int expectedCount = expected->GetCellIds(cellId, expectedIds);
        Check(actualCount == expectedCount,
              label + " has an unexpected cell size.");
        for (int i = 0; i < expectedCount; ++i) {
            Check(actualIds[i] == expectedIds[i],
                  label + " has an unexpected ID value.");
        }
    }
}

void AddAttributeFixtures(const SurfaceMesh::Pointer& mesh) {
    auto pointScalars = DoubleArray::New();
    pointScalars->SetName(PointScalarName);
    pointScalars->SetDimension(1);
    pointScalars->Reserve(mesh->GetNumberOfPoints());
    for (IGsize pointId = 0; pointId < mesh->GetNumberOfPoints(); ++pointId) {
        pointScalars->AddValue(static_cast<double>(pointId) + 0.25);
    }

    auto pointVectors = FloatArray::New();
    pointVectors->SetName(PointVectorName);
    pointVectors->SetDimension(3);
    pointVectors->Reserve(mesh->GetNumberOfPoints());
    for (IGsize pointId = 0; pointId < mesh->GetNumberOfPoints(); ++pointId) {
        const float id = static_cast<float>(pointId);
        pointVectors->AddElement3(id, id + 1.0f, id + 2.0f);
    }

    auto cellScalars = IntArray::New();
    cellScalars->SetName(CellScalarName);
    cellScalars->SetDimension(1);
    cellScalars->Reserve(mesh->GetNumberOfFaces());
    for (IGsize faceId = 0; faceId < mesh->GetNumberOfFaces(); ++faceId) {
        cellScalars->AddValue(static_cast<int>(faceId) + 1000);
    }

    auto* attributes = mesh->GetAttributeSet();
    attributes->AddAttribute(IG_SCALAR, IG_POINT, pointScalars);
    attributes->AddAttribute(IG_VECTOR, IG_POINT, pointVectors);
    attributes->AddAttribute(IG_SCALAR, IG_CELL, cellScalars);
}

void ValidateAttributes(const TriangleStripFilter::Pointer& filter,
                        const SurfaceMesh::Pointer& input,
                        const SurfaceMesh::Pointer& output) {
    auto* inputAttributes = input->GetAttributeSet();
    auto* outputAttributes = output->GetAttributeSet();
    Check(inputAttributes != nullptr && outputAttributes != nullptr,
          "Input or output AttributeSet is null.");
    Check(inputAttributes != outputAttributes,
          "TriangleStripFilter reused the input AttributeSet container.");

    for (const char* name: {PointScalarName, PointVectorName}) {
        const int inputIndex = inputAttributes->GetAttributeIndex(name);
        const int outputIndex = outputAttributes->GetAttributeIndex(name);
        Check(inputIndex >= 0 && outputIndex >= 0,
              std::string("Missing point attribute: ") + name);

        auto& inputAttribute = inputAttributes->GetAttribute(inputIndex);
        auto& outputAttribute = outputAttributes->GetAttribute(outputIndex);
        Check(outputAttribute.attachmentType == IG_POINT,
              std::string("Wrong point attachment for: ") + name);
        Check(outputAttribute.type == inputAttribute.type,
              std::string("Point attribute type changed for: ") + name);
        Check(outputAttribute.pointer->GetArrayType() ==
                      inputAttribute.pointer->GetArrayType(),
              std::string("Point array storage type changed for: ") + name);
        Check(outputAttribute.pointer->GetDimension() ==
                      inputAttribute.pointer->GetDimension(),
              std::string("Point array dimension changed for: ") + name);
        Check(outputAttribute.pointer->GetNumberOfElements() ==
                      inputAttribute.pointer->GetNumberOfElements(),
              std::string("Point tuple count changed for: ") + name);

        for (IGsize pointId = 0;
             pointId < inputAttribute.pointer->GetNumberOfElements(); ++pointId) {
            for (int component = 0;
                 component < inputAttribute.pointer->GetDimension(); ++component) {
                const double expected =
                        inputAttribute.pointer->GetElementValue(pointId, component);
                const double actual =
                        outputAttribute.pointer->GetElementValue(pointId, component);
                Check(std::abs(expected - actual) < 1e-9,
                      std::string("Point attribute value changed for: ") + name);
            }
        }
    }

    const int inputCellIndex = inputAttributes->GetAttributeIndex(CellScalarName);
    const int outputCellIndex = outputAttributes->GetAttributeIndex(CellScalarName);
    Check(inputCellIndex >= 0 && outputCellIndex >= 0,
          "Missing remapped cell attribute.");
    auto& inputCellAttribute = inputAttributes->GetAttribute(inputCellIndex);
    auto& outputCellAttribute = outputAttributes->GetAttribute(outputCellIndex);
    Check(outputCellAttribute.attachmentType == IG_CELL,
          "Cell attribute has the wrong attachment type.");
    Check(outputCellAttribute.pointer.get() != inputCellAttribute.pointer.get(),
          "Cell attribute array was shared instead of remapped.");
    Check(outputCellAttribute.pointer->GetArrayType() == IG_IntArray,
          "Cell attribute storage type was not preserved.");
    Check(outputCellAttribute.pointer->GetNumberOfElements() ==
                  output->GetNumberOfFaces(),
          "Cell attribute tuple count does not match output faces.");

    std::vector<igIndex> outputSourceFaceIds;
    for (const auto& stripFaceIds: filter->GetStripSourceFaceIds()) {
        outputSourceFaceIds.insert(outputSourceFaceIds.end(),
                                   stripFaceIds.begin(), stripFaceIds.end());
    }
    Check(outputSourceFaceIds.size() ==
                  static_cast<std::size_t>(output->GetNumberOfFaces()),
          "Test could not reconstruct the output/source face mapping.");
    for (IGsize outputFaceId = 0;
         outputFaceId < output->GetNumberOfFaces(); ++outputFaceId) {
        const igIndex sourceFaceId =
                outputSourceFaceIds[static_cast<std::size_t>(outputFaceId)];
        const igIndex* sourcePointIds = nullptr;
        const igIndex* outputPointIds = nullptr;
        const int sourcePointCount =
                input->GetFaces()->GetCellIds(sourceFaceId, sourcePointIds);
        const int outputPointCount =
                output->GetFaces()->GetCellIds(outputFaceId, outputPointIds);
        Check(sourcePointIds != nullptr && outputPointIds != nullptr &&
                      sourcePointCount == 3 && outputPointCount == 3,
              "Cannot compare source and output triangle geometry.");
        std::array<igIndex, 3> sourceTriangle{
                sourcePointIds[0], sourcePointIds[1], sourcePointIds[2]};
        std::array<igIndex, 3> outputTriangle{
                outputPointIds[0], outputPointIds[1], outputPointIds[2]};
        std::sort(sourceTriangle.begin(), sourceTriangle.end());
        std::sort(outputTriangle.begin(), outputTriangle.end());
        Check(sourceTriangle == outputTriangle,
              "Cell attribute mapping does not match output triangle geometry.");

        const double expected =
                inputCellAttribute.pointer->GetElementValue(sourceFaceId, 0);
        const double actual =
                outputCellAttribute.pointer->GetElementValue(outputFaceId, 0);
        Check(expected == actual,
              "Cell attribute value does not match its source face.");
    }
}

SurfaceMesh::Pointer ExtractAndTriangulate(DataObject::Pointer input) {
    auto surfaceFilter = ConvertToSurfaceMeshFilter::New();
    surfaceFilter->SetInput(input);
    surfaceFilter->SetConvertMethod(
            ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_MESH);
    Check(surfaceFilter->Execute(), "Failed to extract the volume boundary.");

    auto surface = DynamicCast<SurfaceMesh>(surfaceFilter->GetOutput());
    Check(surface != nullptr, "Surface extraction returned a non-surface object.");
    Check(surface->GetNumberOfFaces() > 0,
          "Surface extraction returned no boundary faces.");

    std::cout << "  Extracted surface: points=" << surface->GetNumberOfPoints()
              << ", faces=" << surface->GetNumberOfFaces() << '\n';

    auto triangulation = MeshTriangulationFilter::New();
    triangulation->SetInput(surface);
    Check(triangulation->Execute(), "Failed to triangulate the extracted surface.");

    auto triangles = DynamicCast<SurfaceMesh>(triangulation->GetOutput());
    Check(triangles != nullptr, "Triangulation returned a non-surface object.");
    Check(triangles->GetNumberOfFaces() > 0,
          "Triangulation returned no triangles.");

    for (IGsize faceId = 0; faceId < triangles->GetNumberOfFaces(); ++faceId) {
        Check(triangles->GetFaces()->GetCellSize(faceId) == 3,
              "Triangulation output still contains a non-triangle face.");
    }

    std::cout << "  Triangulated surface: points=" << triangles->GetNumberOfPoints()
              << ", triangles=" << triangles->GetNumberOfFaces() << '\n';
    return triangles;
}

void ValidateStripCoverage(const TriangleStripFilter::Pointer& filter,
                           const SurfaceMesh::Pointer& input) {
    auto* strips = filter->GetStrips();
    Check(strips != nullptr, "Triangle-strip array is null.");
    Check(strips->GetNumberOfCells() > 0, "No triangle strips were generated.");
    Check(filter->GetNumberOfStrips() == strips->GetNumberOfCells(),
          "GetNumberOfStrips disagrees with the strip cell array.");
    Check(filter->GetNumberOfOutputCells() == filter->GetNumberOfStrips(),
          "Pure triangle output cells must equal the strip count.");
    Check(filter->GetLongestStripLength() > 1,
          "The model was reduced only to one-triangle strips.");
    Check(filter->GetLongestStripLength() <=
                  static_cast<IGsize>(filter->GetMaximumLength()),
          "A generated strip exceeds MaximumLength.");

    const auto& sourceFaceIds = filter->GetStripSourceFaceIds();
    Check(sourceFaceIds.size() ==
                  static_cast<std::size_t>(strips->GetNumberOfCells()),
          "Strip/source-face mapping count is inconsistent.");

    std::vector<bool> covered(
            static_cast<std::size_t>(input->GetNumberOfFaces()), false);
    IGsize coveredTriangleCount = 0;

    for (IGsize stripId = 0; stripId < strips->GetNumberOfCells(); ++stripId) {
        const igIndex* pointIds = nullptr;
        const int pointCount = strips->GetCellIds(stripId, pointIds);
        Check(pointIds != nullptr && pointCount >= 3,
              "A triangle strip has fewer than three points.");
        Check(pointCount <= filter->GetMaximumLength() + 2,
              "A triangle strip point sequence exceeds MaximumLength + 2.");

        const IGsize stripTriangleCount = static_cast<IGsize>(pointCount - 2);
        const auto& mappedFaces =
                sourceFaceIds[static_cast<std::size_t>(stripId)];
        Check(mappedFaces.size() ==
                      static_cast<std::size_t>(stripTriangleCount),
              "A strip's source-face mapping has the wrong length.");

        for (int i = 0; i + 2 < pointCount; ++i) {
            Check(pointIds[i] != pointIds[i + 1] &&
                          pointIds[i] != pointIds[i + 2] &&
                          pointIds[i + 1] != pointIds[i + 2],
                  "A generated strip contains a degenerate triangle.");
        }

        for (const igIndex faceId: mappedFaces) {
            Check(faceId >= 0 &&
                          static_cast<IGsize>(faceId) < input->GetNumberOfFaces(),
                  "A strip references an invalid source face.");
            const auto faceIndex = static_cast<std::size_t>(faceId);
            Check(!covered[faceIndex],
                  "A source triangle is used by more than one strip.");
            covered[faceIndex] = true;
        }
        coveredTriangleCount += stripTriangleCount;
    }

    std::cout << "  Triangles before strip conversion: "
              << input->GetNumberOfFaces() << '\n'
              << "  Triangles after strip conversion (sum(pointCount - 2)): "
              << coveredTriangleCount << '\n';

    Check(coveredTriangleCount == input->GetNumberOfFaces(),
          "The strips do not cover every input triangle exactly once.");
    for (const bool wasCovered: covered) {
        Check(wasCovered, "At least one input triangle is missing from the strips.");
    }

    Check(filter->GetPassThroughPolys() != nullptr,
          "Pass-through polygon array is null.");
    Check(filter->GetPassThroughPolys()->GetNumberOfCells() == 0,
          "A triangulated input unexpectedly produced pass-through polygons.");

    auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    Check(output != nullptr, "TriangleStripFilter returned no SurfaceMesh output.");
    Check(output->GetNumberOfFaces() == input->GetNumberOfFaces(),
          "Reconstructed output has a different triangle count.");
    Check(output->GetPoints().get() == input->GetPoints().get(),
          "TriangleStripFilter did not preserve the shared point array.");

    CellArray::Pointer outputStrips;
    CellArray::Pointer outputStripSourceFaceIds;
    Check(TriangleStripFilter::ReadOutputStrips(
                  output, outputStrips, outputStripSourceFaceIds),
          "The formal output did not retain triangle strips and source-face mappings.");
    Check(outputStrips->GetNumberOfCells() == filter->GetNumberOfStrips(),
          "The formal output has an unexpected triangle-strip count.");
    CheckCellArraysEqual(outputStrips.get(), strips,
                         "Persisted triangle strips");

    for (IGsize stripId = 0; stripId < outputStrips->GetNumberOfCells(); ++stripId) {
        const igIndex* persistedFaceIds = nullptr;
        const int persistedFaceCount = outputStripSourceFaceIds->GetCellIds(
                stripId, persistedFaceIds);
        const auto& expectedFaceIds =
                sourceFaceIds[static_cast<std::size_t>(stripId)];
        Check(persistedFaceCount == static_cast<int>(expectedFaceIds.size()),
              "A persisted strip/source-face mapping has the wrong size.");
        for (int i = 0; i < persistedFaceCount; ++i) {
            Check(persistedFaceIds[i] == expectedFaceIds[static_cast<std::size_t>(i)],
                  "A persisted strip/source-face mapping has the wrong value.");
        }
    }

    ValidateAttributes(filter, input, output);
    std::cout << "  Attributes: new AttributeSet, PointData preserved, "
                 "CellData remapped\n";
}

void TestTriangleStripGeneration(const SurfaceMesh::Pointer& triangles) {
    AddAttributeFixtures(triangles);
    SurfaceMesh::Pointer persistedOutput;
    {
        auto filter = TriangleStripFilter::New();
        filter->SetInput(triangles);
        filter->SetMaximumLength(1000);
        filter->SetJoinContiguousSegments(false);

        Check(filter->Execute(), "TriangleStripFilter::Execute failed.");
        ValidateStripCoverage(filter, triangles);

        Check(triangles->GetNumberOfFaces() == ExpectedTriangleCount,
              "The triangle-strip test model has an unexpected triangle count.");
        Check(filter->GetNumberOfStrips() == ExpectedFullStripCount,
              "The test model was not converted into one continuous triangle strip.");
        Check(filter->GetLongestStripLength() == ExpectedTriangleCount,
              "The generated triangle strip does not cover the full test ribbon.");

        const double averageTrianglesPerStrip =
                static_cast<double>(triangles->GetNumberOfFaces()) /
                static_cast<double>(filter->GetNumberOfStrips());
        std::cout << "  Triangle strips: count=" << filter->GetNumberOfStrips()
                  << ", longest=" << filter->GetLongestStripLength()
                  << ", average triangles/strip=" << averageTrianglesPerStrip
                  << '\n';

        persistedOutput = DynamicCast<SurfaceMesh>(filter->GetOutput());
    }

    CellArray::Pointer persistedStrips;
    CellArray::Pointer persistedSourceFaceIds;
    Check(persistedOutput != nullptr &&
                  TriangleStripFilter::ReadOutputStrips(
                          persistedOutput, persistedStrips,
                          persistedSourceFaceIds),
          "Triangle-strip metadata was lost after the filter lifetime ended.");
    Check(persistedStrips->GetNumberOfCells() == ExpectedFullStripCount,
          "Persisted output has an unexpected strip count.");
    Check(persistedStrips->GetCellSize(0) ==
                  static_cast<int>(ExpectedTriangleCount + 2) &&
                  persistedSourceFaceIds->GetCellSize(0) ==
                          static_cast<int>(ExpectedTriangleCount),
          "Persisted strip or source-face mapping has an unexpected size.");

    std::cout << "  Formal output Metadata: strips and source-face mappings "
                 "persist after filter destruction\n";
}

void TestParaViewOutputCellCount() {
    const std::filesystem::path modelPath{ParaViewRegressionModelFilePath};
    Check(std::filesystem::exists(modelPath),
          "The ParaView comparison model does not exist.");

    auto input = FileIO::ReadFile(modelPath.string());
    Check(input != nullptr, "Cannot read the ParaView comparison model.");
    auto triangles = ExtractAndTriangulate(input);
    Check(triangles->GetNumberOfFaces() == ExpectedParaViewTriangleCount,
          "The ParaView comparison model has an unexpected triangle count.");

    auto filter = TriangleStripFilter::New();
    filter->SetInput(triangles);
    filter->SetMaximumLength(1000);
    filter->SetJoinContiguousSegments(false);
    Check(filter->Execute(),
          "TriangleStripFilter failed for the ParaView comparison model.");
    std::cout << "  ParaView-compatible cells: triangles="
              << triangles->GetNumberOfFaces()
              << ", strips=" << filter->GetNumberOfStrips()
              << ", output cells=" << filter->GetNumberOfOutputCells()
              << '\n';
    Check(filter->GetNumberOfStrips() == ExpectedParaViewOutputCellCount,
          "Triangle-strip count differs from ParaView 5.12/vtkStripper 9.3.");
    Check(filter->GetNumberOfOutputCells() ==
                  ExpectedParaViewOutputCellCount,
          "ParaView-compatible output-cell count is incorrect.");

}

void TestContiguousPolylineJoining(const SurfaceMesh::Pointer& triangles) {
    // An open triangle surface must not manufacture boundary lines. This is
    // the vtkStripper/ParaView behavior being guarded by this regression test.
    auto surfaceOnly = TriangleStripFilter::New();
    surfaceOnly->SetInput(triangles);
    surfaceOnly->SetJoinContiguousSegments(true);
    Check(surfaceOnly->Execute(),
          "TriangleStripFilter failed for the surface-only line test.");
    Check(surfaceOnly->GetPolyLines() != nullptr &&
                  surfaceOnly->GetPolyLines()->GetNumberOfCells() == 0,
          "Triangle boundaries were incorrectly emitted as input polylines.");

    // Add four explicit, contiguous IG_LINE cells to the triangle input.
    // Only these cells are eligible for JoinContiguousSegments.
    auto mixedCells = CellArray::New();
    auto mixedTypes = UnsignedIntArray::New();
    for (IGsize faceId = 0; faceId < triangles->GetNumberOfFaces(); ++faceId) {
        const igIndex* pointIds = nullptr;
        const int pointCount = triangles->GetFaces()->GetCellIds(
                faceId, pointIds);
        mixedCells->AddCellIds(pointIds, pointCount);
        mixedTypes->AddValue(IG_TRIANGLE);
    }
    for (igIndex pointId = 0; pointId < 4; ++pointId) {
        const igIndex line[2]{pointId, pointId + 1};
        mixedCells->AddCellIds(line, 2);
        mixedTypes->AddValue(IG_LINE);
    }
    auto mixedInput = UnstructuredMesh::New();
    mixedInput->SetName("TriangleStripExplicitLines");
    mixedInput->SetPoints(triangles->GetPoints());
    mixedInput->SetCells(mixedCells, mixedTypes);
    mixedInput->SetAttributeSet(
            AttributeSet::Pointer(triangles->GetAttributeSet()));

    auto separate = TriangleStripFilter::New();
    separate->SetInput(mixedInput);
    separate->SetJoinContiguousSegments(false);
    Check(separate->Execute(),
          "TriangleStripFilter failed with separate input line segments.");

    auto* separateLines = separate->GetPolyLines();
    Check(separateLines != nullptr, "Separate polyline array is null.");
    Check(separateLines->GetNumberOfCells() == ExpectedInputLineSegmentCount,
          "The explicit input has an unexpected line-segment count.");
    Check(separate->GetNumberOfOutputCells() ==
                  separate->GetNumberOfStrips() +
                          ExpectedInputLineSegmentCount,
          "Separate-line output-cell count is incorrect.");
    for (IGsize lineId = 0; lineId < separateLines->GetNumberOfCells();
         ++lineId) {
        Check(separateLines->GetCellSize(lineId) == 2,
              "Join disabled: an input segment is not a two-point line.");
    }

    auto joined = TriangleStripFilter::New();
    joined->SetInput(mixedInput);
    joined->SetJoinContiguousSegments(true);
    Check(joined->Execute(),
          "TriangleStripFilter failed while joining boundary segments.");

    auto* joinedLines = joined->GetPolyLines();
    Check(joinedLines != nullptr, "Joined polyline array is null.");
    Check(joinedLines->GetNumberOfCells() == ExpectedJoinedPolylineCount,
          "The contiguous input lines were not joined into one polyline.");
    Check(joined->GetNumberOfOutputCells() ==
                  joined->GetNumberOfStrips() + ExpectedJoinedPolylineCount,
          "Joined-line output-cell count is incorrect.");

    const igIndex* joinedPointIds = nullptr;
    const int joinedPointCount = joinedLines->GetCellIds(0, joinedPointIds);
    Check(joinedPointIds != nullptr &&
                  joinedPointCount == ExpectedJoinedPointCount,
          "The joined model boundary has an unexpected point count.");
    std::unordered_set<igIndex> distinctPointIds;
    for (int i = 0; i < joinedPointCount; ++i) {
        if (i + 1 < joinedPointCount) {
            Check(joinedPointIds[i] != joinedPointIds[i + 1],
                  "The joined polyline contains a zero-length segment.");
        }
        distinctPointIds.insert(joinedPointIds[i]);
    }
    Check(distinctPointIds.size() ==
                  static_cast<std::size_t>(ExpectedJoinedPointCount),
          "The joined polyline does not contain every input point.");

    std::cout << "  Explicit input polylines: before join="
              << separateLines->GetNumberOfCells()
              << ", after join=" << joinedLines->GetNumberOfCells()
              << ", joined point IDs=" << joinedPointCount << '\n';
}

void TestPassThroughPolygonAttributes(const SurfaceMesh::Pointer& triangles) {
    Check(triangles->GetNumberOfPoints() >= 4,
          "Not enough points for the pass-through polygon test.");

    const igIndex pointIds[4]{0, 1, 2, 3};
    auto faces = CellArray::New();
    faces->AddCellIds(pointIds, 4);

    auto polygon = SurfaceMesh::New();
    polygon->SetName("TriangleStripPassThroughPolygon");
    polygon->SetPoints(triangles->GetPoints());
    polygon->SetFaces(faces);
    AddAttributeFixtures(polygon);

    auto filter = TriangleStripFilter::New();
    filter->SetInput(polygon);
    Check(filter->Execute(),
          "TriangleStripFilter failed for a pass-through polygon.");
    Check(filter->GetNumberOfStrips() == 0,
          "A non-triangle polygon unexpectedly generated a strip.");
    Check(filter->GetPassThroughPolys()->GetNumberOfCells() == 1,
          "The non-triangle polygon was not passed through.");
    Check(filter->GetNumberOfOutputCells() == 1,
          "Pass-through polygon output-cell count is incorrect.");

    auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    Check(output != nullptr && output->GetNumberOfFaces() == 1,
          "Pass-through polygon output is invalid.");
    CellArray::Pointer outputStrips;
    CellArray::Pointer outputStripSourceFaceIds;
    Check(TriangleStripFilter::ReadOutputStrips(
                  output, outputStrips, outputStripSourceFaceIds) &&
                  outputStrips->GetNumberOfCells() == 0 &&
                  outputStripSourceFaceIds->GetNumberOfCells() == 0,
          "Pass-through output contains unexpected triangle-strip topology.");
    Check(output->GetAttributeSet() != polygon->GetAttributeSet(),
          "Pass-through output reused the input AttributeSet container.");

    auto* outputAttributes = output->GetAttributeSet();
    const int pointAttributeId =
            outputAttributes->GetAttributeIndex(PointScalarName);
    const int cellAttributeId =
            outputAttributes->GetAttributeIndex(CellScalarName);
    Check(pointAttributeId >= 0 && cellAttributeId >= 0,
          "Pass-through output lost point or cell attributes.");
    auto& pointAttribute = outputAttributes->GetAttribute(pointAttributeId);
    auto& cellAttribute = outputAttributes->GetAttribute(cellAttributeId);
    Check(pointAttribute.pointer->GetNumberOfElements() ==
                  polygon->GetNumberOfPoints(),
          "Pass-through point attribute tuple count changed.");
    Check(cellAttribute.pointer->GetArrayType() == IG_IntArray &&
                  cellAttribute.pointer->GetNumberOfElements() == 1 &&
                  cellAttribute.pointer->GetElementValue(0, 0) == 1000.0,
          "Pass-through cell attribute was not copied from its source face.");
}

} // namespace

int main() {
    try {
        const std::filesystem::path modelPath{ModelFilePath};
        std::cout << "[RUN] TriangleStripFilter integration test\n"
                  << "  Model: " << modelPath.string() << '\n';
        Check(std::filesystem::exists(modelPath),
              "The requested VTK model does not exist.");

        auto input = FileIO::ReadFile(modelPath.string());
        Check(input != nullptr, "FileIO::ReadFile returned null.");
        Check(input->GetDataObjectType() == IG_UNSTRUCTURED_MESH,
              "The requested model is not an UnstructuredMesh.");

        auto unstructured = DynamicCast<UnstructuredMesh>(input);
        Check(unstructured != nullptr, "Failed to cast the input mesh.");
        Check(unstructured->GetNumberOfCells() > 0,
              "The input UnstructuredMesh contains no cells.");
        std::cout << "  Input: points=" << unstructured->GetNumberOfPoints()
                  << ", cells=" << unstructured->GetNumberOfCells() << '\n';

        auto triangles = ExtractAndTriangulate(input);

        std::cout << "[RUN] triangle-strip generation\n";
        TestTriangleStripGeneration(triangles);
        std::cout << "[PASS] triangle-strip generation\n";

        std::cout << "[RUN] ParaView output-cell comparison\n";
        TestParaViewOutputCellCount();
        std::cout << "[PASS] ParaView output-cell comparison\n";

        std::cout << "[RUN] contiguous-polyline joining\n";
        TestContiguousPolylineJoining(triangles);
        std::cout << "[PASS] contiguous-polyline joining\n";

        std::cout << "[RUN] pass-through polygon attributes\n";
        TestPassThroughPolygonAttributes(triangles);
        std::cout << "[PASS] pass-through polygon attributes\n";

        std::cout << "All TriangleStripFilter tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
