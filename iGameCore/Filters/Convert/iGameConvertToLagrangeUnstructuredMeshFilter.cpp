#include "iGameConvertToLagrangeUnstructuredMeshFilter.h"

IGAME_NAMESPACE_BEGIN
ConvertToLagrangeUnstructuredMeshFilter::ConvertToLagrangeUnstructuredMeshFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

ConvertToLagrangeUnstructuredMeshFilter::~ConvertToLagrangeUnstructuredMeshFilter() {}

bool ConvertToLagrangeUnstructuredMeshFilter::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) { return false; }

    auto input = m_Inputs->GetElement(0);
    if (!input) { return false; }

    switch (input->GetDataObjectType()) {
        case IG_NONE:
            return true;
        case IG_UNSTRUCTURED_MESH:
            return this->ExecuteWithUnstructuredMesh(DynamicCast<UnstructuredMesh>(input));
        default:
            return false;
    }
    return true;
}

bool ConvertToLagrangeUnstructuredMeshFilter::ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um) {
    if (um == nullptr) return false;

    LagrangeUnstructuredMesh::Pointer outputMesh = LagrangeUnstructuredMesh::New();
    for (int i = 0; i < um->GetNumberOfPoints(); ++i) { outputMesh->AddPoint(um->GetPoints()->GetPoint(i)); }

    int cellNum = um->GetNumberOfCells();
    igIndex vhs[IGAME_CELL_MAX_SIZE];
    for (int i = 0; i < cellNum; i++) {
        int cellSize = um->GetCell(i)->GetNumberOfPoints();
        for (int j = 0; j < cellSize; j++) { vhs[j] = um->GetCell(i)->GetPointId(j); }
        IGenum cellType = um->GetCellType(i);
        int order = 0;

        switch (cellType) {
            case IG_LAGRANGE_TRIANGLE:
                order = GetOrderFromNodes_Tri(cellSize);
                break;
            case IG_LAGRANGE_QUADRILATERAL:
                order = GetOrderFromNodes_Quad(cellSize);
                break;
            case IG_LAGRANGE_TETRAHEDRON:
                order = GetOrderFromNodes_Tetra(cellSize);
                break;
            case IG_LAGRANGE_HEXAHEDRON:
                order = GetOrderFromNodes_Hex(cellSize);
                break;
            case IG_LAGRANGE_PYRAMID:
                order = GetOrderFromNodes_Pyramid(cellSize);
                break;
            case IG_LAGRANGE_PRISM:
                order = GetOrderFromNodes_Prism(cellSize);
                break;
            case IG_EMPTY_CELL:
                order = 0;
                break;
            default:
                break;
        }
        if (order > 0) { outputMesh->AddCell(vhs, cellSize, cellType, order); }
    }
    outputMesh->SetAttributeSet(um->GetAttributeSet());
    this->SetOutput(outputMesh);
}

int ConvertToLagrangeUnstructuredMeshFilter::GetOrderFromNodes_Quad(int num_nodes) {
    double p = std::sqrt(static_cast<double>(num_nodes)) - 1.0;
    return static_cast<int>(std::round(p));
}

int ConvertToLagrangeUnstructuredMeshFilter::GetOrderFromNodes_Hex(int num_nodes) {
    double p = std::cbrt(static_cast<double>(num_nodes)) - 1.0;
    return static_cast<int>(std::round(p));
}

int ConvertToLagrangeUnstructuredMeshFilter::GetOrderFromNodes_Prism(int num_nodes) {
    // 使用迭代法来反算阶数 p
    // 公式: N = (p+1)^2 * (p+2) / 2
    // 这是p阶2D三角形节点数 ((p+1)(p+2)/2) 与 p阶1D线段节点数 (p+1) 的乘积。
    // 我们从 p=1 开始循环，直到计算出的 N 与输入的 num_nodes 匹配。
    for (int p = 1; p < 10; ++p) { // 循环到9阶，对于绝大多数情况都已足够
        int calculated_nodes = (p + 1) * (p + 1) * (p + 2) / 2;
        if (calculated_nodes == num_nodes) {
            return p; // 找到匹配的阶数，返回 p
        }
    }

    // 如果循环结束仍未找到，说明节点数不匹配任何整数阶的棱柱
    return 0; // 返回0表示错误或无法确定
}

int ConvertToLagrangeUnstructuredMeshFilter::GetOrderFromNodes_Pyramid(int num_nodes) {
    // 使用迭代法来反算阶数 p
    // 公式: N = (p+1)(p+2)(2p+3)/6
    // 我们从 p=1 开始循环，直到计算出的 N 与输入的 num_nodes 匹配
    for (int p = 1; p < 10; ++p) { // 循环到9阶，对于绝大多数情况都已足够
        int calculated_nodes = (p + 1) * (p + 2) * (2 * p + 3) / 6;
        if (calculated_nodes == num_nodes) {
            return p; // 找到匹配的阶数，返回 p
        }
    }

    // 如果循环结束仍未找到，说明节点数不匹配任何整数阶的金字塔
    return 0; // 返回0表示错误或无法确定
}

int ConvertToLagrangeUnstructuredMeshFilter::GetOrderFromNodes_Tri(int num_nodes) {
    // N = (p+1)(p+2)/2  =>  p^2 + 3p + (2-2N) = 0
    double p = (-3.0 + std::sqrt(9.0 - 4.0 * (2.0 - 2.0 * num_nodes))) / 2.0;
    return static_cast<int>(std::round(p));
}

int ConvertToLagrangeUnstructuredMeshFilter::GetOrderFromNodes_Tetra(int num_nodes) {
    // 使用迭代法来反算阶数 p
    // 公式: N = (p+1)(p+2)(p+3)/6
    for (int p = 1; p < 10; ++p) { // 循环到9阶通常已足够
        int calculated_nodes = (p + 1) * (p + 2) * (p + 3) / 6;
        if (calculated_nodes == num_nodes) {
            return p; // 找到匹配的阶数，返回 p
        }
    }

    // 如果循环结束仍未找到，说明节点数不匹配任何整数阶的四面体
    return 0; // 返回0表示错误或无法确定
}

void ConvertToLagrangeUnstructuredMeshFilter::SetConvertMethod(ConvertMethod CM) { this->m_ConvertMethod = CM; }

IGAME_NAMESPACE_END
