#ifndef iGameDataCodecFeatureRemap_h
#define iGameDataCodecFeatureRemap_h

#include <DataCodec/Filter/Adapter/iGameBlockTreeAdapter.h>
#include <DataCodec/Common/Views/ArrayViews.h>
#include <DataCodec/Test/Common/DataCodecTestResult.h>
#include <DataCodec/Filter/Test/Common/iGameIGDCFileRoundTrip.h>
#include <DataCodec/Log/Capture/RemapOrderCapture.h>

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>
namespace iGame::datacodec_test::feature_remap {
using namespace ::datacodec;
using namespace ::datacodec::test;

using iGame::AttributeSet;
using iGame::CellArray;
using iGame::DataObject;
using iGame::FloatArray;
using iGame::Points;
using iGame::SurfaceMesh;
using iGame::VolumeMesh;
using datacodec::ArrayLayout;
using datacodec::BlockPath;
using datacodec::IEncodeAdapter;
using datacodec::IEncodeAttrView;
using datacodec::IndexArrayView;
using datacodec::IndexType;
using datacodec::IndexValueType;
using datacodec::PolyhedronFaceTableView;
using datacodec::TopologyView;
using iGame::iGameBlockTreeAdapter;
using iGame::datacodec_test::DecodeFileToDataObject;
using iGame::datacodec_test::EncodeDataObjectToFile;
using datacodec::log::RemapOrderCapture;
using datacodec::test::Require;
using datacodec::test::TestResult;

constexpr const char* kPointIdentityName = "remap_point_identity";
constexpr const char* kCellIdentityName = "remap_cell_identity";

struct PointSemantic {
    IndexType id{0u};
    std::array<double, 3u> coordinate{};
};

struct CellSemantic {
    IndexType id{0u};
    std::vector<IndexType> pointIds;
    std::vector<std::vector<IndexType>> facePointIds;
};

struct MeshSemanticSignature {
    std::vector<PointSemantic> points;
    std::vector<CellSemantic> cells;
};

inline void PrintResult(const TestResult& result) {
    for (const auto& failure : result.failures) {
        std::cerr << failure.check << ": " << failure.message << '\n';
    }
    for (const auto& diagnostic : result.diagnostics) {
        std::cerr << diagnostic << '\n';
    }
}

inline void AppendRemapFeatureStatus(TestResult& target, const TestResult& source) {
    if (!source.passed) {
        target.passed = false;
    }
    target.failures.insert(target.failures.end(), source.failures.begin(), source.failures.end());
    target.diagnostics.insert(target.diagnostics.end(), source.diagnostics.begin(), source.diagnostics.end());
}

inline FloatArray::Pointer BuildIdentityArray(const std::string& name, const std::size_t count) {
    auto array = FloatArray::New();
    array->SetName(name);
    array->SetDimension(1);
    for (std::size_t index = 0u; index < count; ++index) {
        const float value = static_cast<float>(index);
        array->AddElement(&value);
    }
    return array;
}

inline DataObject::Pointer BuildRemapSemanticSurfaceObject() {
    auto mesh = SurfaceMesh::New();
    mesh->SetName("remap_semantic_surface");

    auto points = Points::New();
    points->AddPoint(2.0f, 0.0f, 0.0f);
    points->AddPoint(0.0f, 0.0f, 0.0f);
    points->AddPoint(1.0f, 1.0f, 0.0f);
    points->AddPoint(0.0f, 1.0f, 0.0f);
    points->AddPoint(1.0f, 0.0f, 0.0f);
    points->AddPoint(2.0f, 1.0f, 0.5f);
    mesh->SetPoints(points);

    auto faces = CellArray::New();
    const igIndex cell0[] = {0, 5, 2};
    const igIndex cell1[] = {1, 4, 3};
    const igIndex cell2[] = {4, 0, 2};
    const igIndex cell3[] = {4, 2, 3};
    faces->AddCellIds(cell0, 3);
    faces->AddCellIds(cell1, 3);
    faces->AddCellIds(cell2, 3);
    faces->AddCellIds(cell3, 3);
    mesh->SetFaces(faces);

    auto attributes = AttributeSet::New();
    attributes->AddAttribute(
        IG_SCALAR,
        IG_POINT,
        BuildIdentityArray(kPointIdentityName, 6u));
    attributes->AddAttribute(
        IG_SCALAR,
        IG_CELL,
        BuildIdentityArray(kCellIdentityName, 4u));
    mesh->SetAttributeSet(attributes);

    return mesh;
}

inline DataObject::Pointer BuildRemapSemanticPolyhedronObject() {
    auto mesh = VolumeMesh::New();
    mesh->SetName("remap_semantic_polyhedron");

    auto points = Points::New();
    points->AddPoint(1.0f, 0.0f, 0.0f);
    points->AddPoint(2.0f, 0.0f, 0.0f);
    points->AddPoint(2.0f, 1.0f, 0.0f);
    points->AddPoint(1.0f, 1.0f, 0.0f);
    points->AddPoint(1.0f, 0.0f, 1.0f);
    points->AddPoint(2.0f, 0.0f, 1.0f);
    points->AddPoint(2.0f, 1.0f, 1.0f);
    points->AddPoint(1.0f, 1.0f, 1.0f);
    points->AddPoint(0.0f, 0.0f, 0.0f);
    points->AddPoint(0.0f, 1.0f, 0.0f);
    points->AddPoint(0.0f, 0.0f, 1.0f);
    points->AddPoint(0.0f, 1.0f, 1.0f);
    mesh->SetPoints(points);

    auto faces = CellArray::New();
    const auto addFace = [&faces](std::initializer_list<igIndex> ids) {
        std::vector<igIndex> values(ids);
        faces->AddCellIds(values.data(), static_cast<int>(values.size()));
    };
    addFace({0, 1, 2, 3});
    addFace({4, 5, 6, 7});
    addFace({0, 1, 5, 4});
    addFace({1, 2, 6, 5});
    addFace({2, 3, 7, 6});
    addFace({3, 0, 4, 7});
    addFace({8, 0, 3, 9});
    addFace({10, 4, 7, 11});
    addFace({8, 0, 4, 10});
    addFace({8, 9, 11, 10});
    addFace({9, 3, 7, 11});

    auto volumeFaces = CellArray::New();
    const igIndex rightCellFaces[] = {0, 1, 2, 3, 4, 5};
    const igIndex leftCellFaces[] = {6, 7, 8, 9, 10, 5};
    volumeFaces->AddCellIds(rightCellFaces, 6);
    volumeFaces->AddCellIds(leftCellFaces, 6);
    mesh->InitVolumesWithPolyhedron(faces, volumeFaces);

    auto attributes = AttributeSet::New();
    attributes->AddAttribute(
        IG_SCALAR,
        IG_POINT,
        BuildIdentityArray(kPointIdentityName, 12u));
    attributes->AddAttribute(
        IG_SCALAR,
        IG_CELL,
        BuildIdentityArray(kCellIdentityName, 2u));
    mesh->SetAttributeSet(attributes);

    return mesh;
}

inline const IEncodeAttrView* FindPointAttribute(const IEncodeAdapter& adapter, const std::string& name) {
    for (std::size_t index = 0u; index < adapter.GetNumberOfPointAttrs(); ++index) {
        const auto& attr = adapter.GetPointAttr(index);
        if (attr.GetName() == name) {
            return &attr;
        }
    }
    return nullptr;
}

inline const IEncodeAttrView* FindCellAttribute(const IEncodeAdapter& adapter, const std::string& name) {
    for (std::size_t index = 0u; index < adapter.GetNumberOfCellAttrs(); ++index) {
        const auto& attr = adapter.GetCellAttr(index);
        if (attr.GetName() == name) {
            return &attr;
        }
    }
    return nullptr;
}

inline bool ReadIdentityValue(
    const IEncodeAttrView& attr,
    const std::size_t tupleIndex,
    IndexType& output,
    TestResult& result,
    const std::string& checkPath) {
    Require(result, attr.GetComponentCount() == 1, checkPath + ".componentCount", "identity attribute must be scalar");
    Require(result, tupleIndex < attr.GetElementCount(), checkPath + ".tupleRange", "identity tuple is out of range");
    if (!result.passed) {
        return false;
    }

    double value = 0.0;
    attr.GetTuple(tupleIndex, &value);
    if (!std::isfinite(value) ||
        value < 0.0 ||
        value > static_cast<double>(std::numeric_limits<IndexType>::max())) {
        result.AddFailure(checkPath + ".value", "identity value is outside IndexType range");
        return false;
    }

    const auto rounded = std::llround(value);
    if (std::abs(value - static_cast<double>(rounded)) > 1.0e-5) {
        result.AddFailure(checkPath + ".integer", "identity value is not integral");
        return false;
    }
    output = static_cast<IndexType>(rounded);
    return true;
}

inline bool ReadIndexValue(
    const IndexArrayView& view,
    const std::size_t index,
    IndexType& output,
    std::string* error) {
    output = 0u;
    if (index >= view.count) {
        if (error != nullptr) {
            *error = "index view read is out of range";
        }
        return false;
    }
    if (view.layout == ArrayLayout::GetterOnly) {
        if (view.getIndex == nullptr) {
            if (error != nullptr) {
                *error = "index view getter is null";
            }
            return false;
        }
        const auto value = view.getIndex(view.userData, index);
        if (value > static_cast<std::uint64_t>(std::numeric_limits<IndexType>::max())) {
            if (error != nullptr) {
                *error = "index view getter value exceeds IndexType range";
            }
            return false;
        }
        output = static_cast<IndexType>(value);
        return true;
    }
    if (view.layout != ArrayLayout::CompactAOS && view.layout != ArrayLayout::StridedAOS) {
        if (error != nullptr) {
            *error = "index view layout is unsupported";
        }
        return false;
    }
    if (view.data == nullptr) {
        if (error != nullptr) {
            *error = "index view data is null";
        }
        return false;
    }

    const auto valueSize = view.ValueSize();
    const auto stride = view.layout == ArrayLayout::StridedAOS
        ? view.strideBytes
        : valueSize;
    if (stride < valueSize) {
        if (error != nullptr) {
            *error = "index view stride is too small";
        }
        return false;
    }
    const auto* source = static_cast<const std::uint8_t*>(view.data) + index * stride;
    std::uint64_t value = 0u;
    if (view.indexType == IndexValueType::UInt64) {
        std::memcpy(&value, source, sizeof(std::uint64_t));
    } else {
        std::uint32_t value32 = 0u;
        std::memcpy(&value32, source, sizeof(std::uint32_t));
        value = value32;
    }
    if (value > static_cast<std::uint64_t>(std::numeric_limits<IndexType>::max())) {
        if (error != nullptr) {
            *error = "index view value exceeds IndexType range";
        }
        return false;
    }
    output = static_cast<IndexType>(value);
    return true;
}

inline bool AppendStandardCellSignatures(
    const IEncodeAdapter& adapter,
    const IEncodeAttrView& cellIdentity,
    MeshSemanticSignature& signature,
    TestResult& result,
    const std::string& checkPath) {
    TopologyView topology;
    Require(result, adapter.BuildTopologyView(topology), checkPath + ".topology", "failed to build topology view");
    Require(
        result,
        topology.cellCount == adapter.GetNumberOfCells(),
        checkPath + ".topology.cellCount",
        "topology cell count mismatch");
    Require(
        result,
        topology.connectivity.IsValid(),
        checkPath + ".topology.connectivity",
        "topology connectivity view is invalid");
    if (!result.passed) {
        return false;
    }

    signature.cells.reserve(topology.cellCount);
    for (std::size_t cellIndex = 0u; cellIndex < topology.cellCount; ++cellIndex) {
        CellSemantic cell;
        if (!ReadIdentityValue(
                cellIdentity,
                cellIndex,
                cell.id,
                result,
                checkPath + ".cell[" + std::to_string(cellIndex) + "].identity")) {
            return false;
        }

        std::size_t begin = 0u;
        std::size_t end = 0u;
        if (topology.fixedCellSizeEnabled) {
            begin = cellIndex * static_cast<std::size_t>(topology.fixedCellSize);
            end = begin + static_cast<std::size_t>(topology.fixedCellSize);
        } else {
            std::string offsetError;
            IndexType beginValue = 0u;
            IndexType endValue = 0u;
            if (!ReadIndexValue(topology.offsets, cellIndex, beginValue, &offsetError) ||
                !ReadIndexValue(topology.offsets, cellIndex + 1u, endValue, &offsetError)) {
                result.AddFailure(
                    checkPath + ".cell[" + std::to_string(cellIndex) + "].offset",
                    offsetError.empty() ? "failed to read cell offsets" : offsetError);
                return false;
            }
            begin = static_cast<std::size_t>(beginValue);
            end = static_cast<std::size_t>(endValue);
        }

        Require(result, begin <= end, checkPath + ".cell[" + std::to_string(cellIndex) + "].range", "cell range is invalid");
        Require(
            result,
            end <= topology.connectivity.count,
            checkPath + ".cell[" + std::to_string(cellIndex) + "].rangeEnd",
            "cell connectivity range exceeds buffer");
        if (!result.passed) {
            return false;
        }

        cell.pointIds.reserve(end - begin);
        for (std::size_t cursor = begin; cursor < end; ++cursor) {
            std::string connectivityError;
            IndexType pointIndex = 0u;
            if (!ReadIndexValue(topology.connectivity, cursor, pointIndex, &connectivityError)) {
                result.AddFailure(
                    checkPath + ".cell[" + std::to_string(cellIndex) + "].connectivity",
                    connectivityError.empty() ? "failed to read connectivity" : connectivityError);
                return false;
            }
            if (static_cast<std::size_t>(pointIndex) >= signature.points.size()) {
                result.AddFailure(
                    checkPath + ".cell[" + std::to_string(cellIndex) + "].pointRange",
                    "cell references a point outside the mesh");
                return false;
            }
            cell.pointIds.push_back(signature.points[static_cast<std::size_t>(pointIndex)].id);
        }
        signature.cells.push_back(std::move(cell));
    }
    return true;
}

inline bool AppendPolyhedronCellSignatures(
    const IEncodeAdapter& adapter,
    const IEncodeAttrView& cellIdentity,
    MeshSemanticSignature& signature,
    TestResult& result,
    const std::string& checkPath) {
    PolyhedronFaceTableView faceTable;
    Require(
        result,
        adapter.BuildPolyhedronFaceTableView(faceTable),
        checkPath + ".polyhedronFaceTable",
        "failed to build polyhedron face table");
    Require(
        result,
        faceTable.cellCount == adapter.GetNumberOfCells(),
        checkPath + ".polyhedronFaceTable.cellCount",
        "polyhedron face table cell count mismatch");
    Require(
        result,
        faceTable.pointCount == signature.points.size(),
        checkPath + ".polyhedronFaceTable.pointCount",
        "polyhedron face table point count mismatch");
    if (!result.passed) {
        return false;
    }

    signature.cells.reserve(faceTable.cellCount);
    for (std::size_t cellIndex = 0u; cellIndex < faceTable.cellCount; ++cellIndex) {
        CellSemantic cell;
        if (!ReadIdentityValue(
                cellIdentity,
                cellIndex,
                cell.id,
                result,
                checkPath + ".polyhedronCell[" + std::to_string(cellIndex) + "].identity")) {
            return false;
        }

        std::string offsetError;
        IndexType faceBeginValue = 0u;
        IndexType faceEndValue = 0u;
        if (!ReadIndexValue(faceTable.cellFaceOffsets, cellIndex, faceBeginValue, &offsetError) ||
            !ReadIndexValue(faceTable.cellFaceOffsets, cellIndex + 1u, faceEndValue, &offsetError)) {
            result.AddFailure(
                checkPath + ".polyhedronCell[" + std::to_string(cellIndex) + "].faceOffset",
                offsetError.empty() ? "failed to read polyhedron cell face offsets" : offsetError);
            return false;
        }
        const auto faceBegin = static_cast<std::size_t>(faceBeginValue);
        const auto faceEnd = static_cast<std::size_t>(faceEndValue);
        Require(
            result,
            faceBegin <= faceEnd && faceEnd <= faceTable.cellFaceIds.count,
            checkPath + ".polyhedronCell[" + std::to_string(cellIndex) + "].faceRange",
            "polyhedron cell face range is invalid");
        if (!result.passed) {
            return false;
        }

        for (std::size_t faceCursor = faceBegin; faceCursor < faceEnd; ++faceCursor) {
            std::string faceError;
            IndexType faceIndexValue = 0u;
            if (!ReadIndexValue(faceTable.cellFaceIds, faceCursor, faceIndexValue, &faceError)) {
                result.AddFailure(
                    checkPath + ".polyhedronCell[" + std::to_string(cellIndex) + "].faceId",
                    faceError.empty() ? "failed to read polyhedron cell face id" : faceError);
                return false;
            }
            const auto faceIndex = static_cast<std::size_t>(faceIndexValue);
            if (faceIndex >= faceTable.faceCount) {
                result.AddFailure(
                    checkPath + ".polyhedronCell[" + std::to_string(cellIndex) + "].faceIdRange",
                    "polyhedron cell references a face outside the face table");
                return false;
            }

            IndexType vertexBeginValue = 0u;
            IndexType vertexEndValue = 0u;
            if (!ReadIndexValue(faceTable.faceVertexOffsets, faceIndex, vertexBeginValue, &faceError) ||
                !ReadIndexValue(faceTable.faceVertexOffsets, faceIndex + 1u, vertexEndValue, &faceError)) {
                result.AddFailure(
                    checkPath + ".polyhedronCell[" + std::to_string(cellIndex) + "].faceVertexOffset",
                    faceError.empty() ? "failed to read polyhedron face vertex offsets" : faceError);
                return false;
            }
            const auto vertexBegin = static_cast<std::size_t>(vertexBeginValue);
            const auto vertexEnd = static_cast<std::size_t>(vertexEndValue);
            Require(
                result,
                vertexBegin <= vertexEnd && vertexEnd <= faceTable.faceVertexIds.count,
                checkPath + ".polyhedronCell[" + std::to_string(cellIndex) + "].faceVertexRange",
                "polyhedron face vertex range is invalid");
            Require(
                result,
                vertexEnd - vertexBegin >= 3u,
                checkPath + ".polyhedronCell[" + std::to_string(cellIndex) + "].faceVertexCount",
                "polyhedron face should have at least three vertices");
            if (!result.passed) {
                return false;
            }

            std::vector<IndexType> facePointIds;
            facePointIds.reserve(vertexEnd - vertexBegin);
            for (std::size_t vertexCursor = vertexBegin; vertexCursor < vertexEnd; ++vertexCursor) {
                IndexType pointIndexValue = 0u;
                if (!ReadIndexValue(faceTable.faceVertexIds, vertexCursor, pointIndexValue, &faceError)) {
                    result.AddFailure(
                        checkPath + ".polyhedronCell[" + std::to_string(cellIndex) + "].faceVertex",
                        faceError.empty() ? "failed to read polyhedron face vertex id" : faceError);
                    return false;
                }
                const auto pointIndex = static_cast<std::size_t>(pointIndexValue);
                if (pointIndex >= signature.points.size()) {
                    result.AddFailure(
                        checkPath + ".polyhedronCell[" + std::to_string(cellIndex) + "].pointRange",
                        "polyhedron face references a point outside the mesh");
                    return false;
                }
                const auto pointId = signature.points[pointIndex].id;
                facePointIds.push_back(pointId);
                cell.pointIds.push_back(pointId);
            }
            std::sort(facePointIds.begin(), facePointIds.end());
            cell.facePointIds.push_back(std::move(facePointIds));
        }

        std::sort(cell.pointIds.begin(), cell.pointIds.end());
        cell.pointIds.erase(std::unique(cell.pointIds.begin(), cell.pointIds.end()), cell.pointIds.end());
        std::sort(cell.facePointIds.begin(), cell.facePointIds.end());
        signature.cells.push_back(std::move(cell));
    }
    return true;
}

inline bool BuildMeshSemanticSignature(
    const DataObject::Pointer& object,
    MeshSemanticSignature& signature,
    TestResult& result,
    const std::string& checkPath) {
    signature = {};
    iGameBlockTreeAdapter tree(object);
    Require(result, tree.GetLeaves().size() == 1u, checkPath + ".leafCount", "remap semantic test expects one leaf");
    if (!result.passed || tree.GetLeaves().empty()) {
        return false;
    }

    auto adapter = tree.GetLeaf(tree.GetLeaves().front().path);
    Require(result, adapter != nullptr, checkPath + ".adapter", "leaf adapter is null");
    if (adapter == nullptr) {
        return false;
    }

    const auto* pointIdentity = FindPointAttribute(*adapter, kPointIdentityName);
    const auto* cellIdentity = FindCellAttribute(*adapter, kCellIdentityName);
    Require(result, pointIdentity != nullptr, checkPath + ".pointIdentity", "point identity attribute is missing");
    Require(result, cellIdentity != nullptr, checkPath + ".cellIdentity", "cell identity attribute is missing");
    if (pointIdentity == nullptr || cellIdentity == nullptr) {
        return false;
    }

    signature.points.reserve(adapter->GetNumberOfPoints());
    for (std::size_t pointIndex = 0u; pointIndex < adapter->GetNumberOfPoints(); ++pointIndex) {
        PointSemantic point;
        if (!ReadIdentityValue(
                *pointIdentity,
                pointIndex,
                point.id,
                result,
                checkPath + ".point[" + std::to_string(pointIndex) + "].identity")) {
            return false;
        }
        double coordinate[3]{0.0, 0.0, 0.0};
        adapter->GetPoint(pointIndex, coordinate);
        point.coordinate = {coordinate[0], coordinate[1], coordinate[2]};
        signature.points.push_back(point);
    }

    if (adapter->IsPolyhedronMesh()) {
        return AppendPolyhedronCellSignatures(*adapter, *cellIdentity, signature, result, checkPath);
    }
    return AppendStandardCellSignatures(*adapter, *cellIdentity, signature, result, checkPath);
}

inline bool NearlySameCoordinate(const std::array<double, 3u>& left, const std::array<double, 3u>& right) {
    for (std::size_t component = 0u; component < left.size(); ++component) {
        if (std::abs(left[component] - right[component]) > 1.0e-5) {
            return false;
        }
    }
    return true;
}

inline bool HasCapturedOrder(const std::unordered_map<BlockPath, std::vector<IndexType>>& orders) {
    return std::any_of(
        orders.begin(),
        orders.end(),
        [](const auto& entry) {
            return !entry.second.empty();
        });
}

inline void CompareSemanticSignatures(
    const MeshSemanticSignature& expected,
    const MeshSemanticSignature& actual,
    TestResult& result,
    const std::string& checkPath) {
    Require(result, expected.points.size() == actual.points.size(), checkPath + ".pointCount", "point count mismatch");
    Require(result, expected.cells.size() == actual.cells.size(), checkPath + ".cellCount", "cell count mismatch");
    if (!result.passed) {
        return;
    }

    std::unordered_map<IndexType, PointSemantic> expectedPoints;
    expectedPoints.reserve(expected.points.size());
    for (const auto& point : expected.points) {
        const auto inserted = expectedPoints.emplace(point.id, point).second;
        Require(result, inserted, checkPath + ".sourcePointUnique", "source point identity is duplicated");
    }
    for (const auto& point : actual.points) {
        const auto it = expectedPoints.find(point.id);
        Require(result, it != expectedPoints.end(), checkPath + ".pointIdentity", "decoded point identity is unknown");
        if (it == expectedPoints.end()) {
            continue;
        }
        Require(
            result,
            NearlySameCoordinate(it->second.coordinate, point.coordinate),
            checkPath + ".pointBinding",
            "decoded point attribute is not bound to its original coordinate");
    }

    std::unordered_map<IndexType, CellSemantic> expectedCells;
    expectedCells.reserve(expected.cells.size());
    for (const auto& cell : expected.cells) {
        const auto inserted = expectedCells.emplace(cell.id, cell).second;
        Require(result, inserted, checkPath + ".sourceCellUnique", "source cell identity is duplicated");
    }
    for (const auto& cell : actual.cells) {
        const auto it = expectedCells.find(cell.id);
        Require(result, it != expectedCells.end(), checkPath + ".cellIdentity", "decoded cell identity is unknown");
        if (it == expectedCells.end()) {
            continue;
        }
        if (!it->second.facePointIds.empty() || !cell.facePointIds.empty()) {
            Require(
                result,
                it->second.facePointIds == cell.facePointIds,
                checkPath + ".polyhedronCellBinding",
                "decoded polyhedron cell attribute is not bound to the original face topology");
            continue;
        }
        Require(
            result,
            it->second.pointIds == cell.pointIds,
            checkPath + ".cellBinding",
            "decoded cell attribute is not bound to the original cell topology");
    }
}

inline std::filesystem::path MakeRemapOutputDirectory(const std::string& caseName) {
    const auto salt = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
        ("igame_datacodec_remap_" + caseName + "_" + std::to_string(salt));
}

inline void RemoveRemapOutput(const std::filesystem::path& directory) {
    std::error_code errorCode;
    std::filesystem::remove_all(directory, errorCode);
}

inline DataCodecEncodeConfigurationParams MakeRemapSemanticConfiguration() {
    auto configuration = MakeEncodeConfigurationParams(
        DataCodecEncodeOptions{
            .tier = DataCodecEncodeTier::Balanced,
            .enableCompressionEnhancement = true,
        });
    auto& params = configuration.controlParams;
    params.attrReference.enabled = false;
    params.geometryReference.enabled = false;
    params.topologyReference.enabled = false;
    return configuration;
}

inline bool TestRemapPreservesMeshSemantics(
    const std::string& caseName,
    const DataObject::Pointer& source) {
    TestResult result;
    const auto checkPrefix = "remap." + caseName;
    const auto outputDirectory = MakeRemapOutputDirectory(caseName);

    std::error_code errorCode;
    std::filesystem::create_directories(outputDirectory, errorCode);
    Require(result, !errorCode, checkPrefix + ".outputDirectory", "failed to create remap output directory");
    if (!result.passed) {
        PrintResult(result);
        return false;
    }

    MeshSemanticSignature expectedSignature;
    if (!BuildMeshSemanticSignature(source, expectedSignature, result, checkPrefix + ".source")) {
        PrintResult(result);
        RemoveRemapOutput(outputDirectory);
        return false;
    }

    auto configuration = MakeRemapSemanticConfiguration();
    RemapOrderCapture remapOrders;
    const auto outputHint = outputDirectory / ("remap_" + caseName + ".igc");
    const auto encodeResult = EncodeDataObjectToFile(
        source,
        outputHint,
        std::move(configuration),
        &remapOrders);
    AppendRemapFeatureStatus(result, encodeResult.status);
    if (!result.passed) {
        PrintResult(result);
        RemoveRemapOutput(outputDirectory);
        return false;
    }

    const auto orderSnapshot = remapOrders.TakeSnapshot();
    Require(
        result,
        HasCapturedOrder(orderSnapshot.pointOrders),
        checkPrefix + ".pointOrderCaptured",
        "point remap order was not captured");
    Require(
        result,
        HasCapturedOrder(orderSnapshot.cellOrders),
        checkPrefix + ".cellOrderCaptured",
        "cell remap order was not captured");
    if (!result.passed) {
        PrintResult(result);
        RemoveRemapOutput(outputDirectory);
        return false;
    }

    const auto decodeResult = DecodeFileToDataObject(encodeResult.encodedFile);
    AppendRemapFeatureStatus(result, decodeResult.status);
    if (!result.passed) {
        PrintResult(result);
        RemoveRemapOutput(outputDirectory);
        return false;
    }

    MeshSemanticSignature actualSignature;
    if (BuildMeshSemanticSignature(decodeResult.decodedObject, actualSignature, result, checkPrefix + ".decoded")) {
        CompareSemanticSignatures(expectedSignature, actualSignature, result, checkPrefix + ".semantic");
    }
    PrintResult(result);
    RemoveRemapOutput(outputDirectory);
    return result.passed;
}

} // namespace iGame::datacodec_test::feature_remap

namespace iGame::datacodec_test {
using namespace ::datacodec;
using namespace ::datacodec::test;

inline int RunDataCodecFeatureRemap() {
    bool passed = true;
    if (!feature_remap::TestRemapPreservesMeshSemantics(
            "surface",
            feature_remap::BuildRemapSemanticSurfaceObject())) {
        passed = false;
    }
    if (!feature_remap::TestRemapPreservesMeshSemantics(
            "polyhedron",
            feature_remap::BuildRemapSemanticPolyhedronObject())) {
        passed = false;
    }
    if (!passed) {
        return 1;
    }
    std::cout << "DataCodec remap semantic feature tests passed\n";
    return 0;
}

} // namespace iGame::datacodec_test

#endif
