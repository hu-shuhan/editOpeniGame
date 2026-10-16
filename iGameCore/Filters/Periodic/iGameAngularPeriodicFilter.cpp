#include "Periodic/iGameAngularPeriodicFilter.h"

#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

// 一份旋转副本里的单元：普通单元 ids 为顶点号序列；
// IG_POLYHEDRON 单元 ids 为核心约定的编码序列：
//   [faceCount, face0_npts, face0顶点..., face1_npts, face1顶点..., ...]
struct CellRecord {
    std::vector<igIndex> ids;
    IGenum type{IG_EMPTY_CELL};
};

// 按单元类型搬运顶点偏移。
// 多面体编码里只有"顶点号"需要 +offset，faceCount/npts 等计数不能动。
void OffsetCellIds(const CellRecord& cell, igIndex offset, std::vector<igIndex>& shifted) {
    const auto& in = cell.ids;
    if (cell.type == IG_POLYHEDRON) {
        std::vector<igIndex> tmp;
        tmp.reserve(in.size());
        size_t r = 0;
        if (r >= in.size()) { shifted.clear(); return; }
        igIndex nFaces = in[r++];
        tmp.push_back(nFaces);
        for (igIndex f = 0; f < nFaces && r < in.size(); ++f) {
            igIndex nPts = in[r++];
            tmp.push_back(nPts);
            for (igIndex j = 0; j < nPts && r < in.size(); ++j) {
                tmp.push_back(in[r++] + offset);
            }
        }
        tmp.shrink_to_fit();
        shifted.swap(tmp);
    } else {
        shifted.resize(in.size());
        for (size_t i = 0; i < in.size(); ++i) { shifted[i] = in[i] + offset; }
    }
}

// 读出任意长度单元的点号序列（指针版，无固定缓冲越界风险）。
bool ReadCellRecord(CellArray* cellArray, IGsize cellId, CellRecord& record) {
    record.ids.clear();
    record.type = IG_EMPTY_CELL;
    if (cellArray == nullptr) return true;
    const igIndex* ids = nullptr;
    int n = cellArray->GetCellIds(cellId, ids);
    if (n > 0) { record.ids.assign(ids, ids + n); }
    return true;
}

// 按输入网格类型把拓扑归一化为统一 CellRecord 序列。
// 返回 false 表示遇到无法安全复制的拓扑（输入本身单元结构异常）。
bool CollectCells(PointSet* src, std::vector<CellRecord>& cells, std::string& message) {
    cells.clear();
    if (src == nullptr) return true;

    // ---------- 结构化网格：物化隐式拓扑 ----------
    if (auto structured = DynamicCast<StructuredMesh>(src)) {
        igIndex* dims = structured->GetDimensionSize();
        structured->GenStructuredCellConnectivities();
        if (dims[2] > 1) {
            CellArray* volumes = structured->GetVolumes();
            const IGsize n = volumes ? volumes->GetNumberOfCells() : 0;
            for (IGsize c = 0; c < n; ++c) {
                CellRecord record;
                ReadCellRecord(volumes, c, record);
                if (record.ids.size() != 8) {
                    message = "StructuredMesh: 3D 结构化单元不是 8 点六面体。";
                    return false;
                }
                record.type = IG_HEXAHEDRON;
                cells.push_back(std::move(record));
            }
        } else {
            CellArray* faces = structured->GetFaces();
            const IGsize n = faces ? faces->GetNumberOfCells() : 0;
            for (IGsize c = 0; c < n; ++c) {
                CellRecord record;
                ReadCellRecord(faces, c, record);
                if (record.ids.size() != 4) {
                    message = "StructuredMesh: 2D 结构化单元不是 4 点四边形。";
                    return false;
                }
                record.type = IG_QUAD;
                cells.push_back(std::move(record));
            }
        }
        return true;
    }

    // ---------- 体网格 ----------
    if (auto volumeMesh = DynamicCast<VolumeMesh>(src)) {
        if (volumeMesh->GetIsPolyhedronType()) {
            // 多面体按核心约定的面编码存储在 CellArray 中，委托官方转换以拿到规范编码
            UnstructuredMesh::Pointer converted = nullptr;
            if (!UnstructuredMesh::TransferVolumeMeshToUnstructuredMesh(volumeMesh, converted) ||
                converted == nullptr) {
                message = "VolumeMesh: 无法把多面体体网格转换为规范的 UnstructuredMesh。";
                return false;
            }
            CellArray* cellArray = converted->GetCells();
            const IGsize n = converted->GetNumberOfCells();
            for (IGsize c = 0; c < n; ++c) {
                CellRecord record;
                ReadCellRecord(cellArray, c, record);
                record.type = IG_POLYHEDRON;
                cells.push_back(std::move(record));
            }
            return true;
        }

        CellArray* volumes = volumeMesh->GetVolumes();
        const IGsize n = volumes ? volumes->GetNumberOfCells() : 0;
        for (IGsize c = 0; c < n; ++c) {
            CellRecord record;
            ReadCellRecord(volumes, c, record);
            IGenum type = VolumeMesh::GetVolumeTypeWithPointNum(static_cast<int>(record.ids.size()));
            if (type == IG_EMPTY_CELL) {
                message = "VolumeMesh: 不支持的体单元顶点数 " +
                          std::to_string(record.ids.size()) + "（需为 4/5/6/8）。";
                return false;
            }
            record.type = type;
            cells.push_back(std::move(record));
        }
        return true;
    }

    // ---------- 非结构化网格：类型权威在 m_Types ----------
    if (auto unstructured = DynamicCast<UnstructuredMesh>(src)) {
        CellArray* cellArray = unstructured->GetCells();
        const IGsize n = unstructured->GetNumberOfCells();
        for (IGsize c = 0; c < n; ++c) {
            CellRecord record;
            ReadCellRecord(cellArray, c, record);
            if (!record.ids.empty()) {
                record.type = unstructured->GetCellType(c);
                cells.push_back(std::move(record));
            }
        }
        return true;
    }

    // ---------- 曲面网格：按面点数推断类型（>4 点归为 IG_POLYGON） ----------
    if (auto surface = DynamicCast<SurfaceMesh>(src)) {
        CellArray* faces = surface->GetFaces();
        const IGsize n = faces ? faces->GetNumberOfCells() : 0;
        for (IGsize c = 0; c < n; ++c) {
            CellRecord record;
            ReadCellRecord(faces, c, record);
            if (record.ids.empty()) continue;
            IGenum type = SurfaceMesh::GetFaceTypeWithPointNum(static_cast<int>(record.ids.size()));
            if (type == IG_EMPTY_CELL) continue;
            record.type = type;
            cells.push_back(std::move(record));
        }
        return true;
    }

    // 其余（纯 PointSet 点云等）：无拓扑，只复制点
    return true;
}

// 同一份旋转副本的 3x3 旋转矩阵（行主序）。
using Rotation = std::array<double, 9>;

// 按 Rodrigues 公式构造绕单位轴 axis 旋转 angleRad 的旋转矩阵。
Rotation RotationMatrix(const Vector3d& axis, double angleRad) {
    const double c = std::cos(angleRad);
    const double s = std::sin(angleRad);
    const double t = 1.0 - c;
    const double x = axis[0], y = axis[1], z = axis[2];
    return Rotation{
        t * x * x + c,     t * x * y - s * z, t * x * z + s * y,
        t * x * y + s * z, t * y * y + c,     t * y * z - s * x,
        t * x * z - s * y, t * y * z + s * x, t * z * z + c,
    };
}

// 份数×角度是否恰好覆盖整周（360°）。
bool IsFullPeriod(int copies, float angleDeg) {
    return std::fabs(static_cast<double>(copies) * angleDeg - 360.0) < 1e-3;
}

// 生成覆盖信息：整周闭合 / 缺口 / 重叠，并列出各份角度。
std::string BuildCoverageInfo(int copies, float angleDeg) {
    const double total = static_cast<double>(copies) * angleDeg;
    const double diff = total - 360.0;
    std::ostringstream oss;
    if (std::fabs(diff) < 1e-3) {
        oss << "整周闭合：";
    } else if (diff < 0.0) {
        oss << "缺口 " << (-diff) << "°：";
    } else {
        oss << "重叠 " << diff << "°：";
    }
    for (int i = 0; i < copies; ++i) {
        if (i > 0) oss << ", ";
        oss << (i * angleDeg) << "°";
    }
    oss << "（覆盖 " << total << "°）";
    return oss.str();
}

// 属性按语义旋转的方式。
enum class AttrRotation {
    None,     // 原样复制（标量、纹理坐标、RGB、无符号整型等）
    Vector3,  // 3 分量向量：v' = R·v
    Tensor9,  // 9 分量张量：T' = R·T·Rᵀ
    Tensor6,  // 6 分量对称张量（Voigt: xx,yy,zz,xy,yz,xz）：T' = R·T·Rᵀ
};

AttrRotation ClassifyRotation(const AttributeSet::Attribute& attr) {
    if (!attr.pointer) return AttrRotation::None;
    const int dimension = attr.pointer->GetDimension();
    if ((attr.type == IG_VECTOR || attr.type == IG_NORMAL) && dimension == 3) {
        return AttrRotation::Vector3;
    }
    if (attr.type == IG_TENSOR) {
        if (dimension == 9) return AttrRotation::Tensor9;
        if (dimension == 6) return AttrRotation::Tensor6;
    }
    return AttrRotation::None;
}

template <typename T>
void RotateVector3(const double* R, const T* src, T* dst) {
    const double x = static_cast<double>(src[0]);
    const double y = static_cast<double>(src[1]);
    const double z = static_cast<double>(src[2]);
    dst[0] = static_cast<T>(R[0] * x + R[1] * y + R[2] * z);
    dst[1] = static_cast<T>(R[3] * x + R[4] * y + R[5] * z);
    dst[2] = static_cast<T>(R[6] * x + R[7] * y + R[8] * z);
}

// 计算 R·A·Rᵀ，A 为行主序 3x3。
inline void SimilarityTransform(const double* R, const double A[3][3], double out[3][3]) {
    double tmp[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            double s = 0.0;
            for (int k = 0; k < 3; ++k) s += R[i * 3 + k] * A[k][j];
            tmp[i][j] = s;
        }
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            double s = 0.0;
            for (int k = 0; k < 3; ++k) s += tmp[i][k] * R[j * 3 + k];
            out[i][j] = s;
        }
    }
}

template <typename T>
void RotateTensor9(const double* R, const T* src, T* dst) {
    double a[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) a[i][j] = static_cast<double>(src[i * 3 + j]);
    }
    double out[3][3];
    SimilarityTransform(R, a, out);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) dst[i * 3 + j] = static_cast<T>(out[i][j]);
    }
}

template <typename T>
void RotateTensor6(const double* R, const T* src, T* dst) {
    const double a[3][3] = {
        {static_cast<double>(src[0]), static_cast<double>(src[3]), static_cast<double>(src[5])},
        {static_cast<double>(src[3]), static_cast<double>(src[1]), static_cast<double>(src[4])},
        {static_cast<double>(src[5]), static_cast<double>(src[4]), static_cast<double>(src[2])},
    };
    double out[3][3];
    SimilarityTransform(R, a, out);
    dst[0] = static_cast<T>(out[0][0]);
    dst[1] = static_cast<T>(out[1][1]);
    dst[2] = static_cast<T>(out[2][2]);
    dst[3] = static_cast<T>(out[0][1]);
    dst[4] = static_cast<T>(out[1][2]);
    dst[5] = static_cast<T>(out[0][2]);
}

// 把源属性数组按 copies 份复制到输出，并按语义对每份做几何同步旋转。
template <typename TArray>
ArrayObject::Pointer CopyAttributeArrayN(typename TArray::Pointer input,
                                         int copies,
                                         AttrRotation rotation,
                                         const std::vector<Rotation>& rotations) {
    if (!input) return nullptr;
    auto output = TArray::New();
    output->SetName(input->GetName());
    output->SetDimension(input->GetDimension());

    const IGsize tuples = input->GetNumberOfElements();
    const IGsize values = input->GetNumberOfValues();
    const int dimension = input->GetDimension();
    output->Resize(tuples * copies);
    if (tuples == 0 || values == 0 || copies <= 0) return output;

    using Elem = std::remove_cv_t<std::remove_pointer_t<decltype(input->RawPointer())>>;
    const Elem* srcBase = input->RawPointer();
    Elem* dstBase = output->RawPointer();

    // 不旋转的数组：整段批量拷贝（避免逐分量循环）
    if (rotation == AttrRotation::None) {
        for (int c = 0; c < copies; ++c) {
            std::copy(srcBase, srcBase + values, dstBase + static_cast<IGsize>(c) * values);
        }
        return output;
    }

    for (int c = 0; c < copies; ++c) {
        const Rotation& R = rotations[static_cast<size_t>(c)];
        const IGsize base = static_cast<IGsize>(c) * tuples;
        for (IGsize i = 0; i < tuples; ++i) {
            const Elem* src = srcBase + i * dimension;
            Elem* dst = dstBase + (base + i) * dimension;
            switch (rotation) {
            case AttrRotation::Vector3:
                RotateVector3(R.data(), src, dst);
                break;
            case AttrRotation::Tensor9:
                RotateTensor9(R.data(), src, dst);
                break;
            case AttrRotation::Tensor6:
                RotateTensor6(R.data(), src, dst);
                break;
            case AttrRotation::None:
            default:
                for (int k = 0; k < dimension; ++k) dst[k] = src[k];
                break;
            }
        }
    }
    return output;
}

ArrayObject::Pointer CopyAttribute(const AttributeSet::Attribute& attr,
                                   int copies,
                                   const std::vector<Rotation>& rotations) {
    const AttrRotation rotation = ClassifyRotation(attr);

    // 浮点数组按语义旋转；整型（含有符号）不参与旋转，原样复制。
    auto copyRotatable = [&](auto array) -> ArrayObject::Pointer {
        using ArrayType = typename decltype(array)::ObjectType;
        return CopyAttributeArrayN<ArrayType>(array, copies, rotation, rotations);
    };
    auto copyPlain = [&](auto array) -> ArrayObject::Pointer {
        using ArrayType = typename decltype(array)::ObjectType;
        return CopyAttributeArrayN<ArrayType>(array, copies, AttrRotation::None, rotations);
    };

    if (auto array = DynamicCast<FloatArray>(attr.pointer)) return copyRotatable(array);
    if (auto array = DynamicCast<DoubleArray>(attr.pointer)) return copyRotatable(array);
    if (auto array = DynamicCast<IntArray>(attr.pointer)) return copyPlain(array);
    if (auto array = DynamicCast<ShortArray>(attr.pointer)) return copyPlain(array);
    if (auto array = DynamicCast<CharArray>(attr.pointer)) return copyPlain(array);
    if (auto array = DynamicCast<LongLongArray>(attr.pointer)) return copyPlain(array);

    if (auto array = DynamicCast<UnsignedIntArray>(attr.pointer)) return copyPlain(array);
    if (auto array = DynamicCast<UnsignedShortArray>(attr.pointer)) return copyPlain(array);
    if (auto array = DynamicCast<UnsignedCharArray>(attr.pointer)) return copyPlain(array);
    if (auto array = DynamicCast<UnsignedLongLongArray>(attr.pointer)) return copyPlain(array);

    return nullptr;
}

// 把源 PointData/CellData 全部属性复制到输出：标量逐份重复，
// 向量/张量按每份的旋转矩阵同步旋转。
bool CopyAttributesToOutput(PointSet* src,
                            UnstructuredMesh* output,
                            int copies,
                            const std::vector<Rotation>& rotations,
                            std::string& message) {
    auto outputAttributes = AttributeSet::New();

    auto srcAttributes = src->GetAttributeSet();
    if (srcAttributes) {
        auto all = srcAttributes->GetAllAttributes();
        const IGsize count = all->GetNumberOfElements();
        for (IGsize i = 0; i < count; ++i) {
            auto& attr = all->GetElement(i);
            if (attr.IsNone() || !attr.pointer) continue;

            auto copied = CopyAttribute(attr, copies, rotations);
            if (!copied) {
                message = "不支持复制该属性数组类型: " + attr.pointer->GetName();
                return false;
            }
            const IGsize index = outputAttributes->AddAttribute(attr.type, attr.attachmentType, copied);
            if (index == static_cast<IGsize>(-1)) {
                message = "添加输出属性失败: " + attr.pointer->GetName();
                return false;
            }
            outputAttributes->GetAttribute(index).UpdateAllDataRange();
        }
    }

    output->SetAttributeSet(outputAttributes);
    return true;
}

} // namespace

AngularPeriodicFilter::AngularPeriodicFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void AngularPeriodicFilter::SetRotationAxis(const Point& origin, const Vector3d& axis) {
    m_AxisOrigin = origin;
    m_AxisNormalized = axis;
    if (m_AxisNormalized.length() > 1e-12) {
        m_AxisNormalized.normalize();
    }
}

bool AngularPeriodicFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "no input mesh";
        return false;
    }
    if (m_AxisNormalized.length() < 1e-12) {
        m_Message = "rotation axis has zero length";
        return false;
    }
    if (m_Angle <= 0.f) {
        m_Message = "period angle must be positive";
        return false;
    }
    if (m_IterationMode == ITERATION_MODE_DIRECT_NB && m_NumberOfCopies < 1) {
        m_Message = "number of copies is invalid";
        return false;
    }

    auto mesh = DynamicCast<PointSet>(input);
    if (mesh == nullptr) {
        m_Message = "input is not a surface/unstructured mesh";
        return false;
    }

    Points* srcPoints = mesh->GetPoints();
    const IGsize numPoints = srcPoints ? srcPoints->GetNumberOfPoints() : 0;
    if (numPoints == 0) {
        m_Message = "input mesh has no points";
        return false;
    }

    // 归一化：一次采集源拓扑（含正确单元类型），避免每份复制时猜类型/丢单元
    std::vector<CellRecord> cells;
    if (!CollectCells(mesh.get(), cells, m_Message)) {
        return false;
    }

    // 计算实际份数：MAX 模式取不超过整周的最大整数份数 floor(360/angle)。
    if (m_IterationMode == ITERATION_MODE_MAX) {
        const double raw = 360.0 / static_cast<double>(m_Angle);
        m_EffectiveCopies = static_cast<int>(std::floor(raw));
        if (m_EffectiveCopies < 1) m_EffectiveCopies = 1;
    } else {
        m_EffectiveCopies = m_NumberOfCopies;
    }

    // 覆盖信息：整周闭合 / 缺口 / 重叠（默认只提示不拦截）。
    m_CoverageInfo = BuildCoverageInfo(m_EffectiveCopies, m_Angle);
    if (m_RequireFullPeriod && !IsFullPeriod(m_EffectiveCopies, m_Angle)) {
        m_Message = "not a full period: " + m_CoverageInfo;
        return false;
    }

    auto outputPoints = Points::New();
    auto outputCells = CellArray::New();
    auto outputTypes = UnsignedIntArray::New();

    // ParaView 语义：相邻两份间隔 m_Angle 度，第 i 份旋转 i×m_Angle。
    const double stepAngle = static_cast<double>(m_Angle) * 3.14159265358979 / 180.0;
    std::vector<Rotation> rotations(static_cast<size_t>(m_EffectiveCopies));
    for (int i = 0; i < m_EffectiveCopies; ++i) {
        rotations[static_cast<size_t>(i)] = RotationMatrix(m_AxisNormalized, stepAngle * i);
    }

    // 预分配输出，避免逐点/逐单元扩容（每个单元每份最多 2 个三角形）
    outputPoints->Reserve(numPoints * static_cast<IGsize>(m_EffectiveCopies));
    const IGsize maxCellsPerCopy = static_cast<IGsize>(cells.size()) * 2;
    outputCells->Reserve(maxCellsPerCopy * static_cast<IGsize>(m_EffectiveCopies));
    outputTypes->Reserve(maxCellsPerCopy * static_cast<IGsize>(m_EffectiveCopies));

    for (int i = 0; i < m_EffectiveCopies; ++i) {
        const Rotation& rotation = rotations[static_cast<size_t>(i)];
        const igIndex pointOffset = static_cast<igIndex>(numPoints * i);

        for (IGsize p = 0; p < numPoints; ++p) {
            outputPoints->AddPoint(RotatePoint(srcPoints->GetPoint(p), rotation));
        }

        std::vector<igIndex> shifted;
        for (const CellRecord& cell : cells) {
            OffsetCellIds(cell, pointOffset, shifted);
            if (shifted.empty()) continue;
            outputCells->AddCellIds(shifted.data(), static_cast<int>(shifted.size()));
            outputTypes->AddValue(static_cast<unsigned int>(cell.type));
        }
    }

    auto output = UnstructuredMesh::New();
    output->SetPoints(outputPoints);
    output->SetCells(outputCells, outputTypes);

    // PointData/CellData 逐份复制到输出，并按语义同步旋转向量/张量
    if (!CopyAttributesToOutput(mesh.get(), output.get(), m_EffectiveCopies, rotations, m_Message)) {
        return false;
    }

    SetOutput(output);
    return true;
}

Point AngularPeriodicFilter::RotatePoint(const Point& p, const RotMat& R) const {
    const double vx = static_cast<double>(p[0]) - m_AxisOrigin[0];
    const double vy = static_cast<double>(p[1]) - m_AxisOrigin[1];
    const double vz = static_cast<double>(p[2]) - m_AxisOrigin[2];

    return Point(static_cast<float>(m_AxisOrigin[0] + R[0] * vx + R[1] * vy + R[2] * vz),
                 static_cast<float>(m_AxisOrigin[1] + R[3] * vx + R[4] * vy + R[5] * vz),
                 static_cast<float>(m_AxisOrigin[2] + R[6] * vx + R[7] * vy + R[8] * vz));
}

IGAME_NAMESPACE_END
