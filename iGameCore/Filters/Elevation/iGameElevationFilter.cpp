#include "Elevation/iGameElevationFilter.h"

#include "iGameAttributeSet.h"
#include "iGameFlatArray.h"
#include "iGameMacro.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

// 构造标量属性的取色范围数组（行 0 = 模长范围、行 1 = 分量范围，一维标量两者一致）
DoubleArray::Pointer MakeScalarDataRange(double minVal, double maxVal) {
    auto range = DoubleArray::New();
    range->SetDimension(2);
    range->Resize(2);
    range->SetElement(0, {minVal, maxVal});
    range->SetElement(1, {minVal, maxVal});
    return range;
}

// 锁定 Elevation 属性的 grow-only 取色范围（rangeLocked + ExpandOnly）：
// 锁定后框架的 UpdateAllDataRange 不再按当次数据重算，颜色条 / 标量面板 / mapper
// 与本过滤器维护的只扩不缩范围同源——标量范围改大再改小，颜色条保持历史
// 最大范围不回缩（对齐 ParaView 颜色条行为）
void LockGrowOnlyRange(const AttributeSet::Pointer& attrs, const std::string& name) {
    const int idx = (attrs != nullptr) ? attrs->GetAttributeIndex(name) : -1;
    if (idx < 0) { return; }
    auto& attr = attrs->GetAttribute(idx);
    attr.rangeLocked = true;
    attr.rangeMode = AttributeSet::RangeMode::ExpandOnly;
}

}  // namespace

ElevationFilter::ElevationFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void ElevationFilter::SetLowPoint(double x, double y, double z) {
    const Vector3f p(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    if (m_LowPoint[0] != p[0] || m_LowPoint[1] != p[1] || m_LowPoint[2] != p[2]) {
        m_LowPoint = p;
        this->Modified();
    }
}

void ElevationFilter::SetLowPoint(const Vector3f& p) {
    SetLowPoint(p[0], p[1], p[2]);
}

void ElevationFilter::SetHighPoint(double x, double y, double z) {
    const Vector3f p(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    if (m_HighPoint[0] != p[0] || m_HighPoint[1] != p[1] || m_HighPoint[2] != p[2]) {
        m_HighPoint = p;
        this->Modified();
    }
}

void ElevationFilter::SetHighPoint(const Vector3f& p) {
    SetHighPoint(p[0], p[1], p[2]);
}

void ElevationFilter::SetScalarRange(double low, double high) {
    if (low >= high) { return; }  // 非法范围（含相等）被拒绝并保持原值
    if (m_RangeLow != low || m_RangeHigh != high) {
        m_RangeLow = low;
        m_RangeHigh = high;
        this->Modified();
    }
}

void ElevationFilter::SetArrayName(const std::string& name) {
    if (m_ArrayName != name) {
        m_ArrayName = name;
        this->Modified();
    }
}

bool ElevationFilter::Execute() {
    IGAME_CORE_INFO("ElevationFilter: Execute() start (low point = ({}, {}, {}), "
                    "high point = ({}, {}, {}), scalar range = [{}, {}])",
                    m_LowPoint[0], m_LowPoint[1], m_LowPoint[2],
                    m_HighPoint[0], m_HighPoint[1], m_HighPoint[2],
                    m_RangeLow, m_RangeHigh);

    // ===== 1. 输入校验 =====
    auto obj = GetInput(0);
    if (obj == nullptr) {
        igError("ElevationFilter: GetInput(0) is nullptr!");
        return false;
    }
    auto mesh = DynamicCast<PointSet>(obj);
    if (mesh == nullptr) {
        igError("ElevationFilter: DynamicCast<PointSet> failed!");
        return false;
    }
    const IGsize nPoints = mesh->GetNumberOfPoints();
    if (nPoints == 0) {
        igError("ElevationFilter: No points in mesh!");
        return false;
    }

    // 标尺线段方向 v = 高点 − 低点（全程 double 计算，避免 float 累积误差）；
    // 两点重合时线段退化为一个点，无法定义投影方向，拒绝执行
    const double vx = static_cast<double>(m_HighPoint[0]) - m_LowPoint[0];
    const double vy = static_cast<double>(m_HighPoint[1]) - m_LowPoint[1];
    const double vz = static_cast<double>(m_HighPoint[2]) - m_LowPoint[2];
    const double l2 = vx * vx + vy * vy + vz * vz;  // |v|²
    if (l2 == 0.0) {
        igError("ElevationFilter: low point and high point coincide!");
        return false;
    }

    // ===== 2. 核心计算（公式对齐 vtkElevationFilter）=====
    // t = ((p − 低点)·v) / |v|²：点 p 在标尺线段上的参数化坐标（低点为 0，高点为 1），
    // 夹断到 [0,1]（低于低点/高于高点饱和在端值），再按标量范围线性映射为输出值；
    // 低点/高点垂直于 v 的分量在点积中自动归零，不影响结果。
    // 同一趟循环统计输出数据的实际范围，供取色范围使用。
    auto elevArr = FloatArray::New();
    elevArr->SetName(m_ArrayName);
    elevArr->SetDimension(1);
    elevArr->Resize(nPoints);

    const double diffScalar = m_RangeHigh - m_RangeLow;  // SetScalarRange 已保证 > 0
    double dataMin = m_RangeLow;  // 夹断保证输出值域落在 [m_RangeLow, m_RangeHigh] 内
    double dataMax = m_RangeLow;
    for (IGsize i = 0; i < nPoints; ++i) {
        const Vector3f pt = mesh->GetPoint(i);
        const double proj =
            (static_cast<double>(pt[0]) - m_LowPoint[0]) * vx +
            (static_cast<double>(pt[1]) - m_LowPoint[1]) * vy +
            (static_cast<double>(pt[2]) - m_LowPoint[2]) * vz;
        double t = proj / l2;
        if (t < 0.0) { t = 0.0; }
        else if (t > 1.0) { t = 1.0; }
        const double value = m_RangeLow + t * diffScalar;
        elevArr->SetValue(i, static_cast<float>(value));
        if (value < dataMin) { dataMin = value; }
        if (value > dataMax) { dataMax = value; }
    }

    // ===== 3. grow-only 取色范围（与 ParaView 颜色条行为一致）=====
    // 首次执行挂载输出数据的实际范围；此后只扩不缩——新数据超出历史范围时扩张，
    // 缩回时保持。效果：标量范围改大后改小，颜色条保持历史最大范围不回缩。
    // 需要重置时可在标量场面板把映射范围模式切回"每帧调整"（相当于 Rescale）。
    if (!m_RunningRangeValid) {
        m_RunningMin = dataMin;
        m_RunningMax = dataMax;
        m_RunningRangeValid = true;
    } else {
        if (dataMin < m_RunningMin) { m_RunningMin = dataMin; }
        if (dataMax > m_RunningMax) { m_RunningMax = dataMax; }
    }
    // 退化兜底：数据为常量（如整模型低于低点全部饱和）时扩出单位宽度，避免取色除零
    const double displayMin = m_RunningMin;
    const double displayMax = (m_RunningMax > m_RunningMin) ? m_RunningMax : (m_RunningMin + 1.0);

    // ===== 4a. 输出复用：输入未变时仅替换数组，保留取色范围 =====
    // 面板"应用"场景：同一输入反复调整参数重新执行，复用已有输出对象，
    // 模型树始终只有一个独立输出节点；属性上的 dataRange / 范围模式等状态原样保留。
    if (m_Output != nullptr && m_LastInput.GetPointer() == obj.GetPointer()) {
        auto attrs = m_Output->GetAttributeSet();
        const int idx = (attrs != nullptr) ? attrs->GetAttributeIndex(m_ArrayName) : -1;
        if (idx >= 0) {
            auto& attr = attrs->GetAttribute(idx);
            attr.SetPointer(elevArr);  // 只换数组指针，不重建属性
            LockGrowOnlyRange(attrs, m_ArrayName);  // 防御：属性曾被外部解锁时补锁
            auto range = attr.GetDataRange();
            if (range == nullptr) {
                range = MakeScalarDataRange(displayMin, displayMax);
                attr.SetDataRange(range);
            } else {
                range->SetElement(0, {displayMin, displayMax});
                range->SetElement(1, {displayMin, displayMax});
                range->Modified();
            }
        } else {
            // 异常兜底：原属性已被外部移除时重新挂载（含范围锁定）
            attrs->AddScalar(IG_POINT, elevArr, MakeScalarDataRange(displayMin, displayMax));
            LockGrowOnlyRange(attrs, m_ArrayName);
        }
        m_Output->Modified();
        SetOutput(0, m_Output);
        IGAME_CORE_INFO("ElevationFilter: Execute() done (reused output, values in [{}, {}], "
                        "color range [{}, {}])", dataMin, dataMax, displayMin, displayMax);
        return true;
    }

    // ===== 4b. 新建独立输出：不修改输入对象 =====
    // 输出新的数据对象，几何（点/面/单元）与输入共享；
    // 结果属性集 = 输入属性集的拷贝（跳过与输出数组同名的旧数组，即覆盖语义）
    //               + 新增 Elevation 数组（带 grow-only 取色范围）；
    // 输入对象保持原样（不挂 Elevation 数组），输出可在模型树中作为独立节点展示。
    // 注意：不能用 DeleteAttribute 标记删除（渲染路径按索引遍历会解引用空指针），
    // 拷贝时直接跳过同名旧数组，保证结果属性集不含 isDeleted 残留项。
    auto inputAttrSet = mesh->GetAttributeSet();
    auto resultAttrSet = AttributeSet::New();
    if (inputAttrSet != nullptr) {
        auto allAttributes = inputAttrSet->GetAllAttributes();
        if (allAttributes != nullptr) {
            for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
                auto& src = allAttributes->GetElement(i);
                if (src.IsNone()) { continue; }
                // 覆盖语义：跳过与输出数组同名的旧数组
                if (src.pointer->GetName() == m_ArrayName) { continue; }
                ArrayObject::Pointer copied;
                if (DynamicCast<FloatArray>(src.pointer) != nullptr) {
                    auto p = FloatArray::New();
                    p->DeepCopy(DynamicCast<FloatArray>(src.pointer));
                    p->SetName(src.pointer->GetName());
                    copied = p;
                } else if (DynamicCast<DoubleArray>(src.pointer) != nullptr) {
                    auto p = DoubleArray::New();
                    p->DeepCopy(DynamicCast<DoubleArray>(src.pointer));
                    p->SetName(src.pointer->GetName());
                    copied = p;
                } else {
                    copied = src.pointer;  // 其他类型共享指针（只读属性，安全）
                }
                resultAttrSet->AddAttribute(src.type, src.attachmentType, copied,
                                             src.GetDataRange());
            }
        }
    }
    resultAttrSet->AddScalar(IG_POINT, elevArr, MakeScalarDataRange(displayMin, displayMax));
    LockGrowOnlyRange(resultAttrSet, m_ArrayName);  // 锁定 grow-only 取色范围，见函数注释

    IGAME_CORE_INFO("ElevationFilter: Added {} (IG_SCALAR/IG_POINT) to independent output, "
                    "elements = {}, values in [{}, {}], color range [{}, {}]",
                    m_ArrayName, elevArr->GetNumberOfElements(), dataMin, dataMax,
                    displayMin, displayMax);

    // 按输入的具体网格类型派生输出对象（几何共享、属性独立）
    DataObject::Pointer result;
    if (auto unstructured = DynamicCast<UnstructuredMesh>(mesh); unstructured != nullptr) {
        auto r = UnstructuredMesh::New();
        r->SetName(unstructured->GetName() + "_Elevation");
        r->SetPoints(unstructured->GetPoints());
        r->SetCells(unstructured->GetCells(),
                    UnsignedIntArray::Pointer(unstructured->GetCellTypes()));
        r->SetAttributeSet(resultAttrSet);
        result = r;
    } else if (auto surface = DynamicCast<SurfaceMesh>(mesh); surface != nullptr) {
        // SurfaceMesh 及其派生类（VolumeMesh / StructuredMesh）都走表面网格分支，
        // 共享输入的几何指针，不修改输入拓扑
        auto r = SurfaceMesh::New();
        r->SetName(surface->GetName() + "_Elevation");
        r->SetPoints(surface->GetPoints());
        r->SetFaces(surface->GetFaces());
        r->SetAttributeSet(resultAttrSet);
        result = r;
    } else {
        // 兜底：裸 PointSet（点云）只共享点几何
        auto r = PointSet::New();
        r->SetName(mesh->GetName() + "_Elevation");
        r->SetPoints(mesh->GetPoints());
        r->SetAttributeSet(resultAttrSet);
        result = r;
    }

    // 记录本次输入与输出：下次执行若输入未变则走复用分支
    m_LastInput = obj;
    m_Output = result;
    SetOutput(0, result);
    IGAME_CORE_INFO("ElevationFilter: Execute() done (independent output created)");
    return true;
}

IGAME_NAMESPACE_END
