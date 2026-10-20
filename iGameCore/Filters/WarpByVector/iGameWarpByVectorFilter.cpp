#include "iGameWarpByVectorFilter.h"

#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameDrawObject.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>
#include <iGameStructuredMesh.h>

#include <algorithm>
#include <cmath>
#include <iostream>

IGAME_NAMESPACE_BEGIN
namespace {

// 创建与源数组同类型、同名、同维度的空数组
ArrayObject::Pointer CreateArrayLike(const ArrayObject::Pointer& source) {
    if (!source) { return nullptr; }

    ArrayObject::Pointer result;
    switch (source->GetArrayType()) {
        case IG_FloatArray: result = FloatArray::New(); break;
        case IG_DoubleArray: result = DoubleArray::New(); break;
        case IG_IntArray:
        case IG_INTARRAY: result = IntArray::New(); break;
        case IG_UnsignedIntArray: result = UnsignedIntArray::New(); break;
        case IG_CharArray: result = CharArray::New(); break;
        case IG_UnsignedCharArray: result = UnsignedCharArray::New(); break;
        case IG_ShortArray: result = ShortArray::New(); break;
        case IG_UnsignedShortArray: result = UnsignedShortArray::New(); break;
        case IG_LongLongArray: result = LongLongArray::New(); break;
        case IG_UnsignedLongLongArray: result = UnsignedLongLongArray::New(); break;
        default: return nullptr;
    }

    result->SetName(source->GetName());
    result->SetDimension(source->GetDimension());
    return result;
}

// 按实际存储类型整表拷贝(与 GenerateIds 保持一致:不经 double 中转)
template <typename TValue>
void CopyAllElements(const ArrayObject::Pointer& source, const ArrayObject::Pointer& result) {
    auto src = DynamicCast<FlatArray<TValue>>(source);
    auto dst = DynamicCast<FlatArray<TValue>>(result);
    if (!src || !dst) { return; }

    const IGsize count = source->GetNumberOfElements();
    const int dimension = source->GetDimension();
    dst->Resize(count);
    for (IGsize i = 0; i < count; ++i) {
        const TValue* srcElement = src->RawPointer(i);
        TValue* dstElement = dst->RawPointer(i);
        for (int c = 0; c < dimension; ++c) {
            dstElement[c] = srcElement[c];
        }
    }
}

bool CopyArrayFully(const ArrayObject::Pointer& source, ArrayObject::Pointer& result) {
    result = CreateArrayLike(source);
    if (!result) { return false; }

    switch (source->GetArrayType()) {
        case IG_FloatArray: CopyAllElements<float>(source, result); break;
        case IG_DoubleArray: CopyAllElements<double>(source, result); break;
        case IG_IntArray:
        case IG_INTARRAY: CopyAllElements<int>(source, result); break;
        case IG_UnsignedIntArray: CopyAllElements<unsigned int>(source, result); break;
        case IG_CharArray: CopyAllElements<char>(source, result); break;
        case IG_UnsignedCharArray: CopyAllElements<unsigned char>(source, result); break;
        case IG_ShortArray: CopyAllElements<short>(source, result); break;
        case IG_UnsignedShortArray: CopyAllElements<unsigned short>(source, result); break;
        case IG_LongLongArray: CopyAllElements<long long>(source, result); break;
        case IG_UnsignedLongLongArray: CopyAllElements<unsigned long long>(source, result); break;
        default: return false;
    }
    return true;
}

// 拷贝整个属性集合(框架的 Attribute::DeepCopy 只支持 float/double,故按类型分发)
bool CopyAttributeSet(DataObject::Pointer input, DataObject::Pointer output) {
    auto outputAttributes = AttributeSet::New();
    if (auto inputAttributes = input->GetAttributeSet()) {
        for (IGsize i = 0; i < static_cast<IGsize>(inputAttributes->GetNumberOfAttributes()); ++i) {
            auto& attr = inputAttributes->GetAttribute(i);
            if (attr.isDeleted || !attr.pointer) { continue; }

            ArrayObject::Pointer array;
            if (!CopyArrayFully(attr.pointer, array)) { return false; }
            outputAttributes->AddAttribute(attr.type, attr.attachmentType, array);
        }
    }
    output->SetAttributeSet(outputAttributes);
    return true;
}

} // namespace

WarpByVectorFilter::WarpByVectorFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

// 按名字 + 挂载类型查找向量数组:优先点关联(与 GenerateIds 的双键匹配一致)
ArrayObject::Pointer WarpByVectorFilter::FindVectorArray(DataObject::Pointer input,
                                                         IGenum& attachment) const {
    auto attributes = input->GetAttributeSet();
    if (!attributes) { return nullptr; }

    const IGsize count = static_cast<IGsize>(attributes->GetNumberOfAttributes());
    const bool byName = !m_VectorName.empty();

    // 第一轮:名字匹配 + 点关联
    for (IGsize i = 0; i < count; ++i) {
        auto& attr = attributes->GetAttribute(i);
        if (attr.isDeleted || !attr.pointer) { continue; }
        if (!byName && attr.type != IG_VECTOR) { continue; }
        if (byName && attr.pointer->GetName() != m_VectorName) { continue; }
        if (attr.attachmentType == IG_POINT) {
            attachment = IG_POINT;
            return attr.pointer;
        }
    }
    // 第二轮:名字匹配(任意挂载类型)
    for (IGsize i = 0; i < count; ++i) {
        auto& attr = attributes->GetAttribute(i);
        if (attr.isDeleted || !attr.pointer) { continue; }
        if (!byName && attr.type != IG_VECTOR) { continue; }
        if (byName && attr.pointer->GetName() != m_VectorName) { continue; }
        attachment = attr.attachmentType;
        return attr.pointer;
    }
    // 名字未指定时(byName == false),上面两轮已按"任意向量数组"处理;
    // 若显式指定了名字却找不到,这里直接返回空,由调用方报错,不做静默回退。
    return nullptr;
}

bool WarpByVectorFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) { return false; }
    if (input->GetDataObjectType() == IG_NONE) { return true; }

    auto inputPoints = input->GetPoints();
    if (inputPoints == nullptr) {
        std::cerr << "[WarpByVector] 输入没有点数据,无法形变。" << std::endl;
        return false;
    }

    IGenum attachment = IG_POINT;
    auto vectorArray = FindVectorArray(input, attachment);
    if (vectorArray == nullptr) {
        std::cerr << "[WarpByVector] 未找到形变向量数组: " << m_VectorName << std::endl;
        return false;
    }
    if (attachment == IG_CELL) {
        std::cerr << "[WarpByVector] 形变向量需为点关联数据(当前为单元关联)。" << std::endl;
        return false;
    }

    const int dimension = vectorArray->GetDimension();
    if (dimension < 1) { return false; }

    const IGsize pointCount = inputPoints->GetNumberOfPoints();
    if (vectorArray->GetNumberOfElements() < pointCount) {
        std::cerr << "[WarpByVector] 向量元素数(" << vectorArray->GetNumberOfElements()
                  << ")少于点数(" << pointCount << ")。" << std::endl;
        return false;
    }

    // ---------------- 独立输出:按输入类型新建对象,深拷贝几何与属性 ----------------
    // 结构化网格保持原类型(转非结构化会产生巨量单元,128^3 这类网格尤其危险),
    // 只拷贝点集与维度信息,形变仅作用于点坐标。
    const bool structuredInput = input->GetDataObjectType() == IG_STRUCTURED_MESH;
    auto inputMesh =
            structuredInput ? nullptr : UnstructuredMesh::TransDataObjToUnstructuredMesh(input);
    if (!structuredInput && inputMesh == nullptr) {
        std::cerr << "[WarpByVector] 输入类型(" << input->GetDataObjectType()
                  << ")不支持形变。" << std::endl;
        return false;
    }

    DataObject::Pointer output =
            structuredInput
                    ? DataObject::CreateDataObject(IG_STRUCTURED_MESH)
                    : static_cast<DataObject::Pointer>(UnstructuredMesh::New());
    if (output == nullptr) { return false; }

    auto points = Points::New();
    if (!points->DeepCopy(inputPoints)) { return false; }
    if (auto pointSet = DynamicCast<PointSet>(output)) {
        pointSet->SetPoints(points);
    } else {
        return false;
    }

    if (structuredInput) {
        auto inStructured = DynamicCast<StructuredMesh>(input);
        auto outStructured = DynamicCast<StructuredMesh>(output);
        if (inStructured == nullptr || outStructured == nullptr) { return false; }
        outStructured->SetDimensionSize(inStructured->GetDimensionSize());
    } else if (auto inputCells = inputMesh->GetCells()) {
        auto outMesh = DynamicCast<UnstructuredMesh>(output);
        if (outMesh == nullptr) { return false; }

        auto cells = CellArray::New();
        if (!cells->DeepCopy(inputCells)) { return false; }

        const IGsize cellCount = cells->GetNumberOfCells();
        auto cellTypes = UnsignedIntArray::New();
        cellTypes->Resize(cellCount);
        for (IGsize i = 0; i < cellCount; ++i) {
            cellTypes->ValueAt(i) = static_cast<unsigned int>(inputMesh->GetCellType(i));
        }
        outMesh->SetCells(cells, cellTypes);
    }
    if (!CopyAttributeSet(input, output)) { return false; }

    // ---------------- 缩放系数(参考 Deformation 的 D_model 归一化) ----------------
    const BoundingBox& box = input->GetBoundingBox();
    const double dx = box.max[0] - box.min[0];
    const double dy = box.max[1] - box.min[1];
    const double dz = box.max[2] - box.min[2];
    const double diagonal = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (m_AutoScale && diagonal > 0.0) {
        double maxMagnitude = 0.0;
        for (IGsize i = 0; i < pointCount; ++i) {
            double sq = 0.0;
            for (int d = 0; d < dimension && d < 3; ++d) {
                const double v = vectorArray->GetElementValue(i, d);
                sq += v * v;
            }
            maxMagnitude = std::max(maxMagnitude, std::sqrt(sq));
        }
        m_AppliedScale = maxMagnitude > 0.0 ? diagonal / maxMagnitude : 1.0;
    } else {
        m_AppliedScale = m_Scale;
    }

    // 单点位移上限(相对模型对角线)
    const double maxDisplacement =
            (m_MaxRatio > 0.0 && diagonal > 0.0) ? m_MaxRatio * diagonal : 0.0;

    // ---------------- 逐点形变 ----------------
    for (IGsize i = 0; i < pointCount; ++i) {
        double offset[3] = {0.0, 0.0, 0.0};
        for (int d = 0; d < dimension && d < 3; ++d) {
            offset[d] = m_AppliedScale * vectorArray->GetElementValue(i, d);
        }

        if (maxDisplacement > 0.0) {
            const double magnitude =
                    std::sqrt(offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2]);
            if (magnitude > maxDisplacement) {
                const double clamp = maxDisplacement / magnitude;
                offset[0] *= clamp;
                offset[1] *= clamp;
                offset[2] *= clamp;
            }
        }

        const auto& p = inputPoints->GetPoint(i);
        points->SetPoint(i, p[0] + offset[0], p[1] + offset[1], p[2] + offset[2]);
    }

    // 点坐标改变:标记数据脏,GetBoundingBox() 会按需重算包围盒
    points->Modified();
    output->SetName(input->GetName());

    // 法线/绘制数据在重建可绘制数据时刷新
    if (auto draw = DynamicCast<DrawObject>(output)) { draw->ForceReConvertToDrawableData(); }

    SetOutput(output);

    std::cout << "[INFO] WarpByVector: 数组 '" << vectorArray->GetName() << "', 点数 " << pointCount
              << ", 缩放 " << m_AppliedScale << (m_AutoScale ? " (自动)" : "") << std::endl;
    return true;
}

IGAME_NAMESPACE_END
