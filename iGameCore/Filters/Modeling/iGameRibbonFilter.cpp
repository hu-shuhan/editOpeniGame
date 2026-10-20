#include "iGameRibbonFilter.h"

#include <iGameArrayObject.h>
#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameCellType.h>
#include <iGameFlatArray.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <cmath>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

/*
 * 逐行对齐 vtkRibbonFilter（Filters/Modeling/vtkRibbonFilter.cxx）：
 *   - 每条线单元独立计算框架，共享点不会被邻居折线的切向污染；
 *   - 平均切向 s = normalize((sPrev + sNext) / 2) 产生转角倒角，s 为零（180° 回折）
 *     时退回 VTK 的备用 bevel 向量 cross(sPrev, n)；
 *   - w = normalize(cross(s, n))，nP = normalize(cross(w, s))，带宽方向
 *     v = w * cos(Angle) + nP * sin(Angle)，两侧点 p∓ = p ∓ Width * v；
 *   - 输出点顺序与 VTK 相同：偶数索引是减号侧（p-）、奇数索引是加号侧（p+）。
 * iGame 没有可持久化的 triangle strip 容器（VTK reader 对 TRIANGLESTRIP 也是直接
 * 跳过、不产出单元），因此把 VTK 的 strip 展开成每段一个 IG_QUAD；四边形顶点必须
 * 按环状顺序 (p-ᵢ, p+ᵢ, p+ᵢ₊₁, p-ᵢ₊₁)，否则 iGame 的扇形三角化会拆出自交的蝴蝶结
 * （线性拉伸侧裙已经踩过这个坑，见 iGameLinearExtrusionFilter.cpp 的注释）。
 */

namespace {

// 输入适配记录：统一保存来自 UnstructuredMesh cell 或 SurfaceMesh edge 的线单元。
// 这只是输入适配用的临时结构，不是新的底层数据类。
struct InputLine {
    std::vector<igIndex> pointIds;
    igIndex sourceCellId{-1};
    IGenum sourceType{IG_EMPTY_CELL};
};

// 单元类型名：GetCellTypeAsString() 对 IG_POLY_LINE 会落到 default 返回空串，
// 而报错信息必须要能指出类型，所以这里自备一份。
const char* LineTypeName(IGenum type) {
    switch (type) {
        case IG_VERTEX:
            return "Vertex";
        case IG_LINE:
            return "Line";
        case IG_POLY_LINE:
            return "PolyLine";
        case IG_TRIANGLE:
            return "Triangle";
        case IG_QUAD:
            return "Quad";
        case IG_POLYGON:
            return "Polygon";
        case IG_TETRA:
            return "Tetra";
        case IG_HEXAHEDRON:
            return "Hexahedron";
        case IG_PYRAMID:
            return "Pyramid";
        case IG_PRISM:
            return "Prism";
        default:
            return "UnsupportedCellType";
    }
}

// 按输入数组的具体类型创建同类型空数组（不把数组偷换成 DoubleArray）。
// 与线性拉伸保持一致：两个 filter 各自持有该帮助函数，避免改动已合并的实现。
ArrayObject::Pointer CreateArrayByTypeExact(IGenum dataType) {
    switch (dataType) {
        case IG_FloatArray:
            return FloatArray::New();
        case IG_DoubleArray:
            return DoubleArray::New();
        case IG_IntArray:
            return IntArray::New();
        case IG_UnsignedIntArray:
            return UnsignedIntArray::New();
        case IG_CharArray:
            return CharArray::New();
        case IG_UnsignedCharArray:
            return UnsignedCharArray::New();
        case IG_ShortArray:
            return ShortArray::New();
        case IG_UnsignedShortArray:
            return UnsignedShortArray::New();
        case IG_LongLongArray:
            return LongLongArray::New();
        case IG_UnsignedLongLongArray:
            return UnsignedLongLongArray::New();
        default:
            return nullptr;
    }
}

// 在输入的点属性中查找可用的点法向数组（UseDefaultNormal 关时按 VTK 使用输入法向）。
// 只按「挂载类型 + 属性类型 + 维度 + 元组数」判断，不靠数组名猜测。
ArrayObject* FindPointNormalArray(AttributeSet* attributeSet, IGsize numberOfPoints) {
    if (attributeSet == nullptr) { return nullptr; }
    auto pointAttributes = attributeSet->GetAllPointAttributes();
    if (pointAttributes == nullptr) { return nullptr; }

    for (IGsize i = 0; i < pointAttributes->GetNumberOfElements(); ++i) {
        auto& attribute = pointAttributes->GetElement(i);
        if (attribute.IsNone() || attribute.pointer == nullptr) { continue; }
        if (attribute.attachmentType != IG_POINT || attribute.type != IG_NORMAL) { continue; }
        if (attribute.pointer->GetDimension() < 3) { continue; }
        if (attribute.pointer->GetNumberOfElements() < numberOfPoints) { continue; }
        return attribute.pointer;
    }
    return nullptr;
}

// 把输入的点 / 单元属性按来源映射复制到输出，保留数组名、具体类型、维度与数据范围。
// 与 VTK 的 CopyNormalsOff() 一致：旧法向不复制（几何已经改变，旧法向不再成立）。
void CopyAttributesBySourceId(DataObject* input, DataObject* output, IGsize numberOfOutputElements,
                              const std::vector<igIndex>& sourceIds, IGenum attachmentType) {
    if (input == nullptr || output == nullptr) { return; }

    auto inputAttributeSet = input->GetAttributeSet();
    if (inputAttributeSet == nullptr) { return; }
    auto inputAttributes = (attachmentType == IG_POINT) ? inputAttributeSet->GetAllPointAttributes()
                                                        : inputAttributeSet->GetAllCellAttributes();
    if (inputAttributes == nullptr || inputAttributes->GetNumberOfElements() == 0) { return; }

    AttributeSet::Pointer outputAttributeSet = output->GetAttributeSet();
    if (outputAttributeSet == nullptr) {
        outputAttributeSet = AttributeSet::New();
        output->SetAttributeSet(outputAttributeSet);
    }

    for (IGsize i = 0; i < inputAttributes->GetNumberOfElements(); ++i) {
        auto& attribute = inputAttributes->GetElement(i);
        if (attribute.IsNone() || attribute.pointer == nullptr) { continue; }
        if (attribute.type == IG_NORMAL) { continue; }

        ArrayObject::Pointer inputArray = attribute.pointer;
        const int dimension = inputArray->GetDimension();
        if (dimension <= 0) { continue; }
        const IGsize numberOfInputElements = inputArray->GetNumberOfElements();

        ArrayObject::Pointer outputArray = CreateArrayByTypeExact(inputArray->GetArrayType());
        if (outputArray == nullptr) { continue; }
        outputArray->SetName(inputArray->GetName());
        outputArray->SetDimension(dimension);
        outputArray->Resize(numberOfOutputElements);

        for (IGsize outId = 0; outId < numberOfOutputElements; ++outId) {
            const igIndex sourceId = sourceIds[outId];
            if (sourceId < 0 || static_cast<IGsize>(sourceId) >= numberOfInputElements) {
                // 来源单元没有对应的输入元组：明确写 0，不留下未初始化内存。
                for (int d = 0; d < dimension; ++d) { outputArray->SetValue(outId * dimension + d, 0.0); }
                continue;
            }
            for (int d = 0; d < dimension; ++d) {
                outputArray->SetValue(outId * dimension + d,
                                      inputArray->GetValue(static_cast<IGsize>(sourceId) * dimension + d));
            }
        }

        outputAttributeSet->AddAttribute(attribute.type, attachmentType, outputArray, attribute.GetDataRange());
    }
}

}  // namespace

bool RibbonFilter::Execute() {
    m_Message.clear();
    // 执行失败时不保留上一次的结果
    this->SetOutput(0, nullptr);

    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "RibbonFilter has no input DataObject.";
        return false;
    }

    auto unstructuredMesh = DynamicCast<UnstructuredMesh>(input);
    auto surfaceMesh = DynamicCast<SurfaceMesh>(input);
    if (unstructuredMesh == nullptr && surfaceMesh == nullptr) {
        m_Message = "RibbonFilter requires an UnstructuredMesh with line cells or a SurfaceMesh with explicit edges.";
        return false;
    }

    // 后续代码只依赖 inputPoints，不再关心具体网格类型。
    Points::Pointer inputPoints =
            (unstructuredMesh != nullptr) ? unstructuredMesh->GetPoints() : surfaceMesh->GetPoints();
    if (inputPoints == nullptr || inputPoints->GetNumberOfPoints() == 0) {
        m_Message = "RibbonFilter input has no points.";
        return false;
    }
    const IGsize numberOfInputPoints = inputPoints->GetNumberOfPoints();

    // 非致命提示累积到 notes，成功时写入 m_Message；失败时与原因一起给出。
    std::string notes;
    auto addNote = [&notes](const std::string& text) {
        if (!notes.empty()) { notes += " "; }
        notes += text;
    };
    auto withNotes = [&notes](const std::string& text) { return notes.empty() ? text : notes + " " + text; };

    // ------------------------------------------------------------------
    // 1. 输入适配：收集线单元，统计被忽略的非线单元。
    // ------------------------------------------------------------------
    std::vector<InputLine> inputLines;

    if (unstructuredMesh != nullptr) {
        CellArray::Pointer cells = unstructuredMesh->GetCells();
        UnsignedIntArray* types = unstructuredMesh->GetCellTypes();
        if (cells == nullptr || types == nullptr || cells->GetNumberOfCells() == 0) {
            m_Message = "RibbonFilter input UnstructuredMesh has no cells.";
            return false;
        }
        const IGsize numberOfCells = cells->GetNumberOfCells();
        std::string ignoredTypes;
        IGsize ignoredCells = 0;
        inputLines.reserve(numberOfCells);
        for (IGsize cellId = 0; cellId < numberOfCells; ++cellId) {
            const IGenum type = static_cast<IGenum>(types->GetValue(cellId));
            if (type != IG_LINE && type != IG_POLY_LINE) {
                // 与 VTK 只遍历 GetLines() 一致：不参与生成的单元只统计，不静默丢弃信息。
                ++ignoredCells;
                const std::string name = LineTypeName(type);
                if (ignoredTypes.find(name) == std::string::npos) {
                    if (!ignoredTypes.empty()) { ignoredTypes += ", "; }
                    ignoredTypes += name;
                }
                continue;
            }
            const igIndex* pointIds = nullptr;
            const int count = cells->GetCellIds(cellId, pointIds);
            if (count < 2 || pointIds == nullptr) {
                addNote("RibbonFilter skipped " + std::string(LineTypeName(type)) + " cell " + std::to_string(cellId) +
                        " which has fewer than two points.");
                continue;
            }
            InputLine line;
            line.pointIds.assign(pointIds, pointIds + count);
            line.sourceCellId = static_cast<igIndex>(cellId);
            line.sourceType = type;
            inputLines.push_back(std::move(line));
        }
        if (ignoredCells > 0) {
            addNote("RibbonFilter only processes line cells; ignored " + std::to_string(ignoredCells) +
                    " non-line cell(s) of type " + ignoredTypes + ".");
        }
    } else {
        // SurfaceMesh 只认文件里显式存在的边数组：不从面自动抽边（BuildEdges 既昂贵又不符合 VTK 语义）。
        CellArray* edges = surfaceMesh->GetEdges();
        if (edges == nullptr || edges->GetNumberOfCells() == 0) {
            m_Message = "RibbonFilter input SurfaceMesh has no explicit edges; "
                        "load a file with LINES/POLY_LINES or use an UnstructuredMesh with IG_LINE/IG_POLY_LINE cells.";
            return false;
        }
        const IGsize numberOfEdges = edges->GetNumberOfCells();
        inputLines.reserve(numberOfEdges);
        for (IGsize edgeId = 0; edgeId < numberOfEdges; ++edgeId) {
            const igIndex* pointIds = nullptr;
            const int count = edges->GetCellIds(edgeId, pointIds);
            if (count < 2 || pointIds == nullptr) {
                addNote("RibbonFilter skipped edge " + std::to_string(edgeId) + " which has fewer than two points.");
                continue;
            }
            InputLine line;
            line.pointIds.assign(pointIds, pointIds + count);
            line.sourceCellId = static_cast<igIndex>(edgeId);
            line.sourceType = (count == 2) ? static_cast<IGenum>(IG_LINE) : static_cast<IGenum>(IG_POLY_LINE);
            inputLines.push_back(std::move(line));
        }
    }

    if (inputLines.empty()) {
        m_Message = withNotes("RibbonFilter found no line cell to ribbon in the input.");
        return false;
    }

    // ------------------------------------------------------------------
    // 2. 确定法向来源：UseDefaultNormal 开 -> 默认法向；
    //    关 -> 优先输入点法向，没有点法向时回退默认法向并说明（首版没有滑动法向）。
    // ------------------------------------------------------------------
    ArrayObject* normalArray = nullptr;
    if (!m_UseDefaultNormal) {
        normalArray = FindPointNormalArray(input->GetAttributeSet(), numberOfInputPoints);
        if (normalArray == nullptr) {
            addNote("RibbonFilter found no usable point normals; UseDefaultNormal is off, "
                    "so the DefaultNormal is used instead.");
        }
    }
    const bool useDefaultNormal = (normalArray == nullptr);
    Vector3d defaultNormal = m_DefaultNormal;
    if (useDefaultNormal && defaultNormal.normalize() == 0.0) {
        m_Message = withNotes("RibbonFilter requires a non-zero DefaultNormal.");
        return false;
    }

    // ------------------------------------------------------------------
    // 3. 逐条折线计算框架并生成点与四边形。
    // ------------------------------------------------------------------
    const double kPi = 3.14159265358979323846;
    const double theta = m_Angle * kPi / 180.0;
    const double cosTheta = std::cos(theta);
    const double sinTheta = std::sin(theta);

    if (!(m_Width > 0.0)) {
        m_Message = withNotes("RibbonFilter requires Width > 0 (Width is the ribbon half width).");
        return false;
    }

    auto outputPoints = Points::New();
    std::vector<igIndex> sourcePointIds;
    auto outputFaces = CellArray::New();
    std::vector<igIndex> sourceFaceIds;

    for (const InputLine& line: inputLines) {
        const std::vector<igIndex>& ids = line.pointIds;
        const size_t numberOfLinePoints = ids.size();

        // 先把整条折线的框架算完再写入输出：任一退化条件命中就整条跳过，
        // 不会像 VTK 那样在输出里留下没有被单元引用的孤立点。
        std::vector<Vector3d> minusPoints(numberOfLinePoints);
        std::vector<Vector3d> plusPoints(numberOfLinePoints);
        std::string reason;

        Vector3d p;
        Vector3d pNext;
        inputPoints->GetPoint(ids[0], p);
        inputPoints->GetPoint(ids[1], pNext);
        Vector3d sNext = pNext - p;
        Vector3d sPrev = sNext;

        for (size_t j = 0; j < numberOfLinePoints; ++j) {
            if (j == 0) {
                // p / pNext / sNext / sPrev 已在循环外初始化
            } else if (j == numberOfLinePoints - 1) {
                sPrev = sNext;
                p = pNext;
            } else {
                p = pNext;
                inputPoints->GetPoint(ids[j + 1], pNext);
                sPrev = sNext;
                sNext = pNext - p;
            }

            Vector3d n;
            if (useDefaultNormal) {
                n = defaultNormal;
            } else {
                const int normalDimension = normalArray->GetDimension();
                n = Vector3d(normalArray->GetValue(ids[j] * normalDimension + 0),
                             normalArray->GetValue(ids[j] * normalDimension + 1),
                             normalArray->GetValue(ids[j] * normalDimension + 2));
            }

            // VTK: Normalize(sNext) == 0 -> "Coincident points!"，放弃该折线
            if (sNext.normalize() == 0.0) {
                reason = "has coincident points";
                break;
            }

            Vector3d s = (sPrev + sNext) / 2.0;  // 平均切向 -> 转角倒角
            if (s.normalize() == 0.0) {
                // VTK: 180° 回折时退回备用 bevel 向量 cross(sPrev, n)
                s = sPrev.cross(n);
                if (s.normalize() == 0.0) {
                    reason = "has an undefined bevel vector";
                    break;
                }
            }

            Vector3d w = s.cross(n);
            if (w.normalize() == 0.0) {
                // VTK: "Bad normal" -> 切线与该点法向平行，带面方向无法确定
                reason = "is parallel to its line normal";
                break;
            }

            Vector3d nP = w.cross(s);  // 正交框架第三轴
            nP.normalize();

            const Vector3d v = w * cosTheta + nP * sinTheta;
            minusPoints[j] = p - m_Width * v;
            plusPoints[j] = p + m_Width * v;
        }

        if (!reason.empty()) {
            addNote("RibbonFilter skipped " + std::string(LineTypeName(line.sourceType)) + " cell " +
                    std::to_string(line.sourceCellId) + " which " + reason + ".");
            continue;
        }

        // 输出点：偶数索引是减号侧，与 VTK 的 newPts->InsertPoint(ptId, sm) 顺序一致。
        const igIndex basePointId = static_cast<igIndex>(outputPoints->GetNumberOfPoints());
        for (size_t j = 0; j < numberOfLinePoints; ++j) {
            outputPoints->AddPoint(minusPoints[j]);
            sourcePointIds.push_back(ids[j]);
            outputPoints->AddPoint(plusPoints[j]);
            sourcePointIds.push_back(ids[j]);
        }

        // VTK 的 triangle strip [p-₀, p+₀, p-₁, p+₁, ...] 等价于逐段四边形；
        // 环状顶点顺序保证 iGame 的扇形三角化不会产生自交。
        for (size_t j = 0; j + 1 < numberOfLinePoints; ++j) {
            const igIndex quadIds[4] = {static_cast<igIndex>(basePointId + 2 * j),
                                        static_cast<igIndex>(basePointId + 2 * j + 1),
                                        static_cast<igIndex>(basePointId + 2 * j + 3),
                                        static_cast<igIndex>(basePointId + 2 * j + 2)};
            outputFaces->AddCellIds(quadIds, 4);
            sourceFaceIds.push_back(line.sourceCellId);
        }
    }

    if (outputPoints->GetNumberOfPoints() == 0 || outputFaces->GetNumberOfCells() == 0) {
        m_Message = withNotes("RibbonFilter skipped every line cell; no ribbon could be generated.");
        return false;
    }

    // ------------------------------------------------------------------
    // 4. 组装输出对象并重建独立的属性集。
    // ------------------------------------------------------------------
    auto output = SurfaceMesh::New();
    const std::string inputName = input->GetName();
    output->SetName(inputName.empty() ? std::string("Ribbon") : inputName + "_Ribbon");
    output->SetPoints(outputPoints);
    output->SetFaces(outputFaces);

    CopyAttributesBySourceId(input, output, outputPoints->GetNumberOfPoints(), sourcePointIds, IG_POINT);
    CopyAttributesBySourceId(input, output, outputFaces->GetNumberOfCells(), sourceFaceIds, IG_CELL);

    m_Message = notes;
    output->Modified();
    SetOutput(0, output);
    return true;
}

IGAME_NAMESPACE_END
