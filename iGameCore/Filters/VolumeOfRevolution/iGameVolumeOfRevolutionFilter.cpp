#include "iGameVolumeOfRevolutionFilter.h"

#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameCellType.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVector.h"
#include "iGameVolumeMesh.h"

#include <cmath>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

constexpr double kPi = 3.141592653589793;
constexpr double kDoubleEpsilon = 2.2204460492503131e-16;

// 输入单元的归一化记录。cellId 用于让每个生成单元继承其来源单元的单元属性。
struct SourceCell {
    IGenum type{IG_EMPTY_CELL};
    std::vector<igIndex> ids;
    IGsize cellId{0};
};

std::string CellTypeName(IGenum type) {
    const char* name = GetCellTypeAsString(static_cast<int>(type));
    const std::string text = (name != nullptr && name[0] != '\0') ? std::string(name)
                                                                 : std::string("未知类型");
    return text + "(" + std::to_string(static_cast<int>(type)) + ")";
}

int TopologicalDimension(IGenum type) {
    switch (type) {
        case IG_VERTEX: return 0;
        case IG_LINE:
        case IG_POLY_LINE:
        case IG_QUADRATIC_EDGE:
        case IG_LAGRANGE_CURVE: return 1;
        case IG_FACE:
        case IG_TRIANGLE:
        case IG_QUAD:
        case IG_POLYGON:
        case IG_QUADRATIC_TRIANGLE:
        case IG_QUADRATIC_QUAD:
        case IG_QUADRATIC_POLYGON:
        case IG_LAGRANGE_TRIANGLE:
        case IG_LAGRANGE_QUADRILATERAL: return 2;
        default: return 3;
    }
}

/** 能否被扫掠 */
bool IsRevolvable(IGenum type) {
    switch (type) {
        case IG_VERTEX:
        case IG_LINE:
        case IG_POLY_LINE:
        case IG_TRIANGLE:
        case IG_QUAD:
        case IG_POLYGON:
        case IG_FACE: 
            return true;
        default: return false;
    }
}

/** 读取任意长度单元的点号。 */
bool ReadCellIds(CellArray* array, IGsize cellId, std::vector<igIndex>& ids) {
    ids.clear();
    if (array == nullptr) { return false; }
    const igIndex* raw = nullptr;
    const int size = array->GetCellIds(cellId, raw);
    if (size <= 0 || raw == nullptr) { return false; }
    ids.assign(raw, raw + size);
    return true;
}


Vector3d RevolvePoint(const Vector3d& in, const Vector3d& axisPosition, const Vector3d& axisDirection,
                      double angleInRadians) {
    const double c = std::cos(angleInRadians);
    const double s = std::sin(angleInRadians);
    const Vector3d translated = in - axisPosition;
    const double dot = translated * axisDirection;
    const Vector3d cross = translated.cross(axisDirection);
    Vector3d out;
    for (int i = 0; i < 3; ++i) {
        out[i] = translated[i] * c + axisDirection[i] * dot * (1.0 - c) - cross[i] * s +
                 axisPosition[i];
    }
    return out;
}

/** 按源数组类型创建一个同类型的空数组，指定元组数量。 */
ArrayObject::Pointer CreateEmptyArrayOfSameType(ArrayObject::Pointer src, IGsize numTuples) {
    if (!src) return nullptr;
    const int dim = src->GetDimension();
    const IGsize numValues = numTuples * dim;

    if (auto farr = DynamicCast<FloatArray>(src)) {
        auto copy = FloatArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto darr = DynamicCast<DoubleArray>(src)) {
        auto copy = DoubleArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto uarr = DynamicCast<UnsignedIntArray>(src)) {
        auto copy = UnsignedIntArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto iarr = DynamicCast<IntArray>(src)) {
        auto copy = IntArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto sarr = DynamicCast<ShortArray>(src)) {
        auto copy = ShortArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto usarr = DynamicCast<UnsignedShortArray>(src)) {
        auto copy = UnsignedShortArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto carr = DynamicCast<CharArray>(src)) {
        auto copy = CharArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto ucarr = DynamicCast<UnsignedCharArray>(src)) {
        auto copy = UnsignedCharArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto llarr = DynamicCast<LongLongArray>(src)) {
        auto copy = LongLongArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto ullarr = DynamicCast<UnsignedLongLongArray>(src)) {
        auto copy = UnsignedLongLongArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }

    igError("CreateEmptyArrayOfSameType: unsupported array type.");
    return nullptr;
}

bool CollectSourceCells(const DataObject::Pointer& input, std::vector<SourceCell>& cells,
                        Points::Pointer& points, std::string& message) {
    cells.clear();
    points = nullptr;

    auto takePoints = [&](PointSet::Pointer pointSet) -> bool {
        auto pts = pointSet->GetPoints();
        if (!pts || pts->GetNumberOfPoints() == 0) {
            message = "输入网格没有点，无法生成旋转体。";
            return false;
        }
        points = pts;
        return true;
    };

    // ---------- 非结构化网格：单元类型以类型数组为准 ----------
    if (auto mesh = DynamicCast<UnstructuredMesh>(input)) {
        if (!takePoints(mesh)) return false;
        CellArray* cellArray = mesh->GetCells();
        const IGsize numCells = mesh->GetNumberOfCells();
        if (cellArray == nullptr || numCells == 0) {
            message = "UnstructuredMesh 输入没有单元；点云不能作为旋转截面。";
            return false;
        }
        for (IGsize cellId = 0; cellId < numCells; ++cellId) {
            const IGenum type = mesh->GetCellType(cellId);
            if (type == IG_EMPTY_CELL) continue;
            if (TopologicalDimension(type) > 2) {
                message = "输入包含体单元（单元 " + std::to_string(cellId) + " 为 " +
                          CellTypeName(type) +
                          "）：所有单元的拓扑维数必须不超过 2，旋转体扫掠整体停止。";
                return false;
            }
            SourceCell cell;
            cell.type = type;
            cell.cellId = cellId;
            if (!ReadCellIds(cellArray, cellId, cell.ids)) continue;
            cells.push_back(std::move(cell));
        }
        return true;
    }

    // ---------- 曲面网格：逐面片旋转 ----------
    if (auto surface = DynamicCast<SurfaceMesh>(input)) {
        if (!takePoints(surface)) return false;
        CellArray* faces = surface->GetFaces();
        const IGsize numFaces = faces ? faces->GetNumberOfCells() : 0;
        if (numFaces == 0) {
            message = "SurfaceMesh 输入没有面片，没有可旋转的二维单元。";
            return false;
        }
        for (IGsize faceId = 0; faceId < numFaces; ++faceId) {
            SourceCell cell;
            cell.cellId = faceId;
            if (!ReadCellIds(faces, faceId, cell.ids)) continue;
            cell.type = SurfaceMesh::GetFaceTypeWithPointNum(static_cast<int>(cell.ids.size()));
            if (cell.type == IG_EMPTY_CELL) continue;
            cells.push_back(std::move(cell));
        }
        return true;
    }

    // ---------- 结构网格：二维曲面逐四边形旋转，一维曲线逐线段旋转 ----------
    if (auto structured = DynamicCast<StructuredMesh>(input)) {
        if (!takePoints(structured)) return false;
        const igIndex* dim = structured->GetDimensionSize();
        const IGsize ni = static_cast<IGsize>(dim[0]);
        const IGsize nj = static_cast<IGsize>(dim[1]);
        const IGsize nk = static_cast<IGsize>(dim[2]);
        if (ni < 1 || nj < 1 || nk < 1) {
            message = "StructuredMesh 输入存在退化维度（" + std::to_string(ni) + "x" +
                      std::to_string(nj) + "x" + std::to_string(nk) + "）。";
            return false;
        }
        if (nk > 1) {
            message = "StructuredMesh 输入是三维体网格（" + std::to_string(ni) + "x" +
                      std::to_string(nj) + "x" + std::to_string(nk) +
                      "）：所有单元的拓扑维数必须不超过 2，旋转体扫掠整体停止。";
            return false;
        }
        if (nj == 1) {
            // 一维曲线：相邻两点连成线段，不把点云自动串联。
            for (IGsize i = 0; i + 1 < ni; ++i) {
                SourceCell cell;
                cell.type = IG_LINE;
                cell.cellId = i;
                cell.ids = {structured->GetPointIndex(i, 0, 0), structured->GetPointIndex(i + 1, 0, 0)};
                cells.push_back(std::move(cell));
            }
            return true;
        }
        structured->GenStructuredCellConnectivities();
        CellArray* faces = structured->GetFaces();
        const IGsize numFaces = faces ? faces->GetNumberOfCells() : 0;
        if (numFaces == 0) {
            message = "StructuredMesh 输入没有可旋转的二维单元。";
            return false;
        }
        for (IGsize faceId = 0; faceId < numFaces; ++faceId) {
            SourceCell cell;
            cell.cellId = faceId;
            if (!ReadCellIds(faces, faceId, cell.ids)) continue;
            cell.type = SurfaceMesh::GetFaceTypeWithPointNum(static_cast<int>(cell.ids.size()));
            if (cell.type == IG_EMPTY_CELL) continue;
            cells.push_back(std::move(cell));
        }
        return true;
    }

    // ---------- 体网格：全部是三维单元，直接拒绝 ----------
    if (DynamicCast<VolumeMesh>(input)) {
        message = "输入是体网格，所有单元的拓扑维数都是 3；旋转体扫掠要求所有单元维数不超过 2"
                  "（需要二维截面单元，如三角形/四边形）。";
        return false;
    }

    message = "不支持该输入数据类型（" + std::to_string(static_cast<int>(input->GetDataObjectType())) +
              "）；支持的输入为 UnstructuredMesh、SurfaceMesh 和 StructuredMesh。";
    return false;
}

} // namespace

// ---------- 主执行函数 ----------
bool VolumeOfRevolutionFilter::Execute() {
    m_Message.clear();
    this->SetOutput(0, nullptr);

    DataObject::Pointer input = GetInput(0);
    if (!input) {
        m_Message = "旋转体生成：没有输入数据对象。";
        igError("{}", m_Message);
        return false;
    }

    std::vector<SourceCell> sourceCells;
    Points::Pointer inputPoints;
    std::string buildMessage;
    if (!CollectSourceCells(input, sourceCells, inputPoints, buildMessage)) {
        m_Message = buildMessage;
        igError("{}", m_Message);
        return false;
    }
    if (sourceCells.empty()) {
        m_Message = "输入中没有可旋转的单元。";
        igError("{}", m_Message);
        return false;
    }

    // -------- 旋转轴/角度/分段数 --------
    Vector3d axisDirection = m_AxisDirection;
    const double axisLength = axisDirection.norm();
    if (!std::isfinite(axisLength) || axisLength < 1e-12) {
        m_Message = "旋转轴方向必须是非零且有限的向量。";
        igError("{}", m_Message);
        return false;
    }
    axisDirection /= axisLength;

    const Vector3d axisPosition = m_AxisPoint;
    for (int c = 0; c < 3; ++c) {
        if (!std::isfinite(axisPosition[c])) {
            m_Message = "轴上一点的坐标必须是有限值。";
            igError("{}", m_Message);
            return false;
        }
    }

    const double angle = m_Angle;
    if (!std::isfinite(angle)) {
        m_Message = "旋转角度必须是有限值。";
        igError("{}", m_Message);
        return false;
    }
    if (std::fabs(angle) < 1e-12) {
        m_Message = "旋转角度为 0；结果会退化为零体积。";
        igError("{}", m_Message);
        return false;
    }
    if (m_Resolution < 3) {
        m_Message = "分段数至少为 3。";
        igError("{}", m_Message);
        return false;
    }

    // -------- 扫掠角度参数 --------
    const bool partialSweep =
            std::fabs(2.0 * kPi - std::fabs(angle)) > 1024.0 * kDoubleEpsilon;
    const int numLayers = m_Resolution + (partialSweep ? 1 : 0);
    const double angleStep = angle / static_cast<double>(m_Resolution);

    // -------- 生成旋转点云：层优先，index = layer * 输入点数 + 输入点号 --------
    const IGsize numPoints = inputPoints->GetNumberOfPoints();
    auto newPoints = Points::New();
    for (int layer = 0; layer < numLayers; ++layer) {
        const double theta = static_cast<double>(layer) * angleStep;
        for (IGsize pointId = 0; pointId < numPoints; ++pointId) {
            Vector3d in;
            inputPoints->GetPoint(pointId, in);
            const Vector3d out = RevolvePoint(in, axisPosition, axisDirection, theta);
            newPoints->AddPoint(out[0], out[1], out[2]);
        }
    }

    // -------- 逐单元扫掠 --------
    auto newCells = CellArray::New();
    auto newTypes = UnsignedIntArray::New();
    std::vector<IGsize> outCellSource; // 每个输出单元对应的来源单元号
    outCellSource.reserve(static_cast<size_t>(sourceCells.size()) * m_Resolution);

    auto outputPointId = [numPoints](int layer, IGsize pointId) -> igIndex {
        return static_cast<igIndex>(layer) * static_cast<igIndex>(numPoints) +
               static_cast<igIndex>(pointId);
    };
    auto layerAfter = [numLayers](int layer) -> int { return (layer + 1) % numLayers; };

    IGsize skippedCells = 0;
    IGenum firstSkippedType = IG_EMPTY_CELL;

    for (const auto& cell : sourceCells) {
        if (!IsRevolvable(cell.type) || cell.ids.empty()) {
            if (skippedCells == 0) firstSkippedType = cell.type;
            ++skippedCells;
            continue;
        }

        const IGsize numCellPoints = cell.ids.size();

        // 点：扫成线
        if (cell.type == IG_VERTEX) {
            for (int layer = 0; layer < m_Resolution; ++layer) {
                const igIndex line[2] = {outputPointId(layer, cell.ids[0]),
                                         outputPointId(layerAfter(layer), cell.ids[0])};
                newCells->AddCellIds(line, 2);
                newTypes->AddValue(IG_LINE);
                outCellSource.push_back(cell.cellId);
            }
            continue;
        }

        // 线段/折线：扫成四边形
        if (cell.type == IG_LINE || cell.type == IG_POLY_LINE) {
            for (IGsize segment = 0; segment + 1 < numCellPoints; ++segment) {
                const IGsize a = cell.ids[segment];
                const IGsize b = cell.ids[segment + 1];
                for (int layer = 0; layer < m_Resolution; ++layer) {
                    const int next = layerAfter(layer);
                    const igIndex quad[4] = {outputPointId(layer, a), outputPointId(layer, b),
                                             outputPointId(next, b), outputPointId(next, a)};
                    newCells->AddCellIds(quad, 4);
                    newTypes->AddValue(IG_QUAD);
                    outCellSource.push_back(cell.cellId);
                }
            }
            continue;
        }

        // 三角形：扫成楔形体
        if (cell.type == IG_TRIANGLE) {
            for (int layer = 0; layer < m_Resolution; ++layer) {
                const int next = layerAfter(layer);
                const igIndex prism[6] = {outputPointId(layer, cell.ids[0]),
                                          outputPointId(layer, cell.ids[1]),
                                          outputPointId(layer, cell.ids[2]),
                                          outputPointId(next, cell.ids[0]),
                                          outputPointId(next, cell.ids[1]),
                                          outputPointId(next, cell.ids[2])};
                newCells->AddCellIds(prism, 6);
                newTypes->AddValue(IG_PRISM);
                outCellSource.push_back(cell.cellId);
            }
            continue;
        }

        // 四边形：扫成六面体
        if (cell.type == IG_QUAD) {
            for (int layer = 0; layer < m_Resolution; ++layer) {
                const int next = layerAfter(layer);
                const igIndex hexahedron[8] = {outputPointId(layer, cell.ids[0]),
                                               outputPointId(layer, cell.ids[1]),
                                               outputPointId(layer, cell.ids[2]),
                                               outputPointId(layer, cell.ids[3]),
                                               outputPointId(next, cell.ids[0]),
                                               outputPointId(next, cell.ids[1]),
                                               outputPointId(next, cell.ids[2]),
                                               outputPointId(next, cell.ids[3])};
                newCells->AddCellIds(hexahedron, 8);
                newTypes->AddValue(IG_HEXAHEDRON);
                outCellSource.push_back(cell.cellId);
            }
            continue;
        }

        // 多边形/面单元：扫成多面体。连接表用核心约定的编码：
        const IGsize n = numCellPoints;
        std::vector<igIndex> encoded(static_cast<size_t>(7 * n + 3), 0);
        for (int layer = 0; layer < m_Resolution; ++layer) {
            const int next = layerAfter(layer);
            encoded[0] = static_cast<igIndex>(n + 2);
            encoded[1] = static_cast<igIndex>(n);
            for (IGsize j = 0; j < n; ++j) {
                encoded[2 + j] = outputPointId(layer, cell.ids[j]);
            }
            encoded[2 + n] = static_cast<igIndex>(n);
            for (IGsize m = 0; m < n; ++m) {
                encoded[3 + n + m] = outputPointId(next, cell.ids[n - 1 - m]);
            }
            for (IGsize j = 0; j < n; ++j) {
                const IGsize base = 3 + 2 * n + 5 * j;
                encoded[base + 0] = 4;
                encoded[base + 1] = outputPointId(layer, cell.ids[j]);
                encoded[base + 2] = outputPointId(layer, cell.ids[(j + 1) % n]);
                encoded[base + 3] = outputPointId(next, cell.ids[(j + 1) % n]);
                encoded[base + 4] = outputPointId(next, cell.ids[j]);
            }
            newCells->AddCellIds(encoded.data(), static_cast<int>(encoded.size()));
            newTypes->AddValue(IG_POLYHEDRON);
            outCellSource.push_back(cell.cellId);
        }
    }

    if (newCells->GetNumberOfCells() == 0) {
        m_Message = "生成的旋转体没有单元；输入中没有可扫掠的一维/二维单元。";
        igError("{}", m_Message);
        return false;
    }

    // -------- 输出 --------
    auto outputMesh = UnstructuredMesh::New();
    outputMesh->SetPoints(newPoints);
    outputMesh->SetCells(newCells, newTypes);

    // -------- 复制点属性：每个输入点复制到所有层 --------
    auto inputAttrs = input->GetAttributeSet();
    auto outputAttrs = outputMesh->GetAttributeSet();
    if (!outputAttrs) {
        outputAttrs = AttributeSet::New();
        outputMesh->SetAttributeSet(outputAttrs);
    }

    if (inputAttrs) {
        auto pointAttrs = inputAttrs->GetAllPointAttributes();
        if (pointAttrs && pointAttrs->GetNumberOfElements() > 0) {
            const IGsize numOutPoints = newPoints->GetNumberOfPoints();
            for (int a = 0; a < pointAttrs->GetNumberOfElements(); ++a) {
                auto& attr = pointAttrs->GetElement(a);
                if (attr.IsDeleted() || !attr.pointer) continue;

                auto inArray = attr.pointer;
                const int dim = inArray->GetDimension();
                auto outArray = CreateEmptyArrayOfSameType(inArray, numOutPoints);
                if (!outArray) continue;
                outArray->SetName(inArray->GetName());

                for (int layer = 0; layer < numLayers; ++layer) {
                    for (IGsize pointId = 0; pointId < numPoints; ++pointId) {
                        const IGsize outIndex = static_cast<IGsize>(layer) * numPoints + pointId;
                        for (int d = 0; d < dim; ++d) {
                            outArray->SetValue(outIndex * dim + d, inArray->GetValue(pointId * dim + d));
                        }
                    }
                }

                DoubleArray::Pointer range = attr.GetDataRange();
                if (attr.type == IG_SCALAR) {
                    if (range) outputAttrs->AddScalar(IG_POINT, outArray, range);
                    else outputAttrs->AddScalar(IG_POINT, outArray);
                } else if (attr.type == IG_VECTOR) {
                    if (range) outputAttrs->AddVector(IG_POINT, outArray, range);
                    else outputAttrs->AddVector(IG_POINT, outArray);
                } else {
                    if (range) outputAttrs->AddAttribute(attr.type, IG_POINT, outArray, range);
                    else outputAttrs->AddAttribute(attr.type, IG_POINT, outArray);
                }
            }
        }

        // -------- 复制单元属性：每个生成单元继承其来源单元的属性 --------
        auto cellAttrs = inputAttrs->GetAllCellAttributes();
        if (cellAttrs && cellAttrs->GetNumberOfElements() > 0) {
            const IGsize numOutCells = static_cast<IGsize>(outCellSource.size());
            for (int a = 0; a < cellAttrs->GetNumberOfElements(); ++a) {
                auto& attr = cellAttrs->GetElement(a);
                if (attr.IsDeleted() || !attr.pointer) continue;

                auto inArray = attr.pointer;
                const int dim = inArray->GetDimension();
                const IGsize numInCells = (dim > 0) ? inArray->GetNumberOfValues() / dim : 0;
                auto outArray = CreateEmptyArrayOfSameType(inArray, numOutCells);
                if (!outArray) continue;
                outArray->SetName(inArray->GetName());

                for (IGsize t = 0; t < numOutCells; ++t) {
                    const IGsize sourceCellId = outCellSource[static_cast<size_t>(t)];
                    if (sourceCellId < numInCells) {
                        for (int d = 0; d < dim; ++d) {
                            outArray->SetValue(t * dim + d, inArray->GetValue(sourceCellId * dim + d));
                        }
                    } else {
                        for (int d = 0; d < dim; ++d) outArray->SetValue(t * dim + d, 0.0);
                    }
                }

                DoubleArray::Pointer range = attr.GetDataRange();
                if (attr.type == IG_SCALAR) {
                    if (range) outputAttrs->AddScalar(IG_CELL, outArray, range);
                    else outputAttrs->AddScalar(IG_CELL, outArray);
                } else if (attr.type == IG_VECTOR) {
                    if (range) outputAttrs->AddVector(IG_CELL, outArray, range);
                    else outputAttrs->AddVector(IG_CELL, outArray);
                } else {
                    if (range) outputAttrs->AddAttribute(attr.type, IG_CELL, outArray, range);
                    else outputAttrs->AddAttribute(attr.type, IG_CELL, outArray);
                }
            }
        }
    }

    if (skippedCells > 0) {
        m_Message = "已跳过 " + std::to_string(skippedCells) + " 个不支持扫掠的单元（例如 " +
                    CellTypeName(firstSkippedType) + "）；支持扫掠的单元为顶点、线段/折线、"
                    "三角形、四边形、多边形。";
        ::iGame::Log::GetCoreLogger()->warn("{}", m_Message);
    }

    SetOutput(0, outputMesh);
    return true;
}

IGAME_NAMESPACE_END
