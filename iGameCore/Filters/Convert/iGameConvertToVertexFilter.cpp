#include "iGameConvertToVertexFilter.h"

#include <iostream>

IGAME_NAMESPACE_BEGIN

namespace {

// 按输入数组的实际类型深拷贝，保留原类型、名称、维度与数值精度，
// 输出数组与原数组完全独立，互不影响。
// 返回 nullptr 表示未识别的数组类型。
ArrayObject::Pointer DeepCopyPointAttributeArray(const ArrayObject::Pointer& src) {
    if (src.IsNull()) return nullptr;
    switch (src->GetArrayType()) {
        case IG_FloatArray: {
            auto dst = FloatArray::New();
            dst->DeepCopy(DynamicCast<FloatArray>(src));
            return dst;
        }
        case IG_DoubleArray: {
            auto dst = DoubleArray::New();
            dst->DeepCopy(DynamicCast<DoubleArray>(src));
            return dst;
        }
        case IG_IntArray: {
            auto dst = IntArray::New();
            dst->DeepCopy(DynamicCast<IntArray>(src));
            return dst;
        }
        case IG_UnsignedIntArray: {
            auto dst = UnsignedIntArray::New();
            dst->DeepCopy(DynamicCast<UnsignedIntArray>(src));
            return dst;
        }
        case IG_CharArray: {
            auto dst = CharArray::New();
            dst->DeepCopy(DynamicCast<CharArray>(src));
            return dst;
        }
        case IG_UnsignedCharArray: {
            auto dst = UnsignedCharArray::New();
            dst->DeepCopy(DynamicCast<UnsignedCharArray>(src));
            return dst;
        }
        case IG_ShortArray: {
            auto dst = ShortArray::New();
            dst->DeepCopy(DynamicCast<ShortArray>(src));
            return dst;
        }
        case IG_UnsignedShortArray: {
            auto dst = UnsignedShortArray::New();
            dst->DeepCopy(DynamicCast<UnsignedShortArray>(src));
            return dst;
        }
        case IG_LongLongArray: {
            auto dst = LongLongArray::New();
            dst->DeepCopy(DynamicCast<LongLongArray>(src));
            return dst;
        }
        case IG_UnsignedLongLongArray: {
            auto dst = UnsignedLongLongArray::New();
            dst->DeepCopy(DynamicCast<UnsignedLongLongArray>(src));
            return dst;
        }
        default:
            return nullptr;
    }
}

} // namespace

ConvertToVertexFilter::ConvertToVertexFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool ConvertToVertexFilter::Execute() {
    auto in = DynamicCast<PointSet>(GetInput(0));
    if (in.IsNull()) return false;

    auto out = UnstructuredMesh::New();
    out->SetName(in->GetName());

    switch (m_ConvertMethod) {
        case IG_CONVERT_POINT_TO_VERTEX:
            if (!ExecutePointToVertex(in, out)) return false;
            break;
        default:
            return false;
    }

    out->Modified();
    SetOutput(0, out);
    return true;
}

bool ConvertToVertexFilter::ExecutePointToVertex(PointSet::Pointer in, UnstructuredMesh::Pointer out) {
    auto inPoints = in->GetPoints();
    auto inAttrs = in->GetAttributeSet();
    auto outPoints = out->GetPoints();
    auto outAttrs = out->GetAttributeSet();

    IGsize pointNum = in->GetNumberOfPoints();
    outPoints->Reset();
    for (IGsize i = 0; i < pointNum; ++i) {
        // 顶点单元不存坐标，只存点下标；点坐标统一放在输出 Points 数组，
        // 对应关系由下标 i 保证。
        outPoints->AddPoint(in->GetPoint(i));
        igIndex cell[1] = {static_cast<igIndex>(i)};
        out->AddCell(cell, 1, IG_VERTEX);
        if ((i & 0x3FF) == 0) { UpdateProgress(static_cast<double>(i) / pointNum); }
    }

    // 属性处理：点属性一一对应，按原数组实际类型深拷贝为输出点属性（IG_POINT）；
    // 顶点单元不复制点属性；原网格单元属性（IG_CELL）与输出顶点单元数量无法对应，丢弃。
    for (int i = 0; i < static_cast<int>(inAttrs->GetNumberOfAttributes()); ++i) {
        auto& attr = inAttrs->GetAttribute(i);
        if (attr.attachmentType != IG_POINT) continue;

        // 按实际类型深拷贝，保留 Double、整型及 64 位 ID 的精度与类型；
        // 输出数组独立于输入，后续修改输出不会影响输入。
        ArrayObject::Pointer pointArr = DeepCopyPointAttributeArray(attr.pointer);
        if (pointArr.IsNull()) {
            std::cerr << "[ConvertToVertexFilter] 无法识别的点属性数组类型，已跳过属性: "
                      << (attr.pointer ? attr.pointer->GetName() : "<null>") << std::endl;
            continue;
        }
        outAttrs->AddAttribute(attr.type, IG_POINT, pointArr);
    }


    UpdateProgress(1.0);
    return true;
}

IGAME_NAMESPACE_END
