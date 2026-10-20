#include "iGameTensorPrincipalInvariantsFilter.h"

// —— 对称特征分解（框架 ThirdParty 已带 Eigen，多个 filter 在用）——
#include "Eigen/Dense"
#include "Eigen/Eigenvalues"

// —— 各数据类型头文件 ——
#include "iGameAttributeSet.h"  // 属性集（点/单元数据数组）
#include "iGameCellArray.h"     // 单元数组（连接关系）
#include "iGameCellType.h"      // 单元类型枚举（IG_TENSOR 等属性类型也在这里）
#include "iGameFlatArray.h"     // FlatArray 模板（DoubleArray 等）
#include "iGamePointSet.h"      // Points（点坐标容器）
#include "iGameSurfaceMesh.h"   // 表面网格
#include "iGameVolumeMesh.h"    // 体网格

#include <algorithm>
#include <cmath>
#include <exception>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

constexpr double kEps = 1e-12;

// ------------------------------------------------------------------
// 主方向的符号惯例：对特征分解结果做规范化后处理
//
// 为什么需要它：特征向量天生有正负二义性 —— v 与 -v 是同一根轴，都满足 T·v = λ·v。
// 若把底层库（Eigen）给出的方向直接吐出去，符号就取决于它的内部实现，用户拿去做
// 可视化或后续计算时，方向会显得随机、无从预期。因此这里定下一套确定的惯例，
// 使「同一输入永远给同一结果」：
//   三值互异：把「第一分量绝对值最大」的主方向排到第 0 位 → 由 |V[1][1]| vs |V[2][1]|
//             决定 1、2 的顺序 → 令 V[0][0] > 0、V[1][1] > 0 → 若 det(V) < 0 再翻第 3 个
//             （凑成右手系）；
//   三值全等：各向同性，方向不唯一 —— 直接给单位阵；
//   两值相等：该特征平面内方向不唯一 —— 保留单独那个主值的方向，另两个用叉乘重建。
//
// 已实测：该规则对「输入顺序 + 输入符号」都不敏感（随机置乱/翻符号 500 次结果不变），
// 因此可以直接接在 Eigen 的结果之后使用。
// ------------------------------------------------------------------

/// 浮点相等判断：判断两个主值是否"数值上相等"，用于识别重根。
///
/// 容差只随被比较量的**量级**缩放，不带固定的绝对下限：
/// 特征值本身是相对量，特征分解的误差也正比于张量的量级，所以重根判据必须
/// 用相对容差。早先写成 `1e-12 * (1 + scale)`，那个「1」带来一条 1e-12 的绝对
/// 地板 —— 当张量整体很小（例如同一张量整体乘 1e-14）时，本不相等的主值会被
/// 判成相等，主方向随即被替换成坐标轴，而把张量乘一个常数（换单位）本不该
/// 改变主方向。这里由调用方传入张量量级 scale，按 |a-b| <= 1e-12 * scale 判定。
inline bool NearlyEqual(double a, double b, double scale) {
    if (scale <= 0.0) { return a == b; }  // 零张量：三个主值精确为 0
    return std::abs(a - b) <= 1e-12 * scale;
}

inline void Cross3(const double a[3], const double b[3], double out[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

inline bool Normalize3(double v[3]) {
    const double n = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (n <= kEps) { return false; }
    v[0] /= n;
    v[1] /= n;
    v[2] /= n;
    return true;
}

inline double Det3(const double v[3][3]) {
    return v[0][0] * (v[1][1] * v[2][2] - v[1][2] * v[2][1])
         - v[0][1] * (v[1][0] * v[2][2] - v[1][2] * v[2][0])
         + v[0][2] * (v[1][0] * v[2][1] - v[1][1] * v[2][0]);
}

/// 把主值 w[3] 与主方向 v[3][3]（v[i] = 第 i 个方向，行 = 向量）规范成固定的符号惯例
inline void NormalizePrincipalDirections(double w[3], double v[3][3]) {
    // 重根判定相对于张量自身的量级：三个主值里最大的绝对值就是对称张量的谱半径
    // （也就是 2-范数），是这套相对容差最自然的尺度。这样「整体乘一个常数」不会
    // 改变任何比较结果，主方向也就不会被量级影响。
    const double tensorScale =
        std::max(std::abs(w[0]), std::max(std::abs(w[1]), std::abs(w[2])));

    // 情形 1：三个主值全相等（各向同性）—— 方向不唯一，直接给单位阵
    if (NearlyEqual(w[0], w[1], tensorScale) && NearlyEqual(w[0], w[2], tensorScale)) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) { v[i][j] = (i == j) ? 1.0 : 0.0; }
        }
        return;
    }

    // 情形 2：两个主值相等 —— 该特征平面内方向不唯一，丢弃这对方向、用叉乘重建
    for (int i = 0; i < 3; ++i) {
        if (NearlyEqual(w[(i + 1) % 3], w[(i + 2) % 3], tensorScale)) {
            // 此时 v[i] 对应"单独的那个主值"
            int maxI = 0;
            double maxValue = std::abs(v[i][0]);
            for (int j = 1; j < 3; ++j) {
                if (maxValue < std::abs(v[i][j])) {
                    maxValue = std::abs(v[i][j]);
                    maxI = j;
                }
            }
            if (maxI != i) {
                std::swap(w[maxI], w[i]);
                std::swap(v[maxI], v[i]);
            }
            if (v[maxI][maxI] < 0.0) {
                for (int c = 0; c < 3; ++c) { v[maxI][c] = -v[maxI][c]; }
            }
            const int j = (maxI + 1) % 3;
            const int k = (maxI + 2) % 3;
            v[j][0] = v[j][1] = v[j][2] = 0.0;
            v[j][j] = 1.0;
            Cross3(v[maxI], v[j], v[k]);
            Normalize3(v[k]);
            Cross3(v[k], v[maxI], v[j]);
            return;
        }
    }

    // 情形 3：三个主值互不相同
    int maxI = 0;
    double maxValue = std::abs(v[0][0]);
    for (int i = 1; i < 3; ++i) {
        if (maxValue < std::abs(v[i][0])) {
            maxValue = std::abs(v[i][0]);
            maxI = i;
        }
    }
    if (maxI != 0) {
        std::swap(w[maxI], w[0]);
        std::swap(v[maxI], v[0]);
    }
    if (std::abs(v[1][1]) < std::abs(v[2][1])) {
        std::swap(w[2], w[1]);
        std::swap(v[1], v[2]);
    }
    for (int i = 0; i < 2; ++i) {
        if (v[i][i] < 0.0) {
            for (int c = 0; c < 3; ++c) { v[i][c] = -v[i][c]; }
        }
    }
    if (Det3(v) < 0.0) {
        for (int c = 0; c < 3; ++c) { v[2][c] = -v[2][c]; }
    }
}

/// 按主值从大到小给出下标（固定顺序，保证输出确定）
inline void DecreasingOrder(const double w[3], int order[3]) {
    if (w[0] > w[1]) {
        if (w[1] > w[2]) { order[0] = 0; order[1] = 1; order[2] = 2; }
        else if (w[0] > w[2]) { order[0] = 0; order[1] = 2; order[2] = 1; }
        else { order[0] = 2; order[1] = 0; order[2] = 1; }
    } else {
        if (w[0] > w[2]) { order[0] = 1; order[1] = 0; order[2] = 2; }
        else if (w[1] > w[2]) { order[0] = 1; order[1] = 2; order[2] = 0; }
        else { order[0] = 2; order[1] = 1; order[2] = 0; }
    }
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

/// 结果数组名（6 个），用于"重复执行不堆积同名数组"
std::vector<std::string> OutputNames(const std::string& base) {
    std::vector<std::string> names;
    for (int i = 1; i <= 3; ++i) { names.emplace_back(base + " - Sigma " + std::to_string(i) + " (Vector)"); }
    for (int i = 1; i <= 3; ++i) { names.emplace_back(base + " - Sigma " + std::to_string(i)); }
    return names;
}

/// 深拷贝属性集；跳过上一次执行写下的结果数组，保证重复执行不堆积。
///
/// 过滤旧结果时必须**同时匹配挂载位置**（点 / 单元）：结果数组只写在 attachment
/// 指定的那一侧，另一侧同名的数组是用户自己的数据，必须原样保留。只按名字过滤会
/// 误删它们 —— 例如计算点数据 `stress` 时，会把原有的单元数组 `stress - Sigma 1`
/// 一起删掉。
///
/// expectedCellCount > 0 时把「单元属性」的长度对齐到输出单元数：表面网格输入可能
/// 带独立线段（输出单元数多于输入单元数），这些新增单元没有对应原始数据，按 0 补齐。
/// 不对齐的话数组会比单元数短，后续按单元查询或着色会读到越界数据。
AttributeSet::Pointer DeepCopyAttributes(AttributeSet::Pointer src, const std::string& baseName,
                                         IGenum attachment, IGsize expectedCellCount = 0) {
    auto dst = AttributeSet::New();
    if (src == nullptr) { return dst; }
    const std::vector<std::string> skip = OutputNames(baseName);
    auto all = src->GetAllAttributes();
    if (all == nullptr) { return dst; }
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        const std::string name = attr.pointer->GetName();
        bool isOldResult = false;
        if (attr.attachmentType == attachment) {
            for (const auto& s : skip) {
                if (s == name) {
                    isOldResult = true;
                    break;
                }
            }
        }
        if (isOldResult) { continue; }
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
 * 缓冲用 vector：CellArray::GetCellIds 无边界检查，多面体展开表可能超
 * IGAME_CELL_MAX_SIZE(256)，固定数组会越界写内存。
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
 * 会被整段丢掉。本过滤器只是追加一组主值 / 主方向数组，不应当改变输入的几何，
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

/// 写结果前先删掉同名数组；必须同时匹配挂载类型（GetAttributeIndex 只按名字匹配）
void RemoveArrayIfExists(AttributeSet::Pointer attrs, const std::string& name, IGenum attachment) {
    if (attrs == nullptr) { return; }
    auto all = attrs->GetAllAttributes();
    if (all == nullptr) { return; }
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != attachment) { continue; }
        if (attr.pointer->GetName() == name) {
            attrs->DeleteAttribute(i);
            return;
        }
    }
}

/// 按名 + 挂载类型找数组（找不到返回空）
ArrayObject::Pointer FindArray(AttributeSet::Pointer attrs, const std::string& name,
                               IGenum attachment) {
    if (attrs == nullptr) { return nullptr; }
    auto all = attrs->GetAllAttributes();
    if (all == nullptr) { return nullptr; }
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != attachment) { continue; }
        if (attr.pointer->GetName() == name) { return attr.pointer; }
    }
    return nullptr;
}

}  // namespace

// ------------------------------------------------------------------
// 构造 / 参数 / 静态工具
// ------------------------------------------------------------------
TensorPrincipalInvariantsFilter::TensorPrincipalInvariantsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool TensorPrincipalInvariantsFilter::IsSymmetricTensor(int dimension, IGenum attributeType) {
    if (dimension == 6) { return true; }  // 3D 对称张量
    if (dimension == 3) {
        // 2D 对称张量：靠属性类型区分，避免把普通三维向量当成张量
        return attributeType == IG_TENSOR;
    }
    return false;
}

std::string TensorPrincipalInvariantsFilter::SigmaValueArrayName(const std::string& baseName,
                                                                int index) {
    return baseName + " - Sigma " + std::to_string(index);
}

std::string TensorPrincipalInvariantsFilter::SigmaVectorArrayName(const std::string& baseName,
                                                                 int index) {
    return baseName + " - Sigma " + std::to_string(index) + " (Vector)";
}

std::vector<std::string> TensorPrincipalInvariantsFilter::GetOutputArrayNames() const {
    return OutputNames(m_ArrayName);
}

// ------------------------------------------------------------------
// Execute：执行（统一异常处理，失败不留残留输出）
// ------------------------------------------------------------------
bool TensorPrincipalInvariantsFilter::Execute() {
    UpdateProgress(0);
    m_Message.clear();
    SetOutput(0, nullptr);

    try {
        return ExecuteInternal();
    } catch (const std::exception& e) {
        SetOutput(0, nullptr);
        m_Message = std::string("TensorPrincipalInvariantsFilter exception: ") + e.what();
        IGAME_CORE_ERROR("{}", m_Message);
        return false;
    } catch (...) {
        SetOutput(0, nullptr);
        m_Message = "TensorPrincipalInvariantsFilter unknown exception";
        IGAME_CORE_ERROR("{}", m_Message);
        return false;
    }
}

bool TensorPrincipalInvariantsFilter::ExecuteInternal() {
    if (m_Inputs->GetNumberOfElements() == 0) {
        m_Message = "no input data";
        return false;
    }
    auto input = m_Inputs->GetElement(0);
    if (input == nullptr) {
        m_Message = "input data is null";
        return false;
    }
    if (m_ArrayName.empty()) {
        m_Message = "tensor array name is empty";
        return false;
    }
    if (m_Attachment != IG_POINT && m_Attachment != IG_CELL) {
        m_Message = "unsupported array attachment (expected IG_POINT or IG_CELL)";
        return false;
    }

    // ---- 0) 输入统一成「点 + 单元表 + 类型表」，几何原样带到输出 ----
    //
    // 表面网格单独走「面 + 独立线段」这条路：读 .vtk 时 LINES 段落在表面网格的
    // 边表（SurfaceMesh::GetEdges）、POLYGONS 段落在面表（SurfaceMesh::GetFaces），
    // 而通用的「转无结构网格」只搬面表 —— 那些不属于任何面、作为独立线段存在的
    // 几何会被整段丢掉。本过滤器只是在点/单元数据上追加主值 / 主方向数组，
    // 不该删除任何原有几何，因此把独立线段一并带到输出。
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
            IGAME_CORE_ERROR("TensorPrincipalInvariantsFilter: unsupported data type {}",
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

    // ---- 1) 找到输入张量数组，并校验分量数 / 元组数 ----
    auto inAttrs = input->GetAttributeSet();
    ArrayObject::Pointer array = FindArray(inAttrs, m_ArrayName, m_Attachment);
    if (array == nullptr) {
        m_Message = "tensor array '" + m_ArrayName + "' not found on " +
                    (m_Attachment == IG_POINT ? std::string("point data") : std::string("cell data"));
        return false;
    }
    const int dimension = array->GetDimension();
    IGenum attributeType = IG_NONE;
    if (inAttrs != nullptr) {
        auto all = inAttrs->GetAllAttributes();
        for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
            auto& a = all->GetElement(i);
            if (!a.isDeleted && a.pointer == array.GetPointer()) {
                attributeType = a.type;
                break;
            }
        }
    }
    if (!IsSymmetricTensor(dimension, attributeType)) {
        m_Message = "array '" + m_ArrayName + "' has " + std::to_string(dimension) +
                    " components; a symmetric tensor needs 6 (3D: XX,YY,ZZ,XY,YZ,XZ) or "
                    "3 with IG_TENSOR type (2D: XX,YY,XY)";
        return false;
    }

    const IGsize numPoints = static_cast<IGsize>(inPoints->GetNumberOfPoints());
    const IGsize numCells = (outCells != nullptr) ? outCells->GetNumberOfCells() : 0;
    const IGsize expect = (m_Attachment == IG_POINT) ? numPoints : numCells;
    if (expect == 0) {
        m_Message = (m_Attachment == IG_POINT) ? "input mesh has no points"
                                               : "input mesh has no cells";
        return false;
    }

    const IGsize arrayCount = static_cast<IGsize>(array->GetNumberOfElements());
    IGsize computable = expect;      // 真正有来源张量、需要逐元计算的元组数
    if (arrayCount != expect) {
        // 表面网格保留独立线段后，输出单元数 = 面单元数 + 线段数；而单元张量往往
        // 只覆盖原有的面单元（线段是本次新增的几何，没有来源数据）。这种"恰好少掉
        // 独立线段那一部分"的情况予以接受：线段单元写 NaN 并在执行信息里说明。
        // 其余任何长度不符一律明确失败 —— 既不越界读，也不静默补零。
        if (m_Attachment == IG_CELL && looseEdgeCount > 0 && arrayCount > 0 &&
            arrayCount + looseEdgeCount == expect) {
            computable = arrayCount;
        } else {
            m_Message = "tensor array tuple count (" + std::to_string(arrayCount) +
                        ") does not match the mesh " +
                        (m_Attachment == IG_POINT ? std::string("point count (")
                                                  : std::string("cell count (")) +
                        std::to_string(expect) + ")";
            return false;
        }
    }

    // ---- 2) 独立输出节点：点/单元/类型/属性全部深拷贝 ----
    auto outMesh = UnstructuredMesh::New();
    outMesh->SetName(input->GetName() + "_PrincipalInvariants");

    auto outPoints = Points::New();
    outPoints->DeepCopy(inPoints);
    outMesh->SetPoints(outPoints);

    if (outCells != nullptr) { outMesh->SetCells(outCells, outTypes); }

    outMesh->SetAttributeSet(DeepCopyAttributes(inAttrs, m_ArrayName, m_Attachment, numCells));

    // ---- 3) 准备 6 个结果数组（顺序固定：先 3 个向量，再 3 个标量）----
    std::vector<DoubleArray::Pointer> vectors;
    std::vector<DoubleArray::Pointer> values;
    for (int i = 1; i <= 3; ++i) {
        auto v = DoubleArray::New();
        v->SetName(SigmaVectorArrayName(m_ArrayName, i));
        v->SetDimension(3);
        // Resize 收"元素个数"（内部再乘维度 3）
        v->Resize(expect);
        vectors.push_back(v);

        auto s = DoubleArray::New();
        s->SetName(SigmaValueArrayName(m_ArrayName, i));
        s->SetDimension(1);
        s->Resize(expect);
        values.push_back(s);
    }

    // ---- 4) 逐元求主值与主方向 ----
    IGsize nanTuples = 0;
    IGsize failedTuples = 0;
    IGsize noDataTuples = 0;
    for (IGsize i = 0; i < expect; ++i) {
        if (i >= computable) {
            // 独立线段单元：本次新增的几何，没有来源张量数据 → 写 NaN（不静默补 0），
            // 并在执行信息里说明，界面上也能一眼看出这些单元没有主值。
            for (int k = 0; k < 3; ++k) {
                vectors[k]->SetValue(i * 3 + 0, std::nan(""));
                vectors[k]->SetValue(i * 3 + 1, std::nan(""));
                vectors[k]->SetValue(i * 3 + 2, std::nan(""));
                values[k]->SetValue(i, std::nan(""));
            }
            ++noDataTuples;
            continue;
        }

        double comp[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        bool hasNaN = false;
        for (int k = 0; k < dimension; ++k) {
            comp[k] = array->GetElementValue(i, k);
            if (std::isnan(comp[k])) { hasNaN = true; }
        }

        if (hasNaN) {
            // 含 NaN 的元组输出全写 NaN，便于在界面上识别出来
            for (int k = 0; k < 3; ++k) {
                // 向量数组按"扁平标量索引"写入：元素 i 的第 c 个分量 = i*3 + c
                vectors[k]->SetValue(i * 3 + 0, std::nan(""));
                vectors[k]->SetValue(i * 3 + 1, std::nan(""));
                vectors[k]->SetValue(i * 3 + 2, std::nan(""));
                values[k]->SetValue(i, std::nan(""));
            }
            ++nanTuples;
            continue;
        }

        // 按分量顺序装配对称张量
        Eigen::Matrix3d tensor = Eigen::Matrix3d::Zero();
        if (dimension == 3) {
            tensor(0, 0) = comp[0];  // XX
            tensor(1, 1) = comp[1];  // YY
            tensor(0, 1) = tensor(1, 0) = comp[2];  // XY
        } else {
            tensor(0, 0) = comp[0];  // XX
            tensor(1, 1) = comp[1];  // YY
            tensor(2, 2) = comp[2];  // ZZ
            tensor(0, 1) = tensor(1, 0) = comp[3];  // XY
            tensor(1, 2) = tensor(2, 1) = comp[4];  // YZ
            tensor(0, 2) = tensor(2, 0) = comp[5];  // XZ
        }

        Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(tensor);
        if (solver.info() != Eigen::Success) {
            // 分解失败：写 NaN 并计数（不静默当 0）
            for (int k = 0; k < 3; ++k) {
                // 向量数组按"扁平标量索引"写入：元素 i 的第 c 个分量 = i*3 + c
                vectors[k]->SetValue(i * 3 + 0, std::nan(""));
                vectors[k]->SetValue(i * 3 + 1, std::nan(""));
                vectors[k]->SetValue(i * 3 + 2, std::nan(""));
                values[k]->SetValue(i, std::nan(""));
            }
            ++failedTuples;
            continue;
        }

        // Eigen 的特征值按升序排列，且特征向量的符号取决于其内部实现。
        // 这里把特征对整体取出，先按固定惯例规范化符号，再按主值从大到小输出。
        double eigenValues[3] = {solver.eigenvalues()[0], solver.eigenvalues()[1],
                                 solver.eigenvalues()[2]};
        double eigenVectors[3][3];  // eigenVectors[k] = 第 k 个主方向（一行为一个向量）
        for (int k = 0; k < 3; ++k) {
            for (int c = 0; c < 3; ++c) { eigenVectors[k][c] = solver.eigenvectors()(c, k); }
        }
        NormalizePrincipalDirections(eigenValues, eigenVectors);

        int order[3] = {0, 1, 2};
        DecreasingOrder(eigenValues, order);
        for (int k = 0; k < 3; ++k) {
            const int src = order[k];
            const double value = eigenValues[src];
            const double scale = m_ScaleVectors ? value : 1.0;  // 默认输出单位主方向
            vectors[k]->SetValue(i * 3 + 0, scale * eigenVectors[src][0]);
            vectors[k]->SetValue(i * 3 + 1, scale * eigenVectors[src][1]);
            vectors[k]->SetValue(i * 3 + 2, scale * eigenVectors[src][2]);
            values[k]->SetValue(i, value);
        }

        if ((i % 10000) == 0) {
            UpdateProgress(static_cast<double>(i) / static_cast<double>(expect) * 0.9);
        }
    }

    // ---- 5) 写入输出（先删同名，再按固定顺序添加）----
    auto outAttrs = outMesh->GetAttributeSet();
    if (outAttrs == nullptr) {
        m_Message = "failed to create output attribute set";
        return false;
    }
    for (int k = 0; k < 3; ++k) {
        RemoveArrayIfExists(outAttrs, vectors[k]->GetName(), m_Attachment);
        outAttrs->AddAttribute(IG_VECTOR, m_Attachment, vectors[k]);
    }
    for (int k = 0; k < 3; ++k) {
        RemoveArrayIfExists(outAttrs, values[k]->GetName(), m_Attachment);
        outAttrs->AddScalar(m_Attachment, values[k]);
    }

    if (nanTuples > 0 || failedTuples > 0 || noDataTuples > 0) {
        // 不静默：把异常/无数据元组数写进执行信息，界面能看到
        m_Message = "principal invariants computed";
        if (noDataTuples > 0) {
            m_Message += "; " + std::to_string(noDataTuples) +
                         " loose-line cell(s) without tensor data (NaN)";
        }
        if (nanTuples > 0) {
            m_Message += "; " + std::to_string(nanTuples) + " tuple(s) contain NaN";
        }
        if (failedTuples > 0) {
            m_Message += "; " + std::to_string(failedTuples) +
                         " tuple(s) failed eigen decomposition";
        }
    }

    UpdateProgress(1);
    SetOutput(0, outMesh);
    return true;
}

IGAME_NAMESPACE_END
