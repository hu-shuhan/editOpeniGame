/**
 * @class   iGameYieldCriteria
 * @brief   在输入数据集的张量属性上计算屈服准则。
 */

#include "iGameYieldCriteriaFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameType.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <cmath>
#include <string>
#include <type_traits>

namespace {
// 深拷贝输入网格，结果数组只加到新网格上，原模型保持不变
iGame::DataObject::Pointer CloneMesh(iGame::DataObject::Pointer input) {
    using namespace iGame;
    auto copyPoints = [](PointSet* src) -> Points::Pointer {
        auto pts = Points::New();
        pts->DeepCopy(src->GetPoints());
        return pts;
    };
    // 逐个属性按类型深拷贝。
    // 不能直接用 AttributeSet::DeepCopy：它内部只处理 FloatArray / DoubleArray，
    // 整型属性（例如 d3plot 模型的 "Node ID"）会复制失败并留下 pointer 为空的槽；
    // 下游面板遍历属性时不判空，就会在生成新模型后崩溃。
    auto copyAttrs = [](DataObject* src) -> AttributeSet::Pointer {
        auto attrs = AttributeSet::New();
        auto srcSet = src ? src->GetAttributeSet() : nullptr;
        if (srcSet == nullptr) { return attrs; }
        auto cloneTyped = [](auto srcPtr) -> ArrayObject::Pointer {
            using T = std::remove_reference_t<decltype(*srcPtr)>;
            auto dst = T::New();
            dst->SetName(srcPtr->GetName());
            dst->DeepCopy(srcPtr);
            return dst;
        };
        const size_t count = srcSet->GetNumberOfAttributes();
        for (size_t i = 0; i < count; ++i) {
            auto& attr = srcSet->GetAttribute(static_cast<IGsize>(i));
            if (attr.isDeleted || attr.pointer == nullptr) { continue; }
            ArrayObject::Pointer copy;
            if (auto p = DynamicCast<FloatArray>(attr.pointer)) { copy = cloneTyped(p); }
            else if (auto p = DynamicCast<DoubleArray>(attr.pointer)) { copy = cloneTyped(p); }
            else if (auto p = DynamicCast<IntArray>(attr.pointer)) { copy = cloneTyped(p); }
            else if (auto p = DynamicCast<UnsignedIntArray>(attr.pointer)) { copy = cloneTyped(p); }
            else if (auto p = DynamicCast<CharArray>(attr.pointer)) { copy = cloneTyped(p); }
            else if (auto p = DynamicCast<UnsignedCharArray>(attr.pointer)) { copy = cloneTyped(p); }
            else if (auto p = DynamicCast<ShortArray>(attr.pointer)) { copy = cloneTyped(p); }
            else if (auto p = DynamicCast<UnsignedShortArray>(attr.pointer)) { copy = cloneTyped(p); }
            else if (auto p = DynamicCast<LongLongArray>(attr.pointer)) { copy = cloneTyped(p); }
            else if (auto p = DynamicCast<UnsignedLongLongArray>(attr.pointer)) { copy = cloneTyped(p); }
            if (copy) { attrs->AddAttribute(attr.type, attr.attachmentType, copy); }
        }
        return attrs;
    };

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto src = DynamicCast<SurfaceMesh>(input);
            auto dst = SurfaceMesh::New();
            dst->SetPoints(copyPoints(src));
            if (src->GetFaces()) {
                auto faces = CellArray::New();
                faces->DeepCopy(src->GetFaces());
                dst->SetFaces(faces);
            }
            if (src->GetEdges()) {
                auto edges = CellArray::New();
                edges->DeepCopy(src->GetEdges());
                dst->SetEdges(edges);
            }
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto src = DynamicCast<UnstructuredMesh>(input);
            auto dst = UnstructuredMesh::New();
            dst->SetPoints(copyPoints(src));
            auto cells = CellArray::New();
            cells->DeepCopy(src->GetCells());
            auto types = UnsignedIntArray::New();
            types->DeepCopy(src->GetCellTypes());
            dst->SetCells(cells, types);
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_VOLUME_MESH: {
            auto src = DynamicCast<VolumeMesh>(input);
            auto dst = VolumeMesh::New();
            dst->SetPoints(copyPoints(src));
            auto vols = CellArray::New();
            vols->DeepCopy(src->GetVolumes());
            dst->SetVolumes(vols);
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_STRUCTURED_MESH: {
            auto src = DynamicCast<StructuredMesh>(input);
            auto dst = StructuredMesh::New();
            dst->SetPoints(copyPoints(src));
            dst->SetDimensionSize(src->GetDimensionSize());
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_POINT_SET: {
            auto src = DynamicCast<PointSet>(input);
            auto dst = PointSet::New();
            dst->SetPoints(copyPoints(src));
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        default:
            return nullptr;
    }
}

// 把 6 分量（xx,yy,zz,xy,yz,zx）或 9 分量张量读成 3x3 对称矩阵
void ReadTensor3x3(iGame::ArrayObject* array, int dimension, IGsize tupleId, double a[3][3]) {
    if (dimension == 6) {
        const IGsize base = tupleId * 6;
        const double xx = array->GetValue(base + 0);
        const double yy = array->GetValue(base + 1);
        const double zz = array->GetValue(base + 2);
        const double xy = array->GetValue(base + 3);
        const double yz = array->GetValue(base + 4);
        const double zx = array->GetValue(base + 5);
        a[0][0] = xx;
        a[1][1] = yy;
        a[2][2] = zz;
        a[0][1] = a[1][0] = xy;
        a[1][2] = a[2][1] = yz;
        a[0][2] = a[2][0] = zx;
    } else {
        const IGsize base = tupleId * 9;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                a[i][j] = array->GetValue(base + i * 3 + j);
            }
        }
    }
}
} // namespace

IGAME_NAMESPACE_BEGIN

void ComputeSymmetricEigen(double a[3][3], double values[3], double vectors[3][3]) {
    // Jacobi 旋转法：迭代把非对角元素消到 0
    double v[3][3] = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};

    for (int sweep = 0; sweep < 64; ++sweep) {
        const double off = std::fabs(a[0][1]) + std::fabs(a[0][2]) + std::fabs(a[1][2]);
        if (off < 1e-15) break;

        for (int p = 0; p < 2; ++p) {
            for (int q = p + 1; q < 3; ++q) {
                if (std::fabs(a[p][q]) < 1e-18) continue;

                const double theta = (a[q][q] - a[p][p]) / (2.0 * a[p][q]);
                const double sign = (theta >= 0.0) ? 1.0 : -1.0;
                const double t = sign / (std::fabs(theta) + std::sqrt(theta * theta + 1.0));
                const double c = 1.0 / std::sqrt(t * t + 1.0);
                const double s = t * c;

                for (int k = 0; k < 3; ++k) {
                    if (k == p || k == q) continue;
                    const double akp = a[k][p];
                    const double akq = a[k][q];
                    a[k][p] = a[p][k] = c * akp - s * akq;
                    a[k][q] = a[q][k] = s * akp + c * akq;
                }
                const double app = a[p][p];
                const double aqq = a[q][q];
                const double apq = a[p][q];
                a[p][p] = c * c * app - 2.0 * s * c * apq + s * s * aqq;
                a[q][q] = s * s * app + 2.0 * s * c * apq + c * c * aqq;
                a[p][q] = a[q][p] = 0.0;

                for (int k = 0; k < 3; ++k) {
                    const double vkp = v[k][p];
                    const double vkq = v[k][q];
                    v[k][p] = c * vkp - s * vkq;
                    v[k][q] = s * vkp + c * vkq;
                }
            }
        }
    }

    for (int i = 0; i < 3; ++i) { values[i] = a[i][i]; }

    // 特征值降序，并同步交换对应特征向量（v 的列 -> 输出行）
    int order[3] = {0, 1, 2};
    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 3; ++j) {
            if (values[order[j]] > values[order[i]]) {
                const int tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }
    double sortedValues[3] = {values[order[0]], values[order[1]], values[order[2]]};
    double sortedVectors[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) { sortedVectors[i][k] = v[k][order[i]]; }
    }
    for (int i = 0; i < 3; ++i) {
        values[i] = sortedValues[i];
        for (int k = 0; k < 3; ++k) { vectors[i][k] = sortedVectors[i][k]; }
    }
}

bool YieldCriteriaFilter::Execute() {
    m_StatusMessage.clear();

    auto input = GetInput(0);
    if (input == nullptr) {
        m_StatusMessage = "输入为空。";
        return false;
    }
    auto srcPointSet = DynamicCast<PointSet>(input);
    if (srcPointSet == nullptr) {
        m_StatusMessage = "当前对象不支持屈服准则计算（需要网格 / 点集）。";
        return false;
    }
    if (m_Criteria.empty()) {
        auto output = CloneMesh(input);
        if (!output) {
            m_StatusMessage = "当前数据类型不支持复制。";
            return false;
        }
        output->SetName(input->GetName() + "_YieldCriteria");
        output->SetTimeFrames(input->PeekTimeFrames());
        SetOutput(output);
        m_StatusMessage = "未选择屈服准则，已输出原模型的数据副本。";
        return true;
    }
    auto srcAttrSet = input->GetAttributeSet();
    if (srcAttrSet == nullptr) {
        m_StatusMessage = "输入没有属性数据。";
        return false;
    }

    // 1) 收集参与计算的张量属性（可多个，点数据与单元数据可混合）。
    //    优先按用户指定的名字；未指定时自动取一个：先找标记为张量的 6/9 分量属性，
    //    再退回到任意 6 / 9 分量属性（部分读取器不把 6 分量数组标记为张量类型）。
    struct TargetTensor {
        ArrayObject::Pointer array;
        IGenum attachment{IG_POINT};
        IGsize tuples{0};
        std::string name;
        int dimension{6};
    };
    std::vector<TargetTensor> targets;

    auto addTargetByName = [&](const std::string& name) -> bool {
        for (const auto& t : targets) {
            if (t.name == name) { return true; } // 去重
        }
        const int index = srcAttrSet->GetAttributeIndex(name);
        if (index < 0) {
            m_StatusMessage = "找不到张量属性：" + name;
            return false;
        }
        auto& attr = srcAttrSet->GetAttribute(static_cast<IGsize>(index));
        if (attr.isDeleted || attr.pointer == nullptr) {
            m_StatusMessage = "张量属性 \"" + name + "\" 不可用。";
            return false;
        }
        const int dim = attr.pointer->GetDimension();
        if (dim != 6 && dim != 9) {
            m_StatusMessage = "属性 \"" + name + "\" 不是 6 或 9 分量的张量。";
            return false;
        }
        TargetTensor t;
        t.array = attr.pointer;
        t.attachment = attr.attachmentType;
        t.tuples = attr.pointer->GetNumberOfElements();
        t.name = name;
        t.dimension = dim;
        targets.push_back(t);
        return true;
    };

    if (!m_TensorArrayNames.empty()) {
        for (const auto& name : m_TensorArrayNames) {
            if (name.empty()) continue;
            if (!addTargetByName(name)) { return false; }
        }
    } else {
        int fallbackIndex = -1;
        const size_t numAttrs = srcAttrSet->GetNumberOfAttributes();
        for (size_t i = 0; i < numAttrs; ++i) {
            auto& attr = srcAttrSet->GetAttribute(static_cast<IGsize>(i));
            if (attr.isDeleted || attr.pointer == nullptr) continue;
            const int dim = attr.pointer->GetDimension();
            if (dim != 6 && dim != 9) continue;
            if (attr.type == IG_TENSOR) {
                addTargetByName(attr.pointer->GetName());
                break;
            }
            if (fallbackIndex < 0) { fallbackIndex = static_cast<int>(i); }
        }
        if (targets.empty() && fallbackIndex >= 0) {
            auto& attr = srcAttrSet->GetAttribute(static_cast<IGsize>(fallbackIndex));
            addTargetByName(attr.pointer->GetName());
        }
    }
    if (targets.empty()) {
        m_StatusMessage = "输入里没有 6 或 9 分量的张量属性。";
        return false;
    }

    // 2) 深拷贝输入（几何/拓扑不变，结果只作为新数组追加）
    auto newMesh = CloneMesh(input);
    if (newMesh == nullptr) {
        m_StatusMessage = "当前数据类型不支持屈服准则计算。";
        return false;
    }
    auto dstAttrSet = newMesh->GetAttributeSet();

    auto makeScalarArray = [](const std::string& name, IGsize count) {
        auto array = DoubleArray::New();
        array->SetName(name);
        array->SetDimension(1);
        array->Reserve(count);
        return array;
    };
    auto makeVectorArray = [](const std::string& name, IGsize count) {
        auto array = DoubleArray::New();
        array->SetName(name);
        array->SetDimension(3);
        array->Reserve(count);
        return array;
    };

    // 3) 逐个张量属性 × 逐个准则计算并追加结果数组
    auto computeOne = [&](const TargetTensor& target, const Criterion criterion) {
        ArrayObject* tensorArray = target.array.GetPointer();
        const int dimension = target.dimension;
        const IGsize numTuples = target.tuples;
        const IGenum attachment = target.attachment;
        const std::string& tensorName = target.name;

        if (criterion == PRINCIPAL_STRESS) {
            auto sigma1 = makeScalarArray(tensorName + " - Sigma 1", numTuples);
            auto sigma2 = makeScalarArray(tensorName + " - Sigma 2", numTuples);
            auto sigma3 = makeScalarArray(tensorName + " - Sigma 3", numTuples);
            auto dir1 = makeVectorArray(tensorName + " - Sigma 1 (Vector)", numTuples);
            auto dir2 = makeVectorArray(tensorName + " - Sigma 2 (Vector)", numTuples);
            auto dir3 = makeVectorArray(tensorName + " - Sigma 3 (Vector)", numTuples);

            for (IGsize t = 0; t < numTuples; ++t) {
                double a[3][3];
                ReadTensor3x3(tensorArray, dimension, t, a);
                double values[3];
                double vectors[3][3];
                ComputeSymmetricEigen(a, values, vectors);
                sigma1->AddValue(values[0]);
                sigma2->AddValue(values[1]);
                sigma3->AddValue(values[2]);
                dir1->AddElement3(vectors[0][0], vectors[0][1], vectors[0][2]);
                dir2->AddElement3(vectors[1][0], vectors[1][1], vectors[1][2]);
                dir3->AddElement3(vectors[2][0], vectors[2][1], vectors[2][2]);
            }

            dstAttrSet->AddAttribute(IG_SCALAR, attachment, sigma1);
            dstAttrSet->AddAttribute(IG_SCALAR, attachment, sigma2);
            dstAttrSet->AddAttribute(IG_SCALAR, attachment, sigma3);
            dstAttrSet->AddAttribute(IG_VECTOR, attachment, dir1);
            dstAttrSet->AddAttribute(IG_VECTOR, attachment, dir2);
            dstAttrSet->AddAttribute(IG_VECTOR, attachment, dir3);
        } else {
            const bool isTresca = (criterion == TRESCA);
            const std::string resultName =
                    tensorName + (isTresca ? " - Tresca Criterion" : " - Von Mises Criterion");
            auto result = makeScalarArray(resultName, numTuples);

            for (IGsize t = 0; t < numTuples; ++t) {
                double a[3][3];
                ReadTensor3x3(tensorArray, dimension, t, a);
                double values[3];
                double vectors[3][3];
                ComputeSymmetricEigen(a, values, vectors);
                const double sigma1 = values[0];
                const double sigma2 = values[1];
                const double sigma3 = values[2];
                double value = 0.0;
                if (isTresca) {
                    value = sigma1 - sigma3;
                } else {
                    const double d12 = sigma1 - sigma2;
                    const double d23 = sigma2 - sigma3;
                    const double d31 = sigma3 - sigma1;
                    value = std::sqrt(0.5 * (d12 * d12 + d23 * d23 + d31 * d31));
                }
                result->AddValue(value);
            }
            dstAttrSet->AddAttribute(IG_SCALAR, attachment, result);
        }
    };

    for (const auto& target : targets) {
        for (const auto criterion : m_Criteria) { computeOne(target, criterion); }
    }

    dstAttrSet->ForceReConvertToDrawableData();
    newMesh->SetName(input->GetName() + "_YieldCriteria");
    // 保留输入的时间帧信息。下游（动画组件、形变信息面板等）在模型切换时会读取时间帧，
    // 若深拷贝丢了时间帧，那些面板会在空时间帧上取 front()/back() 而崩溃。
    newMesh->SetTimeFrames(input->PeekTimeFrames());
    SetOutput(newMesh);

    auto criterionName = [](Criterion c) -> const char* {
        switch (c) {
            case PRINCIPAL_STRESS: return "主应力 (Principal Stress)";
            case TRESCA: return "Tresca 准则";
            default: return "von Mises 准则";
        }
    };
    std::string criteriaText;
    for (size_t i = 0; i < m_Criteria.size(); ++i) {
        if (i > 0) { criteriaText += "、"; }
        criteriaText += criterionName(m_Criteria[i]);
    }
    std::string names;
    for (size_t i = 0; i < targets.size(); ++i) {
        if (i > 0) { names += "、"; }
        names += "\"" + targets[i].name + "\"";
    }
    m_StatusMessage = "基于张量属性 " + names + " 计算 " + criteriaText + "。";
    return true;
}

void YieldCriteriaFilter::SetTensorArrayNames(const std::vector<std::string>& names) {
    m_TensorArrayNames = names;
}

const std::vector<std::string>& YieldCriteriaFilter::GetTensorArrayNames() const {
    return m_TensorArrayNames;
}

void YieldCriteriaFilter::SetTensorArrayName(const std::string& name) {
    m_TensorArrayNames.clear();
    if (!name.empty()) { m_TensorArrayNames.push_back(name); }
}

std::string YieldCriteriaFilter::GetTensorArrayName() const {
    return m_TensorArrayNames.empty() ? std::string() : m_TensorArrayNames.front();
}

void YieldCriteriaFilter::SetCriteria(const std::vector<Criterion>& criteria) {
    m_Criteria = criteria;
}

const std::vector<YieldCriteriaFilter::Criterion>& YieldCriteriaFilter::GetCriteria() const {
    return m_Criteria;
}

void YieldCriteriaFilter::SetCriterion(Criterion criterion) { SetCriteria({criterion}); }

const YieldCriteriaFilter::Criterion& YieldCriteriaFilter::GetCriterion() const {
    static const Criterion defaultCriterion = PRINCIPAL_STRESS;
    return m_Criteria.empty() ? defaultCriterion : m_Criteria.front();
}

const std::string& YieldCriteriaFilter::GetStatusMessage() const { return m_StatusMessage; }

YieldCriteriaFilter::YieldCriteriaFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

IGAME_NAMESPACE_END
