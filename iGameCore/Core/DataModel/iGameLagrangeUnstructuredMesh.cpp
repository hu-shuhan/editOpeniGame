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


std::vector<float> InterpolateLagrangeQuad(FloatArray::Pointer originData, int order, double u, double v) {
    // 检查控制点数量是否与纯拉格朗日单元的公式匹配
    size_t expected_points = (size_t) (order + 1) * (order + 1);
    if (originData->GetNumberOfElements() < expected_points) return {};

    auto u_coords = GetLagrangeNodeCoordinates1D(order);
    auto v_coords = GetLagrangeNodeCoordinates1D(order);
    std::vector<double> u_basis(order + 1), v_basis(order + 1);
    for (int i = 0; i <= order; ++i) {
        u_basis[i] = LagrangeBasis1D(u, i, u_coords);
        v_basis[i] = LagrangeBasis1D(v, i, v_coords);
    }

    std::vector<float> interpolated_data;
    interpolated_data.assign(originData->GetDimension(), 0);
    int k = 0;

    // 1. 处理4个角点
    if (k < originData->GetNumberOfElements()) {
        double N = u_basis[0] * v_basis[0];
        for (int dim = 0; dim < interpolated_data.size(); ++dim)
            interpolated_data[dim] += N * originData->GetElementValue(k, dim);
        k++;
    }
    if (k < originData->GetNumberOfElements()) {
        double N = u_basis[order] * v_basis[0];
        for (int dim = 0; dim < interpolated_data.size(); ++dim)
            interpolated_data[dim] += N * originData->GetElementValue(k, dim);
        k++;
    }
    if (k < originData->GetNumberOfElements()) {
        double N = u_basis[order] * v_basis[order];
        for (int dim = 0; dim < interpolated_data.size(); ++dim)
            interpolated_data[dim] += N * originData->GetElementValue(k, dim);
        k++;
    }
    if (k < originData->GetNumberOfElements()) {
        double N = u_basis[0] * v_basis[order];
        for (int dim = 0; dim < interpolated_data.size(); ++dim)
            interpolated_data[dim] += N * originData->GetElementValue(k, dim);
        k++;
    }

    // 2. 处理 (order-1)*4 个边节点
    if (order > 1) {
        for (int i = 1; i < order; ++i) {
            if (k >= originData->GetNumberOfElements()) break;
            double N = u_basis[i] * v_basis[0];
            for (int dim = 0; dim < interpolated_data.size(); ++dim)
                interpolated_data[dim] += N * originData->GetElementValue(k, dim);
            k++;
        }
        for (int j = 1; j < order; ++j) {
            if (k >= originData->GetNumberOfElements()) break;
            double N = u_basis[order] * v_basis[j];
            for (int dim = 0; dim < interpolated_data.size(); ++dim)
                interpolated_data[dim] += N * originData->GetElementValue(k, dim);
            k++;
        }
        for (int i = order - 1; i > 0; --i) {
            if (k >= originData->GetNumberOfElements()) break;
            double N = u_basis[i] * v_basis[order];
            for (int dim = 0; dim < interpolated_data.size(); ++dim)
                interpolated_data[dim] += N * originData->GetElementValue(k, dim);
            k++;
        }
        for (int j = order - 1; j > 0; --j) {
            if (k >= originData->GetNumberOfElements()) break;
            double N = u_basis[0] * v_basis[j];
            for (int dim = 0; dim < interpolated_data.size(); ++dim)
                interpolated_data[dim] += N * originData->GetElementValue(k, dim);
            k++;
        }
    }

    // 3. 处理内部节点
    if (order > 1) {
        for (int j = 1; j < order; ++j) {
            for (int i = 1; i < order; ++i) {
                if (k >= originData->GetNumberOfElements()) break;
                double N = u_basis[i] * v_basis[j];
                for (int dim = 0; dim < interpolated_data.size(); ++dim)
                    interpolated_data[dim] += N * originData->GetElementValue(k, dim);
                k++;
            }
        }
    }
    return interpolated_data;
}

std::vector<float> InterpolateLagrangeTriangle(FloatArray::Pointer originData, int order, double r, double s) {
    if (order == 2) {
        if (originData->GetNumberOfElements() < 6) return {};

        // 形函数
        double N0 = (1.0 - r - s) * (1.0 - 2.0 * r - 2.0 * s);
        double N1 = r * (2.0 * r - 1.0);
        double N2 = s * (2.0 * s - 1.0);
        double N3 = 4.0 * r * (1.0 - r - s);
        double N4 = 4.0 * r * s;
        double N5 = 4.0 * s * (1.0 - r - s);

        //这里GetElement返回的是常引用(所有Element共用一份)，所以不能用引用来接
        std::vector<float> P0 = originData->GetElement(0);
        std::vector<float> P1 = originData->GetElement(1);
        std::vector<float> P2 = originData->GetElement(2);
        std::vector<float> P3 = originData->GetElement(3);
        std::vector<float> P4 = originData->GetElement(4);
        std::vector<float> P5 = originData->GetElement(5);

        // 计算坐标
        std::vector<float> result(originData->GetDimension());
        for (int i = 0; i < result.size(); ++i) {
            result[i] = P0[i] * N0 + P1[i] * N1 + P2[i] * N2 + P3[i] * N3 + P4[i] * N4 + P5[i] * N5;
        }
        return result;
    }

    double L1 = 1.0 - r - s;
    double L2 = r;
    double L3 = s;

    std::vector<float> interpolated_data;
    interpolated_data.assign(originData->GetDimension(), 0);
    int k = 0;

    // 1. 处理3个角点
    if (k < originData->GetNumberOfElements()) {
        double N = CardinalPolynomial(L2, order, 0) * CardinalPolynomial(L3, order, 0) *
                   CardinalPolynomial(L1, order, order);
        const auto& data = originData->GetElement(k);
        for (int dim = 0; dim < interpolated_data.size(); ++dim) interpolated_data[dim] += N * data[dim];
        k++;
    }
    if (k < originData->GetNumberOfElements()) {
        double N = CardinalPolynomial(L2, order, order) * CardinalPolynomial(L3, order, 0) *
                   CardinalPolynomial(L1, order, 0);
        const auto& data = originData->GetElement(k);
        for (int dim = 0; dim < interpolated_data.size(); ++dim) interpolated_data[dim] += N * data[dim];
        k++;
    }
    if (k < originData->GetNumberOfElements()) {
        double N = CardinalPolynomial(L2, order, 0) * CardinalPolynomial(L3, order, order) *
                   CardinalPolynomial(L1, order, 0);
        const auto& data = originData->GetElement(k);
        for (int dim = 0; dim < interpolated_data.size(); ++dim) interpolated_data[dim] += N * data[dim];
        k++;
    }

    // 2. 处理 (order-1)*3 个边节点
    if (order > 1) {
        // 边 1-2
        for (int i = 1; i < order; ++i) {
            if (k >= originData->GetNumberOfElements()) break;
            double N = CardinalPolynomial(L2, order, i) * CardinalPolynomial(L3, order, 0) *
                       CardinalPolynomial(L1, order, order - i);
            const auto& data = originData->GetElement(k);
            for (int dim = 0; dim < interpolated_data.size(); ++dim) interpolated_data[dim] += N * data[dim];
            k++;
        }
        // 边 2-3
        for (int i = 1; i < order; ++i) {
            if (k >= originData->GetNumberOfElements()) break;
            double N = CardinalPolynomial(L2, order, order - i) * CardinalPolynomial(L3, order, i) *
                       CardinalPolynomial(L1, order, 0);
            const auto& data = originData->GetElement(k);
            for (int dim = 0; dim < interpolated_data.size(); ++dim) interpolated_data[dim] += N * data[dim];
            k++;
        }
        // 边 3-1
        for (int i = 1; i < order; ++i) {
            if (k >= originData->GetNumberOfElements()) break;
            double N = CardinalPolynomial(L2, order, 0) * CardinalPolynomial(L3, order, order - i) *
                       CardinalPolynomial(L1, order, i);
            const auto& data = originData->GetElement(k);
            for (int dim = 0; dim < interpolated_data.size(); ++dim) interpolated_data[dim] += N * data[dim];
            k++;
        }
    }

    // 3. 处理内部节点
    if (order > 2) {
        for (int j = 1; j < order - 1; ++j) {
            for (int i = 1; i < order - j; ++i) {
                if (k >= originData->GetNumberOfElements()) break;
                int m = order - i - j;
                double N = CardinalPolynomial(L2, order, i) * CardinalPolynomial(L3, order, j) *
                           CardinalPolynomial(L1, order, m);
                const auto& data = originData->GetElement(k);
                for (int dim = 0; dim < interpolated_data.size(); ++dim) interpolated_data[dim] += N * data[dim];
                k++;
            }
        }
    }

    return interpolated_data;
}

//细分函数
void TessellateLagrangeFace(FloatArray::Pointer originData, IGenum cell_type, int order, int divisions,
                            FloatArray::Pointer outData, std::vector<igIndex>& outTriangleIndices,
                            std::vector<unsigned char>& outTriangleEdgeMasks) {
    if (order < 2 || divisions < 1) return;

    outData->Clear();
    outTriangleIndices.clear();
    outTriangleEdgeMasks.clear();

    outData->SetDimension(originData->GetDimension());
    if (cell_type == IG_LAGRANGE_QUADRILATERAL || cell_type == IG_QUADRATIC_QUAD) {
        size_t num_cp = originData->GetNumberOfElements();
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
                const auto& p = InterpolateLagrangeQuad(originData, order, u, v);
                for (const auto& val: p) outData->AddValue(val);
                localGridIndices[j][i] = outData->GetNumberOfElements() - 1;
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
        if (originData->GetNumberOfElements() < expected_points) return;

        int numVertsAlongEdge = divisions + 1;
        std::vector<std::vector<igIndex>> localGridIndices(numVertsAlongEdge,
                                                           std::vector<igIndex>(numVertsAlongEdge, -1));

        for (int j = 0; j < numVertsAlongEdge; ++j) {
            for (int i = 0; i < numVertsAlongEdge - j; ++i) {
                double r = static_cast<double>(i) / divisions;
                double s = static_cast<double>(j) / divisions;
                const auto& p = InterpolateLagrangeTriangle(originData, order, r, s);
                for (const auto& val: p) outData->AddValue(val);
                localGridIndices[j][i] = outData->GetNumberOfElements() - 1;
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
    bool needReConvertGeometry = m_ReConvertToDrawableData;
    needReConvertGeometry |= m_Points->GetMTime() > m_ReConvertHelper->GetMTime();
    needReConvertGeometry |= m_Clipper->GetMTime() > m_ReConvertHelper->GetMTime();

    bool needReConvertScalar = needReConvertGeometry;
    needReConvertScalar |= m_AttributeHelper->GetMTime() > m_ReConvertHelper->GetMTime();

    // convert geometry data
    if (needReConvertGeometry) {
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
                FloatArray::Pointer controlPoints = FloatArray::New();
                controlPoints->SetDimension(3);
                for (int j = 0; j < face_cell->GetNumberOfPoints(); ++j)
                    controlPoints->AddElement(face_cell->GetPoint(j));

                FloatArray::Pointer localPoints = FloatArray::New();
                std::vector<igIndex> localTriangleIndices;
                std::vector<unsigned char> localTriangleEdgeMasks;
                TessellateLagrangeFace(controlPoints, face_cell->GetCellType(), order, tessellation_divisions,
                                       localPoints, localTriangleIndices, localTriangleEdgeMasks);
                igIndex pointOffset = finalPoints->GetNumberOfPoints();
                // 将新生成的局部顶点追加到全局顶点列表
                for (int i = 0; i < localPoints->GetNumberOfElements(); ++i) {
                    Point tmp;
                    localPoints->GetElement(i, tmp.pointer());
                    finalPoints->AddPoint(tmp);
                }
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

    // convert scalar data
    bool updateColorMapper = m_ColorMapper->GetMTime() > m_ReConvertHelper->GetMTime();
    if (needReConvertScalar || m_AttributeChanged || updateColorMapper) {
        m_AttributeChanged = false;
        m_ColorWithCell = false;
        if (m_AttributeIndex != -1) {
            auto& attr = this->GetAttributeSet()->GetAttribute(m_AttributeIndex);
            if (attr.type == IG_RGB) {
                this->m_ColorMapper->SetVectorModeToRGBColors();
            } else {
                this->m_ColorMapper->SetVectorModeToComponent();
            }

            if (!attr.isDeleted) {
                if (attr.attachmentType == IG_POINT) {
                    m_ColorWithCell = true;
                    this->SetAttributeWithPointData(attr.pointer, attr.GetDataRange(), m_AttributeDimension);
                } else if (attr.attachmentType == IG_CELL) {
                    m_ColorWithCell = true;
                    this->SetAttributeWithCellData(attr.pointer, attr.GetDataRange(), m_AttributeDimension);
                }
            }
        }

        m_ReConvertToDrawableData = false;
        m_ReConvertHelper->Modified();
    }
}

void LagrangeUnstructuredMesh::SetAttributeWithPointData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange,
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

    // DO NOT write ColorMapper range back to attrRange - this corrupts Magnitude range!
    // Previously: attrRange->SetValue(0, m_ColorMapper->GetRange()[0]);
    // Previously: attrRange->SetValue(1, m_ColorMapper->GetRange()[1]);

    if (attr->GetDimension() != 1) return;

    FloatArray::Pointer attributeScalars = FloatArray::New();
    attributeScalars->SetDimension(attr->GetDimension());

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

    for (int i = 0; i < GetNumberOfCells(); ++i) {
        int order = GetCellOrder(i);
        if (order < 2) std::cerr << "error cell type";

        Cell* cell = GetCell(i);
        if (cell == nullptr) continue;

        //GetFace返回的指针是一个复制品，指向一个固定的空间，非Face真实的地址

        auto tesselateEachFace = [&](Cell* f) {
        FloatArray::Pointer originData = FloatArray::New();
        originData->SetDimension(attr->GetDimension());
        for (int pointIndex = 0; pointIndex < f->GetNumberOfPoints(); ++pointIndex) {
            std::vector<float> singleAttribute(attr->GetDimension());
            for (int attributeDim = 0; attributeDim < attr->GetDimension(); ++attributeDim)
                singleAttribute.at(attributeDim) = attr->GetElementValue(f->GetPointId(pointIndex), attributeDim);
            for (const auto& val: singleAttribute) originData->AddValue(val);
        }

        FloatArray::Pointer outData = FloatArray::New();
        std::vector<int> outTriangleIndices;
        std::vector<unsigned char> outTriangleEdgeMasks;
        TessellateLagrangeFace(originData, f->GetCellType(), order, tessellation_divisions, outData,
                                outTriangleIndices, outTriangleEdgeMasks);

        for (const auto& index: outTriangleIndices) {
            std::vector<float> data = outData->GetElement(index);
            for (const auto& val: data) attributeScalars->AddValue(val);
        }
            for (const auto& mask: outTriangleEdgeMasks) { newEdgeMasks->AddValue(mask); }
    };
        if (auto* volume_cell = dynamic_cast<LagrangeVolume*>(cell))
            for (int j = 0; j < volume_cell->GetNumberOfFaces(); ++j) tesselateEachFace(volume_cell->GetFace(j));
        else if (auto* face_cell = dynamic_cast<LagrangeFace*>(cell))
            tesselateEachFace(face_cell);
    }

    FloatArray::Pointer trianglePointRGBColors = m_ColorMapper->MapScalars(attributeScalars, dimension);
    if (trianglePointRGBColors == nullptr) { return; }

    // 遍历所有细分后的三角形
    for (IGsize i = 0; i < numTriangles; i++) {


        vertexIds[0] = m_TriangleIndices->GetValue(i * 3 + 0);
        vertexIds[1] = m_TriangleIndices->GetValue(i * 3 + 1);
        vertexIds[2] = m_TriangleIndices->GetValue(i * 3 + 2);

        for (int j = 0; j < 3; j++) {
            trianglePointRGBColors->GetElement(3 * i + j, color);
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

void LagrangeUnstructuredMesh::SetAttributeWithCellData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange,
                                                        igIndex dimension) {
    double magnitude_min = attrRange->GetValue(0);
    double magnitude_max = attrRange->GetValue(1);
    if (magnitude_min < magnitude_max) {
        m_ColorMapper->SetRange(magnitude_min, magnitude_max);
    } else if (dimension == -1) {
        m_ColorMapper->InitRange(attr);
    } else {
        m_ColorMapper->InitRange(attr, dimension);
    }

    // DO NOT write ColorMapper range back to attrRange - this corrupts Magnitude range!
    // Previously: attrRange->SetValue(0, m_ColorMapper->GetRange()[0]);
    // Previously: attrRange->SetValue(1, m_ColorMapper->GetRange()[1]);

    if (attr->GetDimension() != 1) return;

    FloatArray::Pointer attributeScalars = FloatArray::New();
    attributeScalars->SetDimension(attr->GetDimension());

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

    for (int i = 0; i < GetNumberOfCells(); ++i) {
        int order = GetCellOrder(i);
        if (order < 2) std::cerr << "error cell type";

        Cell* cell = GetCell(i);
        if (cell == nullptr) continue;

        auto tesselateEachFace = [&](Cell* f) {
            FloatArray::Pointer originData = FloatArray::New();
            originData->SetDimension(attr->GetDimension());
            for (int pointIndex = 0; pointIndex < f->GetNumberOfPoints(); ++pointIndex)
                for (int dim = 0; dim < attr->GetDimension(); ++dim) originData->AddValue(0);

            FloatArray::Pointer outData = FloatArray::New();
            std::vector<int> outTriangleIndices;
            std::vector<unsigned char> outTriangleEdgeMasks;
            TessellateLagrangeFace(originData, f->GetCellType(), order, tessellation_divisions, outData,
                                   outTriangleIndices, outTriangleEdgeMasks);
            std::vector<float> attribute;
            attr->GetElement(i, attribute);
            for (int tri = 0; tri < outTriangleIndices.size() / 3; ++tri)
                for (const auto& val: attribute) attributeScalars->AddValue(val);
            for (const auto& mask: outTriangleEdgeMasks) { newEdgeMasks->AddValue(mask); }
        };
        if (auto* volume_cell = dynamic_cast<LagrangeVolume*>(cell))
            for (int j = 0; j < volume_cell->GetNumberOfFaces(); ++j) tesselateEachFace(volume_cell->GetFace(j));
        else if (auto* face_cell = dynamic_cast<LagrangeFace*>(cell))
            tesselateEachFace(face_cell);
    }

    FloatArray::Pointer triangleRGBColors = m_ColorMapper->MapScalars(attributeScalars, dimension);
    if (triangleRGBColors == nullptr) { return; }

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
