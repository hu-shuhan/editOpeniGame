#include "iGameTextureMapToCylinderFilter.h"

// —— 特征值/矩阵运算（求点云主轴用；框架 ThirdParty 已带 Eigen，多个 filter 在用）——
#include "Eigen/Dense"
#include "Eigen/Eigenvalues"

// —— 各数据类型头文件 ——
#include "iGameAttributeSet.h"  // 属性集（点/单元数据数组）
#include "iGameCellArray.h"     // 单元数组（连接关系）
#include "iGameCellType.h"      // 单元类型枚举
#include "iGameFlatArray.h"     // FlatArray 模板（FloatArray / DoubleArray 等）
#include "iGamePointSet.h"      // Points（点坐标容器）
#include "iGameSurfaceMesh.h"   // 表面网格
#include "iGameVolumeMesh.h"    // 体网格

#include <cmath>
#include <exception>
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

constexpr double kEps = 1e-12;
constexpr double kPi = 3.14159265358979323846;

// ------------------------------------------------------------------
// 小工具：三维向量运算（避免为几个点乘点加引入额外依赖）
// ------------------------------------------------------------------
inline double Dot3(const double a[3], const double b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
inline void Cross3(const double a[3], const double b[3], double out[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}
inline double Norm3(const double a[3]) { return std::sqrt(Dot3(a, a)); }
inline bool Normalize3(double v[3]) {
    const double n = Norm3(v);
    if (n <= kEps) { return false; }
    v[0] /= n;
    v[1] /= n;
    v[2] /= n;
    return true;
}

// ------------------------------------------------------------------
// 深拷贝相关：输出节点必须与输入指针级独立（同批 filter 的统一约定）
// ------------------------------------------------------------------

/// 深拷贝一个数组（元素 + 名字 + 维度），返回同类型的新数组
template <typename T>
typename T::Pointer DeepCopyArray(typename T::Pointer src) {
    if (src == nullptr) { return nullptr; }
    auto dst = T::New();
    dst->DeepCopy(src);
    return dst;
}

/// 按数组实际类型深拷贝一个属性数组（覆盖框架全部 FlatArray 类型）
ArrayObject::Pointer CopyAttribute(ArrayObject::Pointer source) {
    if (source == nullptr) { return nullptr; }
    ArrayObject::Pointer copy;
    switch (source->GetArrayType()) {
#define COPY_ATTRIBUTE(Type) \
    case IG_##Type: copy = DeepCopyArray<Type>(DynamicCast<Type>(source)); break;
        COPY_ATTRIBUTE(FloatArray)
        COPY_ATTRIBUTE(DoubleArray)
        COPY_ATTRIBUTE(IntArray)
        COPY_ATTRIBUTE(UnsignedIntArray)
        COPY_ATTRIBUTE(CharArray)
        COPY_ATTRIBUTE(UnsignedCharArray)
        COPY_ATTRIBUTE(ShortArray)
        COPY_ATTRIBUTE(UnsignedShortArray)
        COPY_ATTRIBUTE(LongLongArray)
        COPY_ATTRIBUTE(UnsignedLongLongArray)
#undef COPY_ATTRIBUTE
        default: break;
    }
    if (copy == nullptr) {
        // 未识别的数组类型：兜底转成 double 保留数据，绝不静默丢弃属性
        const int dim = source->GetDimension();
        if (dim <= 0) { return nullptr; }
        auto fallback = DoubleArray::New();
        fallback->SetName(source->GetName());
        fallback->SetDimension(dim);
        const IGsize values = source->GetNumberOfValues();
        // 注意：FlatArray::Resize 收"元素个数"（内部再乘维度），必须传元素数
        fallback->Resize(source->GetNumberOfElements());
        for (IGsize i = 0; i < values; ++i) { fallback->SetValue(i, source->GetValue(i)); }
        copy = fallback;
    }
    return copy;
}

/// 深拷贝属性集；跳过旧的同名纹理坐标数组，保证重复执行不堆积同名数组。
///
/// expectedCellCount > 0 时把「单元属性」的长度对齐到输出单元数：
/// 表面网格输入可能带独立线段（输出单元数多于输入单元数），
/// 这些新增单元没有对应的原始数据，按 0 补齐。
/// 不对齐的话数组会比单元数短，后续按单元查询或着色会读到越界数据。
AttributeSet::Pointer DeepCopyAttributes(AttributeSet::Pointer src, const std::string& skipName,
                                         IGsize expectedCellCount = 0) {
    auto dst = AttributeSet::New();
    if (src == nullptr) { return dst; }
    auto all = src->GetAllAttributes();
    if (all == nullptr) { return dst; }
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        // 同时匹配"挂载类型"：GetAttributeIndex 只按名字匹配，会误伤同名的点/单元数组
        if (attr.attachmentType == IG_POINT && attr.pointer->GetName() == skipName) { continue; }
        auto copy = CopyAttribute(attr.pointer);
        if (copy == nullptr) { continue; }
        // 单元属性对齐到输出单元数：前面的面沿用原值，新追加的独立线段补 0
        // （FlatArray::Resize 走底层 vector::resize，新增元素会被值初始化为 0）
        if (attr.attachmentType == IG_CELL && expectedCellCount > 0 &&
            copy->GetNumberOfElements() < expectedCellCount) {
            copy->Resize(expectedCellCount);
        }
        DoubleArray::Pointer copyRange = nullptr;
        if (attr.dataRange != nullptr) {
            copyRange = DoubleArray::New();
            copyRange->DeepCopy(attr.dataRange);
        }
        if (copyRange != nullptr) {
            dst->AddAttribute(attr.type, attr.attachmentType, copy, copyRange);
        } else {
            dst->AddAttribute(attr.type, attr.attachmentType, copy);
        }
    }
    return dst;
}

/**
 * 深拷贝单元连接表（逐单元 AddCellIds 重建）。
 * 不用 CellArray::DeepCopy：它对"变长单元"（m_UseOffsets == true）会把 m_Offsets
 * 追加而非覆盖，偏移表错位、GetCellSize 会算错。
 * 缓冲用 vector 而不是固定数组：CellArray::GetCellIds 不做边界检查，
 * 多面体的展开连接表可能远超 IGAME_CELL_MAX_SIZE(256)，固定数组会越界写内存。
 */
CellArray::Pointer DeepCopyCellArray(CellArray::Pointer src) {
    if (src == nullptr) { return nullptr; }
    auto dst = CellArray::New();
    const IGsize n = src->GetNumberOfCells();
    std::vector<igIndex> ids(IGAME_CELL_MAX_SIZE, 0);
    for (IGsize i = 0; i < n; ++i) {
        const IGuint needed = src->GetCellSize(i);
        if (ids.size() < needed) { ids.resize(needed); }
        const int vcnt = src->GetCellIds(i, ids.data());
        if (vcnt <= 0) { continue; }
        dst->AddCellIds(ids.data(), vcnt);
    }
    return dst;
}

/// 收集一个单元表里所有单元的"无向边"，键为 (较小点索引 << 32) | 较大点索引
void CollectCellEdges(CellArray::Pointer cells, std::unordered_set<unsigned long long>& out) {
    if (cells == nullptr) { return; }
    const IGsize n = cells->GetNumberOfCells();
    std::vector<igIndex> ids(IGAME_CELL_MAX_SIZE, 0);
    for (IGsize i = 0; i < n; ++i) {
        const IGuint needed = cells->GetCellSize(i);
        if (needed < 2) { continue; }  // 点单元没有边
        if (ids.size() < needed) { ids.resize(needed); }
        const int vcnt = cells->GetCellIds(i, ids.data());
        if (vcnt < 2) { continue; }
        for (int k = 0; k < vcnt; ++k) {
            const igIndex a = ids[k];
            const igIndex b = ids[(k + 1) % vcnt];
            const igIndex lo = (a < b) ? a : b;
            const igIndex hi = (a < b) ? b : a;
            out.insert((static_cast<unsigned long long>(lo) << 32) |
                       static_cast<unsigned long long>(hi));
        }
    }
}

/**
 * 由「面表 + 边表」重建无结构网格的单元表与单元类型表，保留独立线段。
 *
 * 为什么需要它：读 .vtk 时 POLYGONS 落在表面网格的面表、LINES 落在它的边表，
 * 而通用的「转无结构网格」只搬面表 —— 那些不属于任何面、作为独立线段存在的几何
 * 会被整段丢掉。本过滤器只是在点数据上追加一组纹理坐标，不应当改变输入的几何，
 * 所以这里把独立线段识别出来一并带走。
 *
 * 独立线段的判定：先收集面单元的全部无向边，边表里「不在其中」的才算独立线段。
 * 这样即使边表里混着由面派生出来的边（部分流程会调用 BuildEdges 填充边表），
 * 也不会被当成独立几何重复输出。
 *
 * @param faces      面单元表（可为空）
 * @param edges      边表（可为空，表面网格独有）
 * @param outCells   [出] 新建的单元表
 * @param outTypes   [出] 新建的单元类型表，与 outCells 一一对应
 * @param looseEdges [出] 保留下来的独立线段条数
 */
void BuildCellsWithLooseEdges(CellArray::Pointer faces, CellArray::Pointer edges,
                              CellArray::Pointer& outCells, UnsignedIntArray::Pointer& outTypes,
                              IGsize& looseEdges) {
    outCells = CellArray::New();
    outTypes = UnsignedIntArray::New();
    looseEdges = 0;

    std::vector<igIndex> ids(IGAME_CELL_MAX_SIZE, 0);
    std::vector<unsigned int> types;

    // 1) 面：类型按顶点数判定（3 三角形 / 4 四边形 / 其余多边形）
    if (faces != nullptr) {
        const IGsize n = faces->GetNumberOfCells();
        for (IGsize i = 0; i < n; ++i) {
            const IGuint needed = faces->GetCellSize(i);
            if (ids.size() < needed) { ids.resize(needed); }
            const int vcnt = faces->GetCellIds(i, ids.data());
            if (vcnt <= 0) { continue; }
            outCells->AddCellIds(ids.data(), vcnt);
            types.push_back(static_cast<unsigned int>(
                vcnt == 3 ? IG_TRIANGLE : (vcnt == 4 ? IG_QUAD : IG_POLYGON)));
        }
    }

    // 2) 边：只留下不属于任何面的那些，作为独立线段单元追加在面之后
    if (edges != nullptr && edges->GetNumberOfCells() > 0) {
        std::unordered_set<unsigned long long> faceEdges;
        CollectCellEdges(faces, faceEdges);
        const IGsize n = edges->GetNumberOfCells();
        for (IGsize i = 0; i < n; ++i) {
            if (edges->GetCellSize(i) != 2) { continue; }  // 独立线段按两点一条计
            const int vcnt = edges->GetCellIds(i, ids.data());
            if (vcnt != 2) { continue; }
            const igIndex lo = (ids[0] < ids[1]) ? ids[0] : ids[1];
            const igIndex hi = (ids[0] < ids[1]) ? ids[1] : ids[0];
            const unsigned long long key = (static_cast<unsigned long long>(lo) << 32) |
                                           static_cast<unsigned long long>(hi);
            if (faceEdges.count(key) != 0) { continue; }  // 面的派生边，不是独立几何
            outCells->AddCellIds(ids.data(), 2);
            types.push_back(static_cast<unsigned int>(IG_LINE));
            ++looseEdges;
        }
    }

    outTypes->Resize(static_cast<IGsize>(types.size()));
    for (size_t i = 0; i < types.size(); ++i) {
        outTypes->SetValue(static_cast<IGsize>(i), types[i]);
    }
}

/**
 * 自动求轴：对点云做**主成分分析**，取协方差矩阵最大特征值方向为主轴，
 * 轴的两个端点为点云沿该轴投影的极小/极大位置。
 *
 * 轴对齐网格与圆柱/管类模型能稳定给出轴向；各向同性模型（立方体/球体）
 * 主轴不唯一，此时应关闭自动、手动给出轴两端点。
 */
bool ResolveAutoAxis(Points::Pointer points, double point1[3], double point2[3]) {
    if (points == nullptr) { return false; }
    const IGsize n = points->GetNumberOfPoints();
    if (n <= 0) { return false; }

    Eigen::Vector3d mean = Eigen::Vector3d::Zero();
    for (IGsize i = 0; i < n; ++i) {
        auto p = points->GetPoint(i);
        mean += Eigen::Vector3d(static_cast<double>(p[0]), static_cast<double>(p[1]),
                                static_cast<double>(p[2]));
    }
    mean /= static_cast<double>(n);

    Eigen::Matrix3d cov = Eigen::Matrix3d::Zero();
    for (IGsize i = 0; i < n; ++i) {
        auto p = points->GetPoint(i);
        const Eigen::Vector3d d =
            Eigen::Vector3d(static_cast<double>(p[0]), static_cast<double>(p[1]),
                            static_cast<double>(p[2])) -
            mean;
        cov += d * d.transpose();
    }
    cov /= static_cast<double>(n);

    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(cov);
    if (solver.info() != Eigen::Success) { return false; }
    Eigen::Vector3d axis = solver.eigenvectors().col(2);  // 最大特征值对应的方向
    if (axis.norm() <= kEps) { return false; }
    axis.normalize();

    double tmin = std::numeric_limits<double>::max();
    double tmax = -tmin;
    for (IGsize i = 0; i < n; ++i) {
        auto p = points->GetPoint(i);
        const double t =
            (Eigen::Vector3d(static_cast<double>(p[0]), static_cast<double>(p[1]),
                             static_cast<double>(p[2])) -
             mean)
                .dot(axis);
        if (t < tmin) { tmin = t; }
        if (t > tmax) { tmax = t; }
    }
    if (tmax - tmin <= kEps) { return false; }  // 点云在主轴上没有延展 → 轴无意义

    const Eigen::Vector3d c1 = mean + tmin * axis;
    const Eigen::Vector3d c2 = mean + tmax * axis;
    for (int i = 0; i < 3; ++i) {
        point1[i] = c1[i];
        point2[i] = c2[i];
    }
    return true;
}

}  // namespace

// ------------------------------------------------------------------
// 构造 / 参数
// ------------------------------------------------------------------
TextureMapToCylinderFilter::TextureMapToCylinderFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void TextureMapToCylinderFilter::SetPoint1(double x, double y, double z) {
    m_Point1[0] = x;
    m_Point1[1] = y;
    m_Point1[2] = z;
}

void TextureMapToCylinderFilter::SetPoint2(double x, double y, double z) {
    m_Point2[0] = x;
    m_Point2[1] = y;
    m_Point2[2] = z;
}

void TextureMapToCylinderFilter::GetResolvedAxis(double point1[3], double point2[3]) const {
    for (int i = 0; i < 3; ++i) {
        point1[i] = m_Resolved1[i];
        point2[i] = m_Resolved2[i];
    }
}

// ------------------------------------------------------------------
// Execute：执行（统一异常处理，失败不留残留输出）
// ------------------------------------------------------------------
bool TextureMapToCylinderFilter::Execute() {
    UpdateProgress(0);
    m_Message.clear();
    SetOutput(0, nullptr);

    try {
        return ExecuteInternal();
    } catch (const std::exception& e) {
        SetOutput(0, nullptr);
        m_Message = std::string("TextureMapToCylinderFilter exception: ") + e.what();
        IGAME_CORE_ERROR("{}", m_Message);
        return false;
    } catch (...) {
        SetOutput(0, nullptr);
        m_Message = "TextureMapToCylinderFilter unknown exception";
        IGAME_CORE_ERROR("{}", m_Message);
        return false;
    }
}

bool TextureMapToCylinderFilter::ExecuteInternal() {
    if (m_Inputs->GetNumberOfElements() == 0) {
        m_Message = "no input data";
        return false;
    }
    auto input = m_Inputs->GetElement(0);
    if (input == nullptr) {
        m_Message = "input data is null";
        return false;
    }

    // ---- 0) 输入统一成「点 + 单元表 + 类型表」，几何原样带到输出 ----
    //
    // 这里没有直接调用通用的「转无结构网格」：它对表面网格只搬面表
    // （SurfaceMesh::GetFaces），而读 .vtk 时 LINES 段是存放在边表
    // （SurfaceMesh::GetEdges）里的 —— 走通用转换会把输入里的独立线段整段丢掉。
    // 本过滤器只是在点数据上追加纹理坐标，不该删除任何原有几何，
    // 因此表面网格单独走「面 + 独立线段」的转换路径。
    Points::Pointer inPoints = nullptr;
    CellArray::Pointer outCells = nullptr;
    UnsignedIntArray::Pointer outTypes = nullptr;
    IGsize looseEdgeCount = 0;

    const IGenum inType = input->GetDataObjectType();
    if (inType == IG_SURFACE_MESH) {
        auto sm = DynamicCast<SurfaceMesh>(input);
        if (sm == nullptr) {
            m_Message = "input data claims to be a surface mesh but cannot be cast";
            return false;
        }
        inPoints = sm->GetPoints();
        BuildCellsWithLooseEdges(sm->GetFaces(), sm->GetEdges(), outCells, outTypes, looseEdgeCount);
    } else {
        // 无结构网格直接沿用；体网格等交给框架转换（它们不存在「面/边分离」的问题）
        UnstructuredMesh::Pointer um =
            (inType == IG_UNSTRUCTURED_MESH)
                ? DynamicCast<UnstructuredMesh>(input)
                : UnstructuredMesh::TransDataObjToUnstructuredMesh(input);
        if (um == nullptr) {
            m_Message = "unsupported data type (support: UnstructuredMesh / SurfaceMesh / VolumeMesh)";
            IGAME_CORE_ERROR("TextureMapToCylinderFilter: unsupported data type {}",
                             static_cast<int>(inType));
            return false;
        }
        inPoints = um->GetPoints();
        outCells = DeepCopyCellArray(um->GetCells());
        outTypes = DeepCopyArray<UnsignedIntArray>(um->GetCellTypes());
    }

    if (inPoints == nullptr) {
        m_Message = "input mesh has no point coordinates";
        return false;
    }
    const IGsize numPoints = inPoints->GetNumberOfPoints();
    if (numPoints == 0) {
        // 没有点就无从生成纹理坐标：明确失败，绝不把原模型当结果返回
        m_Message = "input mesh has no points, there is no texture coordinate to generate";
        return false;
    }

    // ---- 1) 确定圆柱轴 ----
    for (int i = 0; i < 3; ++i) {
        m_Resolved1[i] = m_Point1[i];
        m_Resolved2[i] = m_Point2[i];
    }
    if (m_Automatic) {
        if (!ResolveAutoAxis(inPoints, m_Resolved1, m_Resolved2)) {
            m_Message = "cannot determine a cylinder axis automatically (degenerate point cloud); "
                        "turn off automatic cylinder generation and specify Point1/Point2";
            return false;
        }
    }
    double axis[3] = {m_Resolved2[0] - m_Resolved1[0], m_Resolved2[1] - m_Resolved1[1],
                      m_Resolved2[2] - m_Resolved1[2]};
    const double axisLen = Norm3(axis);
    if (axisLen <= kEps) {
        m_Message = "degenerate cylinder axis (Point1 and Point2 coincide)";
        return false;
    }
    double axisUnit[3] = {axis[0] / axisLen, axis[1] / axisLen, axis[2] / axisLen};

    // 参考方向 ref：与轴垂直，作为角度的 0 度基准（由 axisUnit × (1,0,0) 构造）
    //
    // 这里必须用**单位化后的轴**做叉乘：用未归一化的 axis 时，两次叉乘的结果长度会按
    // |axis|² 缩小 —— 轴长 1e-6 时 ref 的长度只剩 1e-12，会被当成"轴退化"误判失败。
    // 而轴长只决定 t 的量程，本来就不该影响能不能建立参考方向。
    // 换成 axisUnit 后 ref 的方向与原来完全一致（都是归一化后的同向结果），
    // 只是不再随轴长消失，短轴、长轴都能正常求纹理坐标。
    double probe[3] = {1.0, 0.0, 0.0};
    double tmp[3];
    Cross3(axisUnit, probe, tmp);
    if (Norm3(tmp) <= kEps) {  // 轴与 X 平行时换 Y 做基准
        probe[0] = 0.0;
        probe[1] = 1.0;
        probe[2] = 0.0;
        Cross3(axisUnit, probe, tmp);
    }
    double ref[3];
    Cross3(tmp, axisUnit, ref);
    if (!Normalize3(ref)) {
        m_Message = "degenerate cylinder axis (cannot build a reference direction)";
        return false;
    }

    // ---- 2) 独立输出节点：点/单元/类型/属性全部深拷贝 ----
    auto outMesh = UnstructuredMesh::New();
    outMesh->SetName(input->GetName() + "_TCoords");

    auto outPoints = Points::New();
    outPoints->DeepCopy(inPoints);
    outMesh->SetPoints(outPoints);

    if (outCells != nullptr) { outMesh->SetCells(outCells, outTypes); }

    // 属性按输出单元数对齐后再挂上（点属性不受影响：点数没有变化）
    const IGsize outCellCount = (outCells != nullptr) ? outCells->GetNumberOfCells() : 0;
    outMesh->SetAttributeSet(
        DeepCopyAttributes(input->GetAttributeSet(), m_ArrayName, outCellCount));

    // ---- 3) 逐点计算 (s, t) ----
    // 用 FloatArray / 2 分量 / 名字 "Texture Coordinates"
    auto tcoords = FloatArray::New();
    tcoords->SetName(m_ArrayName);
    tcoords->SetDimension(2);
    // Resize 收"元素个数"（内部再乘维度 2）
    tcoords->Resize(numPoints);

    auto points = inPoints;
    for (IGsize i = 0; i < numPoints; ++i) {
        auto p = points->GetPoint(i);
        const double x[3] = {static_cast<double>(p[0]), static_cast<double>(p[1]),
                             static_cast<double>(p[2])};
        const double rel[3] = {x[0] - m_Resolved1[0], x[1] - m_Resolved1[1],
                               x[2] - m_Resolved1[2]};

        // t：点沿轴的归一化参数（Point1 处 = 0，Point2 处 = 1；轴长参与分母）
        const double proj = Dot3(rel, axisUnit);
        const double t = proj / axisLen;

        // 点到轴的径向单位向量（归一化失败说明点在轴上，保持零向量）
        double radial[3] = {rel[0] - proj * axisUnit[0], rel[1] - proj * axisUnit[1],
                            rel[2] - proj * axisUnit[2]};
        if (Norm3(radial) > kEps) {
            Normalize3(radial);
        } else {
            radial[0] = radial[1] = radial[2] = 0.0;
        }

        double cosTheta = Dot3(radial, ref);
        if (cosTheta > 1.0) { cosTheta = 1.0; }
        if (cosTheta < -1.0) { cosTheta = -1.0; }
        const double thetaX = std::acos(cosTheta);

        // 方向符号：axis · (ref × radial)，只关心正负，故单位化的轴不影响结果
        double crossRD[3];
        Cross3(ref, radial, crossRD);
        const double thetaY = Dot3(axisUnit, crossRD);

        double s = 0.0;
        if (m_PreventSeam) {
            s = thetaX / kPi;  // 0→1 再 1→0：避免接缝处跳变
        } else {
            s = thetaX / (2.0 * kPi);
            if (thetaY < 0.0) { s = 1.0 - s; }
        }
        tcoords->SetValue(i * 2 + 0, s);
        tcoords->SetValue(i * 2 + 1, t);

        if (numPoints > 0 && (i % 10000) == 0) {
            UpdateProgress(static_cast<double>(i) / static_cast<double>(numPoints) * 0.9);
        }
    }

    // 纹理坐标作为 Point Data 的 IG_TCOORD 数组写入
    outMesh->GetAttributeSet()->AddAttribute(IG_TCOORD, IG_POINT, tcoords);

    UpdateProgress(1);
    SetOutput(0, outMesh);
    return true;
}

IGAME_NAMESPACE_END
