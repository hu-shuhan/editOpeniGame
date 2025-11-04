#include "iGameLagrangeUnstructuredMesh.h"
#include "iGameDataObject.h"
#include "iGameVolume.h"
#include <cmath>
#include <map>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace
{
Point operator*(double scalar, const Point& p) { return Point(p[0] * scalar, p[1] * scalar, p[2] * scalar); }
Point& operator+=(Point& p1, const Point& p2) {
    p1[0] += p2[0];
    p1[1] += p2[1];
    p1[2] += p2[2];
    return p1;
}

//生成p阶拉格朗日单元的1D参考坐标
std::vector<double> GetLagrangeNodeCoordinates1D(int order) {
    std::vector<double> coords(order + 1);
    for (int i = 0; i <= order; ++i) { coords[i] = -1.0 + 2.0 * static_cast<double>(i) / order; }
    return coords;
}

double CardinalPolynomial(double L, int p, int i) {
    if (i == 0) { return 1.0; }
    double res = 1.0;
    for (int k = 0; k < i; ++k) { res *= (p * L - k); }
    double factorial_i = 1.0;
    for (int k = 2; k <= i; ++k) { factorial_i *= k; }
    return res / factorial_i;
}

// 计算1D拉格朗日基函数 L_i(u) 的值
// u: 要求值的坐标点
// i: 第 i 个基函数 (对应第 i 个节点)
// node_coords: 所有节点的坐标列表
double LagrangeBasis1D(double u, int i, const std::vector<double>& node_coords) {
    double result = 1.0;
    double ui = node_coords[i];
    for (size_t j = 0; j < node_coords.size(); ++j) {
        if (i == j) continue;
        double uj = node_coords[j];
        result *= (u - uj) / (ui - uj);
    }
    return result;
}


Point InterpolateLagrangeQuad(const std::vector<Point>& cp, int order, double u, double v) {
    // 检查控制点数量是否与纯拉格朗日单元的公式匹配
    size_t expected_points = (size_t) (order + 1) * (order + 1);
    if (cp.size() < expected_points) return {};

    auto u_coords = GetLagrangeNodeCoordinates1D(order);
    auto v_coords = GetLagrangeNodeCoordinates1D(order);
    std::vector<double> u_basis(order + 1), v_basis(order + 1);
    for (int i = 0; i <= order; ++i) {
        u_basis[i] = LagrangeBasis1D(u, i, u_coords);
        v_basis[i] = LagrangeBasis1D(v, i, v_coords);
    }

    Point interpolated_point{0, 0, 0};
    int k = 0;

    // 1. 处理4个角点
    if (k < cp.size()) {
        double N = u_basis[0] * v_basis[0];
        for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * cp[k][dim];
        k++;
    }
    if (k < cp.size()) {
        double N = u_basis[order] * v_basis[0];
        for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * cp[k][dim];
        k++;
    }
    if (k < cp.size()) {
        double N = u_basis[order] * v_basis[order];
        for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * cp[k][dim];
        k++;
    }
    if (k < cp.size()) {
        double N = u_basis[0] * v_basis[order];
        for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * cp[k][dim];
        k++;
    }

    // 2. 处理 (order-1)*4 个边节点
    if (order > 1) {
        for (int i = 1; i < order; ++i) {
            if (k >= cp.size()) break;
            double N = u_basis[i] * v_basis[0];
            for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * cp[k][dim];
            k++;
        }
        for (int j = 1; j < order; ++j) {
            if (k >= cp.size()) break;
            double N = u_basis[order] * v_basis[j];
            for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * cp[k][dim];
            k++;
        }
        for (int i = order - 1; i > 0; --i) {
            if (k >= cp.size()) break;
            double N = u_basis[i] * v_basis[order];
            for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * cp[k][dim];
            k++;
        }
        for (int j = order - 1; j > 0; --j) {
            if (k >= cp.size()) break;
            double N = u_basis[0] * v_basis[j];
            for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * cp[k][dim];
            k++;
        }
    }

    // 3. 处理内部节点
    if (order > 1) {
        for (int j = 1; j < order; ++j) {
            for (int i = 1; i < order; ++i) {
                if (k >= cp.size()) break;
                double N = u_basis[i] * v_basis[j];
                for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * cp[k][dim];
                k++;
            }
        }
    }
    return interpolated_point;
}


Point InterpolateLagrangeTriangle(const std::vector<Point>& controlPoints, int order, double r, double s) {
    if (order == 2) {
        if (controlPoints.size() < 6) return {};

        // 形函数
        double N0 = (1.0 - r - s) * (1.0 - 2.0 * r - 2.0 * s);
        double N1 = r * (2.0 * r - 1.0);
        double N2 = s * (2.0 * s - 1.0);
        double N3 = 4.0 * r * (1.0 - r - s);
        double N4 = 4.0 * r * s;
        double N5 = 4.0 * s * (1.0 - r - s);

        const Point& P0 = controlPoints[0];
        const Point& P1 = controlPoints[1];
        const Point& P2 = controlPoints[2];
        const Point& P3 = controlPoints[3];
        const Point& P4 = controlPoints[4];
        const Point& P5 = controlPoints[5];

        // 计算坐标
        Point result;
        for (int i = 0; i < 3; ++i) {
            result[i] = P0[i] * N0 + P1[i] * N1 + P2[i] * N2 + P3[i] * N3 + P4[i] * N4 + P5[i] * N5;
        }
        return result;
    }

    double L1 = 1.0 - r - s;
    double L2 = r;
    double L3 = s;

    Point interpolated_point{0, 0, 0};
    int k = 0;

    // 1. 处理3个角点
    if (k < controlPoints.size()) {
        double N = CardinalPolynomial(L2, order, 0) * CardinalPolynomial(L3, order, 0) *
                   CardinalPolynomial(L1, order, order);
        const Point& p = controlPoints[k];
        for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * p[dim];
        k++;
    }
    if (k < controlPoints.size()) {
        double N = CardinalPolynomial(L2, order, order) * CardinalPolynomial(L3, order, 0) *
                   CardinalPolynomial(L1, order, 0);
        const Point& p = controlPoints[k];
        for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * p[dim];
        k++;
    }
    if (k < controlPoints.size()) {
        double N = CardinalPolynomial(L2, order, 0) * CardinalPolynomial(L3, order, order) *
                   CardinalPolynomial(L1, order, 0);
        const Point& p = controlPoints[k];
        for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * p[dim];
        k++;
    }

    // 2. 处理 (order-1)*3 个边节点
    if (order > 1) {
        // 边 1-2
        for (int i = 1; i < order; ++i) {
            if (k >= controlPoints.size()) break;
            double N = CardinalPolynomial(L2, order, i) * CardinalPolynomial(L3, order, 0) *
                       CardinalPolynomial(L1, order, order - i);
            const Point& p = controlPoints[k];
            for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * p[dim];
            k++;
        }
        // 边 2-3
        for (int i = 1; i < order; ++i) {
            if (k >= controlPoints.size()) break;
            double N = CardinalPolynomial(L2, order, order - i) * CardinalPolynomial(L3, order, i) *
                       CardinalPolynomial(L1, order, 0);
            const Point& p = controlPoints[k];
            for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * p[dim];
            k++;
        }
        // 边 3-1
        for (int i = 1; i < order; ++i) {
            if (k >= controlPoints.size()) break;
            double N = CardinalPolynomial(L2, order, 0) * CardinalPolynomial(L3, order, order - i) *
                       CardinalPolynomial(L1, order, i);
            const Point& p = controlPoints[k];
            for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * p[dim];
            k++;
        }
    }

    // 3. 处理内部节点
    if (order > 2) {
        for (int j = 1; j < order - 1; ++j) {
            for (int i = 1; i < order - j; ++i) {
                if (k >= controlPoints.size()) break;
                int m = order - i - j;
                double N = CardinalPolynomial(L2, order, i) * CardinalPolynomial(L3, order, j) *
                           CardinalPolynomial(L1, order, m);
                const Point& p = controlPoints[k];
                for (int dim = 0; dim < 3; ++dim) interpolated_point[dim] += N * p[dim];
                k++;
            }
        }
    }

    return interpolated_point;
}

//细分函数
void TessellateLagrangeFace(const std::vector<Point>& controlPoints, IGenum cell_type, int order, int divisions,
                            std::vector<Point>& outPoints, std::vector<igIndex>& outTriangleIndices,
                            std::vector<unsigned char>& outTriangleEdgeMasks) {
    if (order < 2 || divisions < 1) return;

    outPoints.clear();
    outTriangleIndices.clear();
    outTriangleEdgeMasks.clear();

    if (cell_type == IG_LAGRANGE_QUADRILATERAL || cell_type == IG_QUADRATIC_QUAD) {
        size_t num_cp = controlPoints.size();
        if (order == 2) {
            if (num_cp < 8) return;
        } else {
            size_t expected_points = (size_t) (order + 1) * (order + 1);
            if (num_cp < expected_points) return;
        }

        int numVertsAlongEdge = divisions + 1;
        std::vector<std::vector<igIndex>> localGridIndices(numVertsAlongEdge, std::vector<igIndex>(numVertsAlongEdge));

        for (int j = 0; j < numVertsAlongEdge; ++j) {
            for (int i = 0; i < numVertsAlongEdge; ++i) {
                double u = -1.0 + 2.0 * static_cast<double>(i) / divisions;
                double v = -1.0 + 2.0 * static_cast<double>(j) / divisions;
                Point p = InterpolateLagrangeQuad(controlPoints, order, u, v);
                outPoints.push_back(p);
                localGridIndices[j][i] = outPoints.size() - 1;
            }
        }
        // 遍历每个小四边形网格，并为生成的两个小三角形计算精确的边掩码
        for (int j = 0; j < divisions; ++j) {
            for (int i = 0; i < divisions; ++i) {
                igIndex p00 = localGridIndices[j][i], p10 = localGridIndices[j][i + 1];
                igIndex p01 = localGridIndices[j + 1][i], p11 = localGridIndices[j + 1][i + 1];

                // 第一个三角形 T1 = (p00, p10, p11)
                // 边定义: 0=(p00-p10), 1=(p10-p11), 2=(p11-p00)
                unsigned char mask1 = 0;
                if (j == 0) mask1 |= 1;             // 如果在底部，显示边 0
                if (i == divisions - 1) mask1 |= 2; // 如果在右侧，显示边 1
                outTriangleIndices.insert(outTriangleIndices.end(), {p00, p10, p11});
                outTriangleEdgeMasks.push_back(mask1);

                // 第二个三角形 T2 = (p00, p11, p01)
                // 边定义: 0=(p00-p11), 1=(p11-p01), 2=(p01-p00)
                unsigned char mask2 = 0;
                if (j == divisions - 1) mask2 |= 2; // 如果在顶部，显示边 1
                if (i == 0) mask2 |= 4;             // 如果在左侧，显示边 2
                outTriangleIndices.insert(outTriangleIndices.end(), {p00, p11, p01});
                outTriangleEdgeMasks.push_back(mask2);
            }
        }
    } else if (cell_type == IG_LAGRANGE_TRIANGLE || cell_type == IG_QUADRATIC_TRIANGLE) {
        size_t expected_points = (size_t) (order + 1) * (order + 2) / 2;
        if (controlPoints.size() < expected_points) return;

        int numVertsAlongEdge = divisions + 1;
        std::vector<std::vector<igIndex>> localGridIndices(numVertsAlongEdge,
                                                           std::vector<igIndex>(numVertsAlongEdge, -1));

        for (int j = 0; j < numVertsAlongEdge; ++j) {
            for (int i = 0; i < numVertsAlongEdge - j; ++i) {
                double r = static_cast<double>(i) / divisions;
                double s = static_cast<double>(j) / divisions;
                Point p = InterpolateLagrangeTriangle(controlPoints, order, r, s);
                outPoints.push_back(p);
                localGridIndices[j][i] = outPoints.size() - 1;
            }
        }

        for (int j = 0; j < divisions; ++j) {
            for (int i = 0; i < divisions - j; ++i) {
                igIndex p00 = localGridIndices[j][i], p10 = localGridIndices[j][i + 1],
                        p01 = localGridIndices[j + 1][i];

                // “朝下”的小三角形 T_down = (p00, p10, p01)
                // 边定义: 0=(p00-p10), 1=(p10-p01), 2=(p01-p00)
                unsigned char mask_down = 0;
                if (j == 0) mask_down |= 1;                 // 如果在底部 (s=0)，显示边 0
                if (i + j == divisions - 1) mask_down |= 2; // 如果在斜边 (r+s=1)，显示边 1
                if (i == 0) mask_down |= 4;                 // 如果在左侧 (r=0)，显示边 2
                outTriangleIndices.insert(outTriangleIndices.end(), {p00, p10, p01});
                outTriangleEdgeMasks.push_back(mask_down);

                // 如果存在，处理“朝上”的小三角形 T_up = (p10, p11, p01)
                if (i < divisions - j - 1) {
                    igIndex p11 = localGridIndices[j + 1][i + 1];
                    // T_up 的所有三条边都在内部，所以掩码永远是0
                    outTriangleIndices.insert(outTriangleIndices.end(), {p10, p11, p01});
                    outTriangleEdgeMasks.push_back(0);
                }
            }
        }
    }
}

// --- 单元工厂函数 ---
Cell* GetTypedCell(IGenum type) {

    // 拉格朗日单元
    static LagrangeLine::Pointer s_LagrangeLine = LagrangeLine::New();
    static LagrangeTriangle::Pointer s_LagrangeTriangle = LagrangeTriangle::New();
    static LagrangeQuadrilateral::Pointer s_LagrangeQuad = LagrangeQuadrilateral::New();
    static LagrangeTetrahedron::Pointer s_LagrangeTetra = LagrangeTetrahedron::New();
    static LagrangeHexahedron::Pointer s_LagrangeHex = LagrangeHexahedron::New();
    static LagrangePyramid::Pointer s_LagrangePyramid = LagrangePyramid::New();
    static LagrangePrism::Pointer s_LagrangePrism = LagrangePrism::New();

    switch (type) {
        case IG_QUADRATIC_EDGE:
            return s_LagrangeLine.get();
        case IG_LAGRANGE_TRIANGLE:
            return s_LagrangeTriangle.get();
        case IG_LAGRANGE_QUADRILATERAL:
            return s_LagrangeQuad.get();
        case IG_LAGRANGE_TETRAHEDRON:
            return s_LagrangeTetra.get();
        case IG_LAGRANGE_HEXAHEDRON:
            return s_LagrangeHex.get();
        case IG_LAGRANGE_PYRAMID:
            return s_LagrangePyramid.get();
        case IG_LAGRANGE_PRISM:
            return s_LagrangePrism.get();
        default:
            return nullptr;
    }
}

} // end anonymous namespace


// --- 类成员函数实现 ---

LagrangeUnstructuredMesh::LagrangeUnstructuredMesh() {
    m_ViewStyle = IG_SURFACE;
    m_Cells = CellArray::New();
    m_CellShapes = UnsignedIntArray::New();
    m_CellOrders = IntArray::New();
}

void LagrangeUnstructuredMesh::AddCell(igIndex* cell_nodes, int num_nodes, IGenum specific_cell_type, int order) {
    m_Cells->AddCellIds(cell_nodes, num_nodes);
    m_CellShapes->AddValue(specific_cell_type);
    m_CellOrders->AddValue(order);
    this->Modified();
}

IGsize LagrangeUnstructuredMesh::GetNumberOfCells() const noexcept { return m_Cells->GetNumberOfCells(); }
int LagrangeUnstructuredMesh::GetCellPointIds(const IGsize cellId, const igIndex*& ids) {
    return m_Cells->GetCellIds(cellId, ids);
}
void LagrangeUnstructuredMesh::GetCellPointIds(const IGsize cellId, IdArray::Pointer ids) {
    if (ids) { m_Cells->GetCellIds(cellId, ids); }
}
IGenum LagrangeUnstructuredMesh::GetSpecificCellType(const IGsize cellId) const {
    return m_CellShapes->GetValue(cellId);
}
int LagrangeUnstructuredMesh::GetCellOrder(const IGsize cellId) const { return m_CellOrders->GetValue(cellId); }

Cell* LagrangeUnstructuredMesh::GetCell(const IGsize cellId) {
    IGenum cell_type = GetSpecificCellType(cellId);
    int order = GetCellOrder(cellId);

    Cell* cell = GetTypedCell(cell_type);
    if (cell == nullptr) { return nullptr; }

    if (auto* l_line = dynamic_cast<LagrangeLine*>(cell)) {
        l_line->SetOrder(order);
    } else if (auto* l_face = dynamic_cast<LagrangeFace*>(cell)) {
        l_face->SetOrder(order);
    } else if (auto* l_volume = dynamic_cast<LagrangeVolume*>(cell)) {
        l_volume->SetOrder(order);
    }

    cell->Reset();
    const igIndex* ids = nullptr;
    int num_nodes = GetCellPointIds(cellId, ids);
    cell->m_PointIds->SetNumberOfIds(num_nodes);
    cell->m_Points->SetNumberOfPoints(num_nodes);
    for (int i = 0; i < num_nodes; i++) {
        cell->m_PointIds->SetId(i, ids[i]);
        // GetPoint(ids[i]) 从 m_Points 中获取点
        cell->m_Points->SetPoint(i, GetPoint(ids[i]));
    }

    return cell;
}

void LagrangeUnstructuredMesh::ConvertToDrawableData() {
    if (m_Points->GetMTime() > m_Positions->GetMTime() || m_ReConvertToDrawableData) {
        m_ReConvertToDrawableData = false;
        std::vector<std::vector<int>> rawLineIndices;
        auto finalPoints = Points::New();
        // 用于收集高阶单元细分后结果的临时缓冲区
        auto tessellatedPoints = Points::New();
        auto highOrderTriangleIndices = UnsignedIntArray::New();

        auto pointIndices = UnsignedIntArray::New();
        auto lineIndices = UnsignedIntArray::New();
        auto triangleIndices = UnsignedIntArray::New();
        auto triangleEdgeMasks = UnsignedCharArray::New();
        m_TriangleQualities = FloatArray::New();
        std::map<igIndex, igIndex> pointIndexMap;
        const int tessellation_divisions = 8;
        const igIndex numOriginalPoints = m_Points->GetNumberOfPoints();
        if (numOriginalPoints > 0) {
            // 将所有原始顶点完整地复制到最终顶点缓冲区
            finalPoints->SetNumberOfPoints(numOriginalPoints);
            memcpy(finalPoints->RawPointer(), m_Points->RawPointer(), numOriginalPoints * sizeof(Point));
            // 为所有原始顶点创建索引，以便将它们作为点云渲染
            pointIndices->SetDimension(1);
            pointIndices->Resize(numOriginalPoints);
            for (igIndex i = 0; i < numOriginalPoints; ++i) { pointIndices->SetValue(i, i); }
        }
        //从高阶和线性单元中收集所有三角形索引
        for (int id = 0; id < GetNumberOfCells(); id++) {
            int order = GetCellOrder(id);
            if (order < 2) std::cerr << "error cell type";

            const igIndex* cell_node_ids = nullptr;
            GetCellPointIds(id, cell_node_ids);
            Cell* cell = GetCell(id);
            if (!cell) continue;

            // 生成线框
            const int num_edges = cell->GetNumberOfEdges();
            for (int i = 0; i < num_edges; ++i) {
                const igIndex* edge_pt_ids = nullptr;
                int num_edge_pts = 0;
                if (auto* l_face = dynamic_cast<LagrangeFace*>(cell)) {
                    num_edge_pts = l_face->GetEdgePointIds(i, edge_pt_ids);
                } else if (auto* l_volume = dynamic_cast<LagrangeVolume*>(cell)) {
                    num_edge_pts = l_volume->GetEdgePointIds(i, edge_pt_ids);
                }
                if (num_edge_pts >= 2) {
                    std::vector<igIndex> temp;
                    temp.push_back(cell_node_ids[edge_pt_ids[0]]);
                    temp.push_back(cell_node_ids[edge_pt_ids[1]]);
                    rawLineIndices.push_back(temp);
                }
            }
            // 对高阶单元的面进行曲面细分
            auto process_face = [&](Cell* face_cell) {
                if (!face_cell) return;
                std::vector<Point> controlPoints;
                for (int j = 0; j < face_cell->GetNumberOfPoints(); ++j)
                    controlPoints.push_back(face_cell->GetPoint(j));
                std::vector<Point> localPoints;
                std::vector<igIndex> localTriangleIndices;
                std::vector<unsigned char> localTriangleEdgeMasks;
                TessellateLagrangeFace(controlPoints, face_cell->GetCellType(), order, tessellation_divisions,
                                       localPoints, localTriangleIndices, localTriangleEdgeMasks);
                igIndex pointOffset = finalPoints->GetNumberOfPoints();
                // 将新生成的局部顶点追加到全局顶点列表
                for (const auto& p: localPoints) { finalPoints->AddPoint(p); }
                // 将带偏移量的局部索引追加到全局索引列表
                for (const auto& idx: localTriangleIndices) { triangleIndices->AddValue(idx + pointOffset); }
                // 追加边掩码
                for (const auto& mask: localTriangleEdgeMasks) { triangleEdgeMasks->AddValue(mask); }
            };

            if (auto* l_volume = dynamic_cast<LagrangeVolume*>(cell)) {
                for (int i = 0; i < l_volume->GetNumberOfFaces(); i++) { process_face(l_volume->GetFace(i)); }
            } else if (auto* l_face = dynamic_cast<LagrangeFace*>(cell)) {
                process_face(l_face);
            }
        }

        // 构建线框的顶点和重映射索引
        lineIndices->SetDimension(2);
        const igIndex numLines = rawLineIndices.size();
        for (igIndex i = 0; i < numLines; ++i) {

            // 将取出的值存入 igIndex 类型的数组中
            igIndex old_indices[2];
            old_indices[0] = rawLineIndices[i][0];
            old_indices[1] = rawLineIndices[i][1];

            igIndex new_indices[2];
            for (int j = 0; j < 2; ++j) {
                igIndex oldIndex = old_indices[j];
                if (pointIndexMap.find(oldIndex) == pointIndexMap.end()) {
                    finalPoints->AddPoint(m_Points->GetPoint(oldIndex));
                    igIndex newIndex = finalPoints->GetNumberOfPoints() - 1;
                    pointIndexMap[oldIndex] = newIndex;
                    new_indices[j] = newIndex;
                } else {
                    new_indices[j] = pointIndexMap[oldIndex];
                }
            }
            lineIndices->AddElement2(new_indices[0], new_indices[1]);
        }

        m_Positions = finalPoints->ConvertToArray();
        m_Positions->Modified();
        m_PointIndices = pointIndices;
        m_PointIndices->Modified();
        m_LineIndices = lineIndices;
        m_LineIndices->Modified();
        m_TriangleIndices = triangleIndices;
        m_TriangleIndices->Modified();
        m_TriangleEdgeMasks = triangleEdgeMasks;
        m_TriangleEdgeMasks->Modified();
        m_TriangleQualities->Modified();
    }


    if (m_AttributeIndex == -1) {
        m_UseColor = false;
        m_ColorWithCell = false;
    } else {
        m_UseColor = true;

        auto& attr = this->GetAttributeSet()->GetAttribute(m_AttributeIndex);
        if (attr.type == IG_RGB) {
            this->m_ColorMapper->SetVectorModeToRGBColors();
        } else {
            this->m_ColorMapper->SetVectorModeToComponent();
        }
        if (!attr.isDeleted) {
            if (attr.attachmentType == IG_POINT) {
                if (m_AttributeHelper->GetMTime() > m_Colors->GetMTime() ||
                    m_ColorMapper->GetMTime() > m_Colors->GetMTime()) {
                    m_ColorWithCell = false;
                    this->SetAttributeWithPointData(attr.pointer, attr.GetDataRange(), m_AttributeDimension);
                }
            } else if (attr.attachmentType == IG_CELL) {
                if (m_AttributeHelper->GetMTime() > m_CellColors->GetMTime() ||
                    m_ColorMapper->GetMTime() > m_CellColors->GetMTime()) {
                    m_ColorWithCell = true;
                    this->SetAttributeWithCellData(attr.pointer, attr.GetDataRange(), m_AttributeDimension);
                }
            }
        }
    }
}

void LagrangeUnstructuredMesh::SetAttributeWithCellData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange,
                                                        igIndex dimension) {
    if (m_ColorMapper->GetMTime() <= this->GetMTime()) {
        double magnitude_min = attrRange->GetValue(0);
        double magnitude_max = attrRange->GetValue(1);
        if (magnitude_min < magnitude_max) {
            m_ColorMapper->SetRange(magnitude_min, magnitude_max);
        } else if (dimension == -1) {
            m_ColorMapper->InitRange(attr);
        } else {
            m_ColorMapper->InitRange(attr, dimension);
        }
    }

    attrRange->SetValue(0, m_ColorMapper->GetRange()[0]);
    attrRange->SetValue(1, m_ColorMapper->GetRange()[1]);

    FloatArray::Pointer qualityScalars = FloatArray::New();
    qualityScalars->Resize(m_TriangleQualities->GetNumberOfElements());
    for (int i = 0; i < m_TriangleQualities->GetNumberOfElements(); ++i) {
        qualityScalars->SetValue(i, m_TriangleQualities->GetValue(i));
    }
    FloatArray::Pointer triangleRGBColors = m_ColorMapper->MapScalars(qualityScalars, dimension);
    if (triangleRGBColors == nullptr) { return; }

    FloatArray::Pointer newPositions = FloatArray::New();
    FloatArray::Pointer newColors = FloatArray::New();
    UnsignedCharArray::Pointer newEdgeMasks = UnsignedCharArray::New();

    newPositions->SetDimension(3);
    newColors->SetDimension(3);
    newEdgeMasks->SetDimension(3);

    IGsize numTriangles = m_TriangleIndices->GetNumberOfElements() / 3;
    newPositions->Reserve(numTriangles * 3);
    newColors->Reserve(numTriangles * 3);
    newEdgeMasks->Reserve(numTriangles);

    float color[3]{};
    igIndex vertexIds[3]{};

    // 遍历所有细分后的三角形
    for (IGsize i = 0; i < numTriangles; i++) {
        triangleRGBColors->GetElement(i, color);

        vertexIds[0] = m_TriangleIndices->GetValue(i * 3 + 0);
        vertexIds[1] = m_TriangleIndices->GetValue(i * 3 + 1);
        vertexIds[2] = m_TriangleIndices->GetValue(i * 3 + 2);

        for (int j = 0; j < 3; j++) {
            float p[3];
            m_Positions->GetElement(vertexIds[j], p);
            newPositions->AddElement3(p[0], p[1], p[2]);
            newColors->AddElement3(color[0], color[1], color[2]);
            // TODO: 这里的边掩码需要根据具体需求进行调整
            // newEdgeMasks.AddValue();
        }
    }
    m_CellPositionSize = newPositions->GetNumberOfElements();
    m_CellPositions = newPositions;
    m_CellPositions->Modified();

    m_CellColors = newColors;
    m_CellColors->Modified();

    m_CellTriangleEdgeMasks = newEdgeMasks;
    m_CellTriangleEdgeMasks->Modified();
}

IGAME_NAMESPACE_END
