#include "iGameIntegrateVariablesFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCell.h"
#include "iGameCellType.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <algorithm>
#include <cmath>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

struct AttributeAccumulator {
    ArrayObject::Pointer input;
    IGenum type{IG_NONE};
    IGenum attachment{IG_NONE};
    IGsize cellOffset{0};
    std::vector<double> values;
}; // 用于累加点属性或单元属性的积分结果

struct GeometryView {
    Points::Pointer points;
    CellArray::Pointer cells;
    UnstructuredMesh::Pointer unstructured;
    int fixedDimension{0};
    IGsize cellAttributeOffsetCandidate{0};
    bool usesSurfaceCellLayout{false};
    bool usesStructuredCellOrdering{false};
    bool hasUnsupportedPolyhedronTopology{false};
}; // 统一保存输入网格的几何数据和维度信息

// vtkDataSetAttributes 中会导致单元不参与积分的 ghost 标记。
constexpr unsigned char kDuplicateCell = 1;
constexpr unsigned char kHiddenCell = 32;
constexpr unsigned char kExcludedGhostCellMask = kDuplicateCell | kHiddenCell;

// 使用 double 精度计算两个空间点之间的距离平方。
double SquaredDistance(const Point& p0, const Point& p1) {
    const double dx = static_cast<double>(p1[0]) - p0[0];
    const double dy = static_cast<double>(p1[1]) - p0[1];
    const double dz = static_cast<double>(p1[2]) - p0[2];
    return dx * dx + dy * dy + dz * dz;
}

// 使用 double 精度计算三个空间点构成的三角形面积。
double TriangleArea(const Point& p0, const Point& p1, const Point& p2) {
    const double e10 = static_cast<double>(p1[0]) - p0[0];
    const double e11 = static_cast<double>(p1[1]) - p0[1];
    const double e12 = static_cast<double>(p1[2]) - p0[2];
    const double e20 = static_cast<double>(p2[0]) - p0[0];
    const double e21 = static_cast<double>(p2[1]) - p0[1];
    const double e22 = static_cast<double>(p2[2]) - p0[2];
    const double cx = e11 * e22 - e12 * e21;
    const double cy = e12 * e20 - e10 * e22;
    const double cz = e10 * e21 - e11 * e20;
    return std::sqrt(cx * cx + cy * cy + cz * cz) * 0.5;
}

// 使用 double 精度计算四个空间点构成的四面体有符号体积。
double SignedTetraVolume(const Point& p0, const Point& p1,
                         const Point& p2, const Point& p3) {
    const double ax = static_cast<double>(p1[0]) - p0[0];
    const double ay = static_cast<double>(p1[1]) - p0[1];
    const double az = static_cast<double>(p1[2]) - p0[2];
    const double bx = static_cast<double>(p2[0]) - p0[0];
    const double by = static_cast<double>(p2[1]) - p0[1];
    const double bz = static_cast<double>(p2[2]) - p0[2];
    const double cx = static_cast<double>(p3[0]) - p0[0];
    const double cy = static_cast<double>(p3[1]) - p0[1];
    const double cz = static_cast<double>(p3[2]) - p0[2];
    return (cx * (ay * bz - az * by) +
            cy * (az * bx - ax * bz) +
            cz * (ax * by - ay * bx)) / 6.0;
}

// 累加折线各线段的长度，并将每段长度的一半分配给两个端点作为积分权重。
double AddLineWeights(const std::vector<Point>& points, std::vector<double>& weights) {
    double measure = 0.0;
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        const double length = std::sqrt(SquaredDistance(points[i], points[i + 1]));
        weights[i] += length * 0.5;
        weights[i + 1] += length * 0.5;
        measure += length;
    }
    return measure;
}

// 累加指定三角形的面积，并将面积均分给三个顶点作为积分权重。
double AddTriangleWeights(const std::vector<Point>& points,
                          size_t i0, size_t i1, size_t i2,
                          std::vector<double>& weights) {
    const double area = TriangleArea(points[i0], points[i1], points[i2]);
    const double share = area / 3.0;
    for (const size_t id : {i0, i1, i2}) weights[id] += share;
    return area;
}

// 累加指定四面体的有符号体积，并将体积均分给四个顶点作为积分权重。
double AddTetraWeights(const std::vector<Point>& points,
                       size_t i0, size_t i1, size_t i2, size_t i3,
                       std::vector<double>& weights) {
    const double volume = SignedTetraVolume(
            points[i0], points[i1], points[i2], points[i3]);
    const double share = volume * 0.25;
    for (const size_t id : {i0, i1, i2, i3}) weights[id] += share;
    return volume;
}

// 按照 ParaView 的线性策略计算单元测度及各顶点的积分权重。
bool ComputeIntegrationWeights(int dimension, const std::vector<Point>& points,
                               bool usesStructuredCellOrdering,
                               std::vector<double>& weights, double& measure) {
    weights.assign(points.size(), 0.0);
    measure = 0.0;
    if (points.empty()) return false;

    const auto addTriangle = [&](size_t i0, size_t i1, size_t i2) {
        measure += AddTriangleWeights(points, i0, i1, i2, weights);
    };
    const auto addTetra = [&](size_t i0, size_t i1, size_t i2, size_t i3) {
        measure += AddTetraWeights(points, i0, i1, i2, i3, weights);
    };

    if (dimension == 1 && points.size() >= 2) {
        measure = AddLineWeights(points, weights);
    } else if (dimension == 2 && points.size() >= 3) {
        for (size_t i = 1; i + 1 < points.size(); ++i)
            addTriangle(0, i, i + 1);
    } else if (dimension == 3) {
        switch (points.size()) {
            case 4:
                addTetra(0, 1, 2, 3);
                break;
            case 5: { // 对齐 vtkPyramid::TriangulateLocalIds(1)，沿底面 1--3 拆分
                addTetra(0, 1, 3, 4);
                addTetra(1, 2, 3, 4);
                break;
            }
            case 6: // 三棱柱：底面为 0、1、2，顶面为 3、4、5
                // 对齐 vtkWedge::TriangulateLocalIds(1) 的有向四面体点序。
                addTetra(0, 2, 1, 3);
                addTetra(1, 5, 4, 3);
                addTetra(1, 5, 3, 2);
                break;
            case 8: // 六面体
                if (usesStructuredCellOrdering) {
                    // StructuredMesh 的局部点序为
                    // 0,1,5,4,3,2,6,7（按 VTK 六面体点序表示）。
                    // 将同一五四面体拆分映射到该点序，避免单位立方体体积变成 -1。
                    addTetra(0, 1, 4, 3);
                    addTetra(1, 3, 2, 6);
                    addTetra(1, 3, 6, 4);
                    addTetra(1, 4, 6, 5);
                    addTetra(4, 6, 7, 3);
                } else {
                    addTetra(0, 1, 3, 4);
                    addTetra(1, 4, 5, 6);
                    addTetra(1, 4, 6, 3);
                    addTetra(1, 3, 6, 2);
                    addTetra(3, 6, 7, 4);
                }
                break;
            default:
                return false;
        }
    } else {
        return false;
    }

    if (!std::isfinite(measure)) return false;
    return std::all_of(weights.begin(), weights.end(),
                       [](double value) { return std::isfinite(value); });
}

// 从受支持的数据对象中取得点、单元连接关系和维度信息。
bool ResolveGeometry(const DataObject::Pointer& input, GeometryView& geometry) {
    geometry = {};
    geometry.points = input->GetPoints();
    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto mesh = DynamicCast<SurfaceMesh>(input);
            if (!mesh) return false;
            geometry.cells = mesh->GetCellArray();
            if (geometry.cells && geometry.cells->GetNumberOfCells() > 0) {
                geometry.fixedDimension = 2;
                geometry.usesSurfaceCellLayout = true;
                // 旧版 VTK POLYDATA 的 CELL_DATA 顺序为 LINES 后接 POLYGONS，
                // 而 SurfaceMesh 将二者分别保存在边和面数组中。
                geometry.cellAttributeOffsetCandidate = mesh->GetNumberOfEdges();
            } else {
                // 旧版 VTK POLYDATA LINES 在 iGame 中保存为 SurfaceMesh 的边。
                geometry.cells = mesh->GetEdges();
                geometry.fixedDimension = 1;
            }
            break;
        }
        case IG_VOLUME_MESH: {
            auto mesh = DynamicCast<VolumeMesh>(input);
            if (!mesh) return false;
            geometry.cells = mesh->GetCellArray();
            geometry.fixedDimension = 3;
            geometry.hasUnsupportedPolyhedronTopology = mesh->GetIsPolyhedronType();
            break;
        }
        case IG_UNSTRUCTURED_MESH:
            geometry.unstructured = DynamicCast<UnstructuredMesh>(input);
            if (!geometry.unstructured) return false;
            geometry.cells = geometry.unstructured->GetCells();
            break;
        case IG_STRUCTURED_MESH: {
            auto mesh = DynamicCast<StructuredMesh>(input);
            if (!mesh) return false;
            const igIndex* dims = mesh->GetDimensionSize();
            if (dims[0] < 1 || dims[1] < 1 || dims[2] < 1) return false;
            const IGsize expectedPoints = static_cast<IGsize>(dims[0]) *
                                          static_cast<IGsize>(dims[1]) *
                                          static_cast<IGsize>(dims[2]);
            if (!geometry.points ||
                geometry.points->GetNumberOfPoints() != expectedPoints) return false;

            const igIndex strides[3] = {1, dims[0], dims[0] * dims[1]};
            int activeAxes[3] = {0, 0, 0};
            int dimension = 0;
            for (int axis = 0; axis < 3; ++axis) {
                if (dims[axis] > 1) activeAxes[dimension++] = axis;
            }
            geometry.fixedDimension = dimension;
            if (dimension == 3) {
                mesh->GenStructuredCellConnectivities();
                geometry.cells = mesh->GetVolumes();
                geometry.usesStructuredCellOrdering = true;
            } else if (dimension == 2 && dims[2] == 1) {
                // 常见 XY 面网格直接复用模型已有的面单元，避免大网格再复制一份。
                mesh->GenStructuredCellConnectivities();
                geometry.cells = mesh->GetFaces();
            } else if (dimension == 1 || dimension == 2) {
                // StructuredMesh 的 GetCells() 只返回体单元；其现有面生成逻辑
                // 也只覆盖 XY 平面。在这里按活动轴生成 XZ/YZ 面或一维线段。
                auto cells = CellArray::New();
                const int axis0 = activeAxes[0];
                const igIndex step0 = strides[axis0];
                if (dimension == 1) {
                    for (igIndex i = 0; i + 1 < dims[axis0]; ++i) {
                        const igIndex line[2] = {i * step0, (i + 1) * step0};
                        cells->AddCellIds(line, 2);
                    }
                } else {
                    const int axis1 = activeAxes[1];
                    const igIndex step1 = strides[axis1];
                    for (igIndex j = 0; j + 1 < dims[axis1]; ++j) {
                        for (igIndex i = 0; i + 1 < dims[axis0]; ++i) {
                            const igIndex base = i * step0 + j * step1;
                            const igIndex quad[4] = {base, base + step0,
                                                     base + step0 + step1,
                                                     base + step1};
                            cells->AddCellIds(quad, 4);
                        }
                    }
                }
                geometry.cells = cells;
            }
            break;
        }
        default:
            return false;
    }
    return geometry.points && geometry.cells;
}

// SurfaceMesh 同时保存 LINES 和 POLYGONS 时，文件中的单元属性按“边、面”
// 连续排列。只有数组长度与该布局完全一致时才使用偏移，避免把程序运行时
// 从面派生出的边误认为原始 Cell Data 的一部分。
bool ResolveCellAttributeOffset(const GeometryView& geometry,
                                const ArrayObject::Pointer& array,
                                IGsize& offset) {
    offset = 0;
    if (!array) return false;
    const IGsize selectedCellCount = geometry.cells->GetNumberOfCells();
    const IGsize elementCount = array->GetNumberOfElements();
    if (!geometry.usesSurfaceCellLayout) {
        return elementCount >= selectedCellCount;
    }
    if (elementCount == selectedCellCount) return true;

    const IGsize combinedCellCount =
            geometry.cellAttributeOffsetCandidate + selectedCellCount;
    if (geometry.cellAttributeOffsetCandidate > 0 &&
        elementCount == combinedCellCount) {
        offset = geometry.cellAttributeOffsetCandidate;
        return true;
    }
    return false;
}

// 读取 VTK 约定的单元 ghost 数组。数组缺失时按全部可见处理；
// 已存在但无法与当前单元布局对应时由 validLayout 报错。
std::vector<unsigned char> LoadCellGhostFlags(AttributeSet* attributes,
                                              const GeometryView& geometry,
                                              bool& validLayout) {
    const IGsize cellCount = geometry.cells->GetNumberOfCells();
    std::vector<unsigned char> flags(static_cast<size_t>(cellCount), 0);
    validLayout = true;
    if (!attributes) return flags;

    for (IGsize i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attr = attributes->GetAttribute(i);
        if (attr.IsNone() || attr.isDeleted || !attr.pointer ||
            attr.attachmentType != IG_CELL ||
            attr.pointer->GetName() != "vtkGhostType" ||
            attr.pointer->GetDimension() != 1) {
            continue;
        }

        IGsize cellOffset = 0;
        if (!ResolveCellAttributeOffset(geometry, attr.pointer, cellOffset)) {
            validLayout = false;
            return flags;
        }
        for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
            const double value =
                    attr.pointer->GetElementValue(cellOffset + cellId, 0);
            if (std::isfinite(value) && value >= 0.0 && value <= 255.0) {
                flags[static_cast<size_t>(cellId)] =
                        static_cast<unsigned char>(value);
            }
        }
        break;
    }
    return flags;
}

bool IsExcludedGhostCell(const std::vector<unsigned char>& flags, IGsize cellId) {
    return (flags[static_cast<size_t>(cellId)] & kExcludedGhostCellMask) != 0;
}

// 读取指定单元的顶点编号和坐标。
int LoadCellPoints(const GeometryView& geometry, IGsize cellId,
                   const igIndex*& pointIds, std::vector<Point>& cellPoints) {
    const int pointCount = geometry.cells->GetCellIds(cellId, pointIds);
    if (pointCount <= 0 || !pointIds) return 0;

    cellPoints.clear();
    cellPoints.reserve(static_cast<size_t>(pointCount));
    const IGsize totalPointCount = geometry.points->GetNumberOfPoints();
    for (int i = 0; i < pointCount; ++i) {
        if (pointIds[i] < 0 ||
            static_cast<IGsize>(pointIds[i]) >= totalPointCount) return 0;
        cellPoints.push_back(geometry.points->GetPoint(pointIds[i]));
    }
    return pointCount;
}

// 根据网格类型、单元类型和顶点数确定可支持的积分维度。
int ResolveIntegrationDimension(const GeometryView& geometry, IGsize cellId,
                                const std::vector<Point>& cellPoints) {
    const size_t pointCount = cellPoints.size();
    if (geometry.unstructured) {
        const IGenum cellType = geometry.unstructured->GetCellType(cellId);
        // 只接受当前积分拆分公式明确支持的线性单元；类型和顶点数必须同时
        // 匹配，不能把 IG_VOLUME、IG_POLYHEDRON 或畸形单元仅凭点数当成
        // 四面体、金字塔、三棱柱或六面体。
        switch (cellType) {
            case IG_LINE: return pointCount == 2 ? 1 : 0;
            case IG_POLY_LINE: return pointCount >= 2 ? 1 : 0;
            case IG_TRIANGLE: return pointCount == 3 ? 2 : 0;
            case IG_QUAD: return pointCount == 4 ? 2 : 0;
            case IG_FACE:
            case IG_POLYGON:
                return pointCount >= 3 ? 2 : 0;
            case IG_TETRA: return pointCount == 4 ? 3 : 0;
            case IG_PYRAMID: return pointCount == 5 ? 3 : 0;
            case IG_PRISM: return pointCount == 6 ? 3 : 0;
            case IG_HEXAHEDRON: return pointCount == 8 ? 3 : 0;
            default: return 0;
        }
    }
    if (geometry.hasUnsupportedPolyhedronTopology) return 0;
    if (geometry.fixedDimension == 1 && pointCount >= 2) return 1;
    if (geometry.fixedDimension == 2 && pointCount >= 3) return 2;
    if (geometry.fixedDimension == 3 &&
        (pointCount == 4 || pointCount == 5 ||
         pointCount == 6 || pointCount == 8)) return 3;
    return 0;
}

// 返回输入单元本身的拓扑维度，不考虑当前积分器是否支持该单元。这个维度
// 必须与“可积分维度”分开，否则不支持的三维单元会让二维单元错误成为最高维度。
int ResolveTopologicalDimension(const GeometryView& geometry, IGsize cellId) {
    if (!geometry.unstructured) return geometry.fixedDimension;

    switch (geometry.unstructured->GetCellType(cellId)) {
        case IG_LINE:
        case IG_POLY_LINE:
        case IG_QUADRATIC_EDGE:
        case IG_LAGRANGE_CURVE:
            return 1;

        case IG_FACE:
        case IG_TRIANGLE:
        case IG_QUAD:
        case IG_POLYGON:
        case IG_QUADRATIC_TRIANGLE:
        case IG_QUADRATIC_QUAD:
        case IG_QUADRATIC_POLYGON:
        case IG_BIQUADRATIC_QUAD:
        case IG_QUADRATIC_LINEAR_QUAD:
        case IG_BIQUADRATIC_TRIANGLE:
        case IG_LAGRANGE_TRIANGLE:
        case IG_LAGRANGE_QUADRILATERAL:
            return 2;

        case IG_VOLUME:
        case IG_POLYHEDRON:
        case IG_TETRA:
        case IG_HEXAHEDRON:
        case IG_PYRAMID:
        case IG_PRISM:
        case IG_QUADRATIC_TETRA:
        case IG_QUADRATIC_HEXAHEDRON:
        case IG_QUADRATIC_PRISM:
        case IG_QUADRATIC_PYRAMID:
        case IG_TRIQUADRATIC_HEXAHEDRON:
        case IG_TRIQUADRATIC_PYRAMID:
        case IG_QUADRATIC_LINEAR_WEDGE:
        case IG_BIQUADRATIC_QUADRATIC_WEDGE:
        case IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
        case IG_LAGRANGE_TETRAHEDRON:
        case IG_LAGRANGE_HEXAHEDRON:
        case IG_LAGRANGE_PRISM:
        case IG_LAGRANGE_PYRAMID:
            return 3;

        default:
            return 0;
    }
}

// 收集可参与积分的点属性和单元属性，并为每个属性创建累加器。
bool CollectAttributes(AttributeSet* attributes, const GeometryView& geometry,
                       IGsize pointCount,
                       const std::string& measureName,
                       std::vector<AttributeAccumulator>& pointAttributes,
                       std::vector<AttributeAccumulator>& cellAttributes) {
    if (!attributes) return true;
    for (IGsize i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attr = attributes->GetAttribute(i);
        if (attr.IsNone() || !attr.pointer || attr.isDeleted) continue;

        const int components = attr.pointer->GetDimension();
        if (components <= 0) continue;

        AttributeAccumulator accumulator;
        accumulator.input = attr.pointer;
        accumulator.type = attr.type;
        accumulator.attachment = attr.attachmentType;
        accumulator.values.assign(static_cast<size_t>(components), 0.0);

        if (attr.attachmentType == IG_POINT &&
            attr.pointer->GetNumberOfElements() >= pointCount) {
            pointAttributes.push_back(std::move(accumulator));
        } else if (attr.attachmentType == IG_CELL &&
                   attr.pointer->GetName() != measureName) {
            if (!ResolveCellAttributeOffset(
                        geometry, attr.pointer, accumulator.cellOffset)) {
                return false;
            }
            // ParaView 会用新生成的几何测度替换输出中同名的单元属性数组。
            cellAttributes.push_back(std::move(accumulator));
        }
    }
    return true;
}

// 将属性累加结果发布到输出数据，并按需将单元属性除以总测度。
void PublishAttributes(const std::vector<AttributeAccumulator>& accumulators,
                       AttributeSet* outputAttributes, bool divide,
                       double measure) {
    if (!outputAttributes) return;
    for (const auto& accumulator : accumulators) {
        auto output = DoubleArray::New();
        output->SetName(accumulator.input->GetName());
        output->SetDimension(static_cast<int>(accumulator.values.size()));
        std::vector<double> values = accumulator.values;
        if (divide && measure != 0.0) {
            for (double& value : values) value /= measure;
        }
        output->AddElement(values.data());
        outputAttributes->AddAttribute(
                accumulator.type, accumulator.attachment, output);
    }
}

} // 匿名命名空间

// 初始化过滤器的输入和输出端口数量。
IntegrateVariablesFilter::IntegrateVariablesFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

// 根据当前最高积分维度返回对应的几何测度名称。
std::string IntegrateVariablesFilter::GetMeasureName() const {
    switch (m_IntegrationDimension) {
        case 1: return "Length";
        case 2: return "Area";
        case 3: return "Volume";
        default: return "";
    }
}

// 执行变量积分，生成包含积分属性、总测度和积分中心的单点输出。
bool IntegrateVariablesFilter::Execute() {
    m_Message.clear();
    m_IntegrationDimension = 0;
    m_IntegratedCellCount = 0;
    m_IntegratedMeasure = 0.0;
    // Filter 对象可能被重复执行。先清空上次输出，避免本次失败后
    // 调用方继续拿到上一次的成功结果。
    SetOutput(nullptr);

    auto input = GetInput(0);
    if (!input) {
        m_Message = "No input data, please load and select a mesh first.";
        return false;
    }

    GeometryView geometry;
    if (!ResolveGeometry(input, geometry)) {
        m_Message = "Unsupported input. Integrate Variables requires a surface, volume, unstructured, or structured mesh.";
        return false;
    }
    const IGsize pointCount = geometry.points->GetNumberOfPoints();
    const IGsize cellCount = geometry.cells->GetNumberOfCells();
    bool validCellDataLayout = true;
    const auto cellGhostFlags = LoadCellGhostFlags(
            input->GetAttributeSet(), geometry, validCellDataLayout);
    if (!validCellDataLayout) {
        m_Message = "Cell data does not match the selected surface-cell layout.";
        return false;
    }
    // 与 ParaView 一致，混合维度网格只积分其中最高维度的单元。输入拓扑维度
    // 和当前支持的维度分别统计，避免不支持的三维单元被忽略后回退去积分二维单元。
    std::vector<int> cellTopologicalDimensions(static_cast<size_t>(cellCount), 0);
    std::vector<int> cellDimensions(static_cast<size_t>(cellCount), 0);
    int highestInputDimension = 0;
    const igIndex* pointIds = nullptr;
    std::vector<Point> cellPoints;
    for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
        const int topologicalDimension =
                ResolveTopologicalDimension(geometry, cellId);
        cellTopologicalDimensions[static_cast<size_t>(cellId)] =
                topologicalDimension;
        highestInputDimension = std::max(highestInputDimension,
                                         topologicalDimension);
        if (LoadCellPoints(geometry, cellId, pointIds, cellPoints) == 0) continue;
        const int dimension = ResolveIntegrationDimension(geometry, cellId, cellPoints);
        cellDimensions[static_cast<size_t>(cellId)] = dimension;
    }
    m_IntegrationDimension = highestInputDimension;
    if (highestInputDimension == 0) {
        m_Message = "No supported 1D, 2D, or 3D cells were found.";
        return false;
    }
    bool hasVisibleHighestDimensionCell = false;
    for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
        if (cellTopologicalDimensions[static_cast<size_t>(cellId)] !=
                    highestInputDimension ||
            IsExcludedGhostCell(cellGhostFlags, cellId)) {
            continue;
        }
        hasVisibleHighestDimensionCell = true;
        if (cellDimensions[static_cast<size_t>(cellId)] !=
                highestInputDimension) {
            m_Message = "A highest-dimensional cell uses an unsupported or malformed topology.";
            return false;
        }
    }

    const std::string measureName = GetMeasureName();
    std::vector<AttributeAccumulator> pointAttributes;
    std::vector<AttributeAccumulator> cellAttributes;
    if (!CollectAttributes(input->GetAttributeSet(), geometry, pointCount,
                           measureName, pointAttributes, cellAttributes)) {
        m_Message = "A cell-data array does not match the selected cells.";
        return false;
    }

    Vector3d weightedCenter(0.0, 0.0, 0.0); // 累加积分中心的分子部分，最后除以总测度得到积分中心坐标
    std::vector<double> pointWeights;

    for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
        if (IsExcludedGhostCell(cellGhostFlags, cellId)) continue;
        const int dimension = cellDimensions[static_cast<size_t>(cellId)];
        if (dimension != m_IntegrationDimension) {
            continue;
        }

        const int cellPointCount = LoadCellPoints(geometry, cellId, pointIds, cellPoints);
        if (cellPointCount == 0) continue;
        double measure = 0.0;
        if (!ComputeIntegrationWeights(dimension, cellPoints,
                                       geometry.usesStructuredCellOrdering,
                                       pointWeights, measure)) {
            continue;
        }

        ++m_IntegratedCellCount;
        m_IntegratedMeasure += measure;
        for (int localId = 0; localId < cellPointCount; ++localId) {
            const double weight = pointWeights[static_cast<size_t>(localId)];
            const Point& point = cellPoints[static_cast<size_t>(localId)];
            weightedCenter += Vector3d(point) * weight;

            for (auto& accumulator : pointAttributes) {
                for (size_t component = 0; component < accumulator.values.size(); ++component) {
                    accumulator.values[component] +=
                            accumulator.input->GetElementValue(pointIds[localId], static_cast<int>(component)) * weight;
                }
            }
        }

        for (auto& accumulator : cellAttributes) {
            for (size_t component = 0; component < accumulator.values.size(); ++component) {
                accumulator.values[component] +=
                        accumulator.input->GetElementValue(
                                accumulator.cellOffset + cellId,
                                static_cast<int>(component)) * measure;
            }
        }
    }

    if (m_IntegratedCellCount == 0 && hasVisibleHighestDimensionCell) {
        m_Message = "No cells of the highest integration dimension could be integrated.";
        return false;
    }

    Point outputPoint(0.0f, 0.0f, 0.0f);
    if (m_IntegratedMeasure != 0.0) {
        outputPoint = Point(weightedCenter / m_IntegratedMeasure);
    }

    auto output = UnstructuredMesh::New();
    auto outputPoints = Points::New();
    outputPoints->AddPoint(outputPoint);
    output->SetPoints(outputPoints);
    igIndex vertexId = 0;
    output->AddCell(&vertexId, 1, IG_VERTEX);

    auto outputAttributes = output->GetAttributeSet();
    PublishAttributes(pointAttributes, outputAttributes, false, m_IntegratedMeasure);
    PublishAttributes(cellAttributes, outputAttributes,
                      m_DivideAllCellDataByMeasure, m_IntegratedMeasure);

    auto measureArray = DoubleArray::New();
    measureArray->SetName(measureName);
    measureArray->SetDimension(1);
    measureArray->AddValue(m_IntegratedMeasure);
    outputAttributes->AddAttribute(IG_SCALAR, IG_CELL, measureArray);
    outputAttributes->ForceReConvertToDrawableData();

    output->SetName(input->GetName() + "_IntegrateVariables");
    SetOutput(output);
    m_Message = "Integrated " + std::to_string(m_IntegratedCellCount) +
                " cells; total " + measureName + " = " +
                std::to_string(m_IntegratedMeasure) + ".";
    return true;
}

IGAME_NAMESPACE_END
