#include "iGameCellSizeFilter.h"
#include "iGameStructuredMesh.h"
#include <cmath>
#include <limits>

IGAME_NAMESPACE_BEGIN

namespace {
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
}

bool CellSizeFilter::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) {
        m_Message = "No input data, please load a model first.";
        return false;
    }

    auto input = m_Inputs->GetElement(0);
    if (!input) {
        m_Message = "Input data is null, please load a model first.";
        return false;
    }

    m_Cells = nullptr;
    m_Points = nullptr;
    m_Message.clear();
    m_ComputedCellCount = 0;
    UnstructuredMesh::Pointer unstructuredMesh = nullptr;
    int meshDim = -1; // -1: determine dimension from cell type (UnstructuredMesh only)
    DataObject::Pointer outputModel = nullptr;

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto sm = DynamicCast<SurfaceMesh>(input);
            if (!sm) { m_Message = "SurfaceMesh cast failed."; igError("CellSizeFilter: SurfaceMesh cast failed"); return false; }
            m_Cells = sm->GetFaces();
            m_Points = sm->GetPoints();
            meshDim = 2;
            // Deep-copy geometry & topology into an independent output node
            auto out = SurfaceMesh::New();
            auto pts = Points::New();
            pts->DeepCopy(sm->GetPoints());
            auto faces = CellArray::New();
            faces->Reset();
            faces->DeepCopy(sm->GetFaces());
            out->SetPoints(pts);
            out->SetFaces(faces);
            outputModel = out;
            break;
        }
        case IG_VOLUME_MESH: {
            auto vm = DynamicCast<VolumeMesh>(input);
            if (!vm) { m_Message = "VolumeMesh cast failed."; igError("CellSizeFilter: VolumeMesh cast failed"); return false; }
            m_Cells = vm->GetCells();
            m_Points = vm->GetPoints();
            meshDim = 3;
            auto out = VolumeMesh::New();
            auto pts = Points::New();
            pts->DeepCopy(vm->GetPoints());
            auto vols = CellArray::New();
            vols->Reset();
            vols->DeepCopy(vm->GetCells());
            out->SetPoints(pts);
            out->SetVolumes(vols);
            outputModel = out;
            break;
        }
        case IG_UNSTRUCTURED_MESH: {
            unstructuredMesh = DynamicCast<UnstructuredMesh>(input);
            if (!unstructuredMesh) { m_Message = "UnstructuredMesh cast failed."; igError("CellSizeFilter: UnstructuredMesh cast failed"); return false; }
            m_Cells = unstructuredMesh->GetCells();
            m_Points = unstructuredMesh->GetPoints();
            meshDim = -1;
            auto out = UnstructuredMesh::New();
            auto pts = Points::New();
            pts->DeepCopy(unstructuredMesh->GetPoints());
            out->SetPoints(pts);
            auto cells = CellArray::New();
            cells->Reset();
            cells->DeepCopy(unstructuredMesh->GetCells());
            auto types = UnsignedIntArray::New();
            types->DeepCopy(unstructuredMesh->GetCellTypes());
            out->SetCells(cells, types);
            outputModel = out;
            break;
        }
        case IG_STRUCTURED_MESH: {
            auto sm = DynamicCast<StructuredMesh>(input);
            if (!sm) { m_Message = "StructuredMesh cast failed."; igError("CellSizeFilter: StructuredMesh cast failed"); return false; }
            sm->GenStructuredCellConnectivities();
            m_Cells = sm->GetCells();
            m_Points = sm->GetPoints();
            meshDim = (int)sm->GetDimension();
            auto out = StructuredMesh::New();
            auto pts = Points::New();
            pts->DeepCopy(sm->GetPoints());
            out->SetPoints(pts);
            out->SetDimensionSize(sm->GetDimensionSize());
            out->GenStructuredCellConnectivities();
            outputModel = out;
            break;
        }
        default:
            m_Message = "Unsupported data type, only SurfaceMesh / VolumeMesh / UnstructuredMesh / StructuredMesh are supported.";
            igError("CellSizeFilter: unsupported data type {}", (int)input->GetDataObjectType());
            return false;
    }
    if (!outputModel) {
        m_Message = "Failed to create the output model.";
        return false;
    }
    if (!m_Cells) {
        m_Message = "No cells in the mesh, cannot compute cell size.";
        igDebug("CellSizeFilter: no cells");
        return false;
    }
    if (!m_Points) {
        m_Message = "No points in the mesh, cannot compute cell size.";
        igDebug("CellSizeFilter: no points");
        return false;
    }
    if (m_Cells->GetNumberOfCells() == 0) {
        m_Message = "Cell count is 0, cannot compute cell size.";
        return false;
    }

    igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};
    igIndex vNum = 0;
    igIndex cellNum = m_Cells->GetNumberOfCells();

    // Output three attributes for any mesh: Length / Area / Volume
    // Each cell fills only the attribute matching its dimension; non-matching
    // dimensions are NaN so "not applicable" is distinguishable from a real 0
    DoubleArray::Pointer lengthArray = DoubleArray::New();
    DoubleArray::Pointer areaArray = DoubleArray::New();
    DoubleArray::Pointer volumeArray = DoubleArray::New();
    lengthArray->SetName("Length");
    areaArray->SetName("Area");
    volumeArray->SetName("Volume");
    lengthArray->SetDimension(1);
    areaArray->SetDimension(1);
    volumeArray->SetDimension(1);
    lengthArray->Reserve(cellNum);
    areaArray->Reserve(cellNum);
    volumeArray->Reserve(cellNum);

    for (igIndex i = 0; i < cellNum; i++) {
        vNum = m_Cells->GetCellIds(i, vhs);
        IGenum cellType = IG_EMPTY_CELL;
        int dim = meshDim;
        if (dim < 0) {
            cellType = unstructuredMesh->GetCellType(i);
            dim = (int)Cell::GetCellDimension(cellType);
        }
        std::vector<Point> points;
        points.reserve(vNum);
        for (igIndex j = 0; j < vNum; j++) {
            points.push_back(m_Points->GetPoint(vhs[j]));
        }
        double size = ComputeCellSize(cellType, dim, points);
        lengthArray->AddValue(dim == 1 ? size : kNaN);
        areaArray->AddValue(dim == 2 ? size : kNaN);
        volumeArray->AddValue(dim == 3 ? size : kNaN);
    }

    m_ComputedCellCount = static_cast<int>(cellNum);

    // Write results into the independent output node (deep copy), input stays untouched
    auto outAttrSet = outputModel->GetAttributeSet();
    // Clone the input attribute set too, so the output node keeps the original attributes
    if (input->GetAttributeSet()) {
        outAttrSet->DeepCopy(input->GetAttributeSet());
    }
    outAttrSet->AddAttribute(IG_SCALAR, IG_CELL, lengthArray);
    outAttrSet->AddAttribute(IG_SCALAR, IG_CELL, areaArray);
    outAttrSet->AddAttribute(IG_SCALAR, IG_CELL, volumeArray);
    outputModel->SetName(input->GetName() + "_CellSize");
    this->SetOutput(outputModel);
    m_Message = "Computed cell size for " + std::to_string(cellNum) + " cells.";
    return true;
}

double CellSizeFilter::ComputeCellSize(IGenum cellType, int dim, const std::vector<Point>& points) {
    int vNum = (int)points.size();
    // All "cannot compute" paths return NaN: not applicable, distinguishable from a real 0
    if (cellType != IG_EMPTY_CELL) {
        switch (cellType) {
            case IG_LINE:       return ComputeLineLength(points);
            case IG_TRIANGLE:   return ComputeTriangleArea(points);
            case IG_QUAD:        return ComputeQuadArea(points);
            case IG_POLYGON:    return ComputePolygonArea(points);
            case IG_TETRA:      return ComputeTetVolume(points);
            case IG_HEXAHEDRON: return ComputeHexVolume(points);
            case IG_PYRAMID:    return ComputePyramidVolume(points);
            case IG_PRISM:      return ComputePrismVolume(points);
            default:            return kNaN;  // variable-length/quadratic/Lagrange cells
        }
    }
    // Without cellType (SurfaceMesh/VolumeMesh) dispatch by dim+vNum, these meshes contain no variable-length cells
    switch (dim) {
        case 1: if (vNum >= 2) { return ComputeLineLength(points); } return kNaN;
        case 2: if (vNum == 3) { return ComputeTriangleArea(points); }
                if (vNum == 4) { return ComputeQuadArea(points); }
                if (vNum > 4)  { return ComputePolygonArea(points); }
                return kNaN;
        case 3: if (vNum == 4) { return ComputeTetVolume(points); }
                if (vNum == 5) { return ComputePyramidVolume(points); }
                if (vNum == 6) { return ComputePrismVolume(points); }
                if (vNum == 8) { return ComputeHexVolume(points); }
                return kNaN;
        default: return kNaN;
    }
}

double CellSizeFilter::ComputeLineLength(const std::vector<Point>& points) {
    // Line/polyline cell: sum adjacent segment lengths, 2-point line degenerates to single segment
    double len = 0.0;
    for (size_t i = 0; i + 1 < points.size(); i++) {
        len += (points[i + 1] - points[i]).norm();
    }
    return len;
}

double CellSizeFilter::ComputeTriangleArea(const std::vector<Point>& points) {
    Point edge1 = points[1] - points[0];
    Point edge2 = points[2] - points[0];
    return edge1.cross(edge2).norm() / 2.0;
}

double CellSizeFilter::ComputeQuadArea(const std::vector<Point>& points) {
    double a1 = (points[1] - points[0]).cross(points[2] - points[0]).norm() / 2.0;
    double a2 = (points[2] - points[0]).cross(points[3] - points[0]).norm() / 2.0;
    return a1 + a2;
}

double CellSizeFilter::ComputePolygonArea(const std::vector<Point>& points) {
    double area = 0.0;
    for (size_t i = 1; i + 1 < points.size(); i++) {
        Point e1 = points[i] - points[0];
        Point e2 = points[i + 1] - points[0];
        area += e1.cross(e2).norm() / 2.0;
    }
    return area;
}

double CellSizeFilter::ComputeTetSignedVolume(const Point& p0, const Point& p1,
                                              const Point& p2, const Point& p3) {
    Point a = p1 - p0;
    Point b = p2 - p0;
    Point c = p3 - p0;
    return a.cross(b).dot(c) / 6.0;
}

double CellSizeFilter::ComputeTetVolume(const std::vector<Point>& points) {
    return std::abs(ComputeTetSignedVolume(points[0], points[1], points[2], points[3]));
}

double CellSizeFilter::ComputePyramidVolume(const std::vector<Point>& points) {
    // Base quad 0,1,2,3; apex 4. Split into two tetrahedra
    double v1 = std::abs(ComputeTetSignedVolume(points[0], points[1], points[2], points[4]));
    double v2 = std::abs(ComputeTetSignedVolume(points[0], points[2], points[3], points[4]));
    return v1 + v2;
}

double CellSizeFilter::ComputePrismVolume(const std::vector<Point>& points) {
    // Base triangle 0,1,2; top 3,4,5 (3~0,4~1,5~2). Staircase decomposition into 3 tetrahedra
    double v1 = std::abs(ComputeTetSignedVolume(points[0], points[1], points[2], points[3]));
    double v2 = std::abs(ComputeTetSignedVolume(points[1], points[2], points[3], points[4]));
    double v3 = std::abs(ComputeTetSignedVolume(points[2], points[3], points[4], points[5]));
    return v1 + v2 + v3;
}

double CellSizeFilter::ComputeHexVolume(const std::vector<Point>& points) {
    auto P0 = points[0], P1 = points[1], P2 = points[2], P3 = points[3];
    auto P4 = points[4], P5 = points[5], P6 = points[6], P7 = points[7];
    auto X1 = (P1 - P0) + (P2 - P3) + (P5 - P4) + (P6 - P7);
    auto X2 = (P3 - P0) + (P2 - P1) + (P7 - P4) + (P6 - P5);
    auto X3 = (P4 - P0) + (P5 - P1) + (P6 - P2) + (P7 - P3);
    return std::abs(X1.dot(X2.cross(X3)) / 64.0);
}

IGAME_NAMESPACE_END
