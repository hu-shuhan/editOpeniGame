#include "iGameExtractComponentFilter.h"

#include "iGameAttributeSet.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

namespace {

// 按输入数组的具体类型创建同类型数组（写法对齐 iGameExtractSubsetFilter 的 CreateArrayLike）：
// 保持 Int / LongLong 等类型不变，避免隐式类型转换或精度丢失。
// 类型不在下表时返回 nullptr，由调用方回落到 DoubleArray，保证特殊数组也不中断。
ArrayObject::Pointer CreateArrayLike(const ArrayObject::Pointer& inArray) {
    if (inArray == nullptr) return nullptr;
    if (DynamicCast<FloatArray>(inArray)) return FloatArray::New();
    if (DynamicCast<DoubleArray>(inArray)) return DoubleArray::New();
    if (DynamicCast<IntArray>(inArray)) return IntArray::New();
    if (DynamicCast<UnsignedIntArray>(inArray)) return UnsignedIntArray::New();
    if (DynamicCast<LongLongArray>(inArray)) return LongLongArray::New();
    if (DynamicCast<UnsignedLongLongArray>(inArray)) return UnsignedLongLongArray::New();
    if (DynamicCast<CharArray>(inArray)) return CharArray::New();
    if (DynamicCast<UnsignedCharArray>(inArray)) return UnsignedCharArray::New();
    if (DynamicCast<ShortArray>(inArray)) return ShortArray::New();
    if (DynamicCast<UnsignedShortArray>(inArray)) return UnsignedShortArray::New();
    return nullptr;
}

// 结果对象持有独立的 Points（与输入共享底层缓冲，但时间戳互不影响）：
// 若直接把输入的 Points 交给 result->SetPoints()，PointSet::SetPoints() 会对该共享对象调
// Modified()，把输入模型的几何时间戳顶掉，使输入模型也重跑一遍表面提取。
Points::Pointer CreateSharedPoints(const Points::Pointer& source) {
    if (source == nullptr) { return nullptr; }
    auto points = Points::New();
    points->ShallowCopy(source);
    return points;
}

}  // namespace

ExtractComponentFilter::ExtractComponentFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void ExtractComponentFilter::SetComponents(int c1) {
    const bool modified = (m_Components[0] != c1) || (m_NumberOfComponents != 1);
    m_Components[0] = c1;
    m_NumberOfComponents = 1;
    if (modified) { Modified(); }
}

void ExtractComponentFilter::SetComponents(int c1, int c2) {
    const bool modified = (m_Components[0] != c1) || (m_Components[1] != c2) || (m_NumberOfComponents != 2);
    m_Components[0] = c1;
    m_Components[1] = c2;
    m_NumberOfComponents = 2;
    if (modified) { Modified(); }
}

void ExtractComponentFilter::SetComponents(int c1, int c2, int c3) {
    const bool modified = (m_Components[0] != c1) || (m_Components[1] != c2) || (m_Components[2] != c3) ||
                          (m_NumberOfComponents != 3);
    m_Components[0] = c1;
    m_Components[1] = c2;
    m_Components[2] = c3;
    m_NumberOfComponents = 3;
    if (modified) { Modified(); }
}

bool ExtractComponentFilter::Execute() {
    m_Message.clear();
    // 执行失败时不保留上一次的结果
    this->SetOutput(0, nullptr);

    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "输入数据对象为空";
        return false;
    }

    auto attributeSet = input->GetAttributeSet();
    if (attributeSet == nullptr) {
        m_Message = "输入数据对象没有属性集";
        return false;
    }

    if (m_OutputArrayName.empty()) {
        m_Message = "输出数组名不能为空";
        return false;
    }

    // 输入数组解析：按「名字 + 挂载类型」定位；名字为空时取第一个匹配挂载限制的数组。
    // 挂载类型用于区分同名的点 / 单元数组（如 Velocity 同时挂在 PointData 与 CellData）。
    AttributeSet::Attribute attr;
    auto matches = [&](const AttributeSet::Attribute& candidate) {
        if (candidate.IsNone() || candidate.pointer == nullptr) return false;
        if (!m_InputArrayName.empty() && candidate.pointer->GetName() != m_InputArrayName) return false;
        if (m_InputAttachmentType != IG_NONE && candidate.attachmentType != m_InputAttachmentType) return false;
        return true;
    };
    auto inputAttributes = attributeSet->GetAllAttributes();
    for (IGsize i = 0; i < inputAttributes->GetNumberOfElements(); ++i) {
        auto& candidate = inputAttributes->GetElement(i);
        if (matches(candidate)) {
            attr = candidate;
            break;
        }
    }
    if (attr.IsNone()) {
        m_Message = m_InputArrayName.empty() ? "找不到输入数组" : "找不到输入数组: " + m_InputArrayName;
        return false;
    }

    // 分量索引校验：不允许负数，也不允许超出数组维度（如 1/2 维数组提取第 3 个分量）
    const int dimension = attr.pointer->GetDimension();
    for (int c = 0; c < m_NumberOfComponents; ++c) {
        if (m_Components[c] < 0 || m_Components[c] >= dimension) {
            m_Message = "分量索引超出数组维度";
            return false;
        }
    }

    // 输出数组：与输入同类型，维度 = 提取的分量个数
    // （FlatArray::Resize 按「元素数 × 维度」分配，所以必须先 SetDimension 再 Resize；
    //   写入用扁平下标 i * 维度 + c）
    const IGsize elementNum = attr.pointer->GetNumberOfElements();
    const IGsize outDimension = static_cast<IGsize>(m_NumberOfComponents);
    ArrayObject::Pointer output = CreateArrayLike(attr.pointer);
    if (output == nullptr) { output = DoubleArray::New(); }
    output->SetDimension(m_NumberOfComponents);
    output->SetName(m_OutputArrayName);
    output->Resize(elementNum);
    for (IGsize i = 0; i < elementNum; ++i) {
        for (int c = 0; c < m_NumberOfComponents; ++c) {
            output->SetValue(i * outDimension + static_cast<IGsize>(c),
                             attr.pointer->GetElementValue(i, m_Components[c]));
        }
    }

    // 继承语义：输出新数据对象，几何与输入共享；
    // 结果属性集 = 输入属性集的拷贝 + 新增结果数组，跳过与输出名同名的旧数组（覆盖语义）。
    // 注意 1：不能用 DeleteAttribute 标记删除（渲染路径按索引遍历会解引用空指针），
    //         拷贝时直接跳过同名旧数组，保证结果属性集不含 isDeleted 残留项。
    // 注意 2：默认深拷贝（Attribute::DeepCopy 覆盖全部数组类型并保留维度与 dataRange），
    //         结果与输入完全解耦；SetShallowCopyAttributes(true) 可改为只读共享输入数组，
    //         省掉大模型的属性内存翻倍（结果只新增自己的数组，从不修改已有数组）。
    auto resultAttrSet = AttributeSet::New();
    auto allAttributes = attributeSet->GetAllAttributes();
    for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
        auto& src = allAttributes->GetElement(i);
        if (src.IsNone() || src.pointer == nullptr) continue;
        if (src.pointer->GetName() == m_OutputArrayName) continue;
        if (m_ShallowCopyAttributes) {
            resultAttrSet->AddAttribute(src.type, src.attachmentType, src.pointer);
            continue;
        }
        AttributeSet::Attribute copied;
        if (copied.DeepCopy(src)) {
            resultAttrSet->AddAttribute(copied.type, copied.attachmentType, copied.pointer, copied.dataRange);
        } else {
            // 未知数组类型无法克隆（Attribute::DeepCopy 只覆盖 FlatArray 系列）：
            // 退回只读共享，保证属性不丢
            resultAttrSet->AddAttribute(src.type, src.attachmentType, src.pointer);
        }
    }
    resultAttrSet->AddScalar(attr.attachmentType, output);

    if (auto unstructured = DynamicCast<UnstructuredMesh>(input); unstructured != nullptr) {
        auto result = UnstructuredMesh::New();
        result->SetName(unstructured->GetName() + "_ExtractComponent");
        result->SetPoints(CreateSharedPoints(unstructured->GetPoints()));
        result->SetCells(unstructured->GetCells(), UnsignedIntArray::Pointer(unstructured->GetCellTypes()));
        result->SetAttributeSet(resultAttrSet);
        SetOutput(result);
        return true;
    }

    if (auto surface = DynamicCast<SurfaceMesh>(input); surface != nullptr) {
        auto result = SurfaceMesh::New();
        result->SetName(surface->GetName() + "_ExtractComponent");
        result->SetPoints(CreateSharedPoints(surface->GetPoints()));
        result->SetFaces(surface->GetFaces());
        result->SetAttributeSet(resultAttrSet);
        SetOutput(result);
        return true;
    }

    m_Message = "暂不支持该数据类型（仅支持非结构化网格/表面网格）";
    return false;
}

IGAME_NAMESPACE_END
