#include "iGameGenerateIdsFilter.h"

#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameDrawObject.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>
#include <iGameUnstructuredMesh.h>

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

// 按实际存储类型整表拷贝(逐元素类型化直拷,不经 double)
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

// 拷贝整个属性集合到输出对象,保留数组类型、分量数、属性种类与挂载类型。
// 注意:框架的 AttributeSet::Attribute::DeepCopy() 目前只支持 float/double 数组,
// 遇到 LongLongArray 等类型会静默丢成空指针,所以这里自行按类型分发拷贝,
// 保证连续调用本滤波器(或 Id 数组继续向下游传递)时不会丢失属性。
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

iGameGenerateIdsFilter::iGameGenerateIdsFilter(IGenum dataType) {
    m_DataType = dataType;
    m_ArrayName = "Ids";
    m_StartId = 0;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

// 在输入对象之外构建独立输出:深拷贝几何与属性,避免像以前那样直接修改原模型。
DataObject::Pointer iGameGenerateIdsFilter::BuildIndependentOutput(DataObject::Pointer input) {
    // 非结构化网格(包含由面网格/体网格转换而来的情况):拷贝点、单元与单元类型
    if (auto inputMesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(input)) {
        auto output = UnstructuredMesh::New();

        auto points = Points::New();
        if (!points->DeepCopy(inputMesh->GetPoints())) { return nullptr; }
        output->SetPoints(points);

        if (auto inputCells = inputMesh->GetCells()) {
            auto cells = CellArray::New();
            if (!cells->DeepCopy(inputCells)) { return nullptr; }

            const IGsize cellCount = cells->GetNumberOfCells();
            auto cellTypes = UnsignedIntArray::New();
            cellTypes->Resize(cellCount);
            for (IGsize i = 0; i < cellCount; ++i) {
                // 保持与输入完全一致的单元顺序与类型,Id 才能与源数据一一对应
                cellTypes->ValueAt(i) = static_cast<unsigned int>(inputMesh->GetCellType(i));
            }
            output->SetCells(cells, cellTypes);
        }

        if (!CopyAttributeSet(input, output)) { return nullptr; }
        output->SetName(input->GetName());
        return output;
    }

    // 其它类型(例如点集):按原类型新建对象,仅拷贝点与属性
    auto output = DataObject::CreateDataObject(input->GetDataObjectType());
    if (output == nullptr) { return nullptr; }

    if (auto inputPoints = input->GetPoints()) {
        if (auto pointSet = DynamicCast<PointSet>(output)) {
            auto points = Points::New();
            if (!points->DeepCopy(inputPoints)) { return nullptr; }
            pointSet->SetPoints(points);
        }
    }

    if (!CopyAttributeSet(input, output)) { return nullptr; }
    output->SetName(input->GetName());
    return output;
}

bool iGameGenerateIdsFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) { return false; }

    if (m_DataType != IG_POINT && m_DataType != IG_CELL) { return false; }

    auto output = BuildIndependentOutput(input);
    if (output == nullptr) { return false; }

    SetOutput(output);
    return Run();
}

bool iGameGenerateIdsFilter::Run() {
    auto output = GetOutput();
    IGsize count = 0;
    if (m_DataType == IG_POINT) {
        auto points = output->GetPoints();
        if (points == nullptr) { return false; }
        count = points->GetNumberOfPoints();
    } else {
        auto cells = output->GetCellArray();
        if (cells == nullptr) { return false; }
        count = cells->GetNumberOfCells();
    }

    if (count == 0) { return true; }

    // IDs are 64-bit integers: store them in LongLongArray and write them
    // directly through the typed accessor. Round-tripping through double
    // (SetValue) would lose precision beyond 2^53.
    LongLongArray::Pointer arr = LongLongArray::New();
    arr->SetName(m_ArrayName);
    arr->SetDimension(1);
    arr->Resize(count);

    for (IGsize i = 0; i < count; ++i) {
        arr->ValueAt(i) = m_StartId + static_cast<long long>(i);
    }

    auto attrs = output->GetAttributeSet();
    if (attrs == nullptr) {
        output->SetAttributeSet(AttributeSet::New());
        attrs = output->GetAttributeSet();
    }

    // Overwrite must match BOTH the array name and the attachment type
    // (IG_POINT / IG_CELL). Looking up by name alone would either keep adding
    // duplicate attributes or replace the attribute attached to the other type.
    int existing = -1;
    for (IGsize i = 0; i < static_cast<IGsize>(attrs->GetNumberOfAttributes()); ++i) {
        auto& attr = attrs->GetAttribute(i);
        if (attr.pointer && attr.attachmentType == m_DataType &&
            attr.pointer->GetName() == m_ArrayName) {
            existing = static_cast<int>(i);
            break;
        }
    }

    if (existing >= 0) {
        auto& attr = attrs->GetAttribute(existing);
        attr.pointer = arr;
        attr.UpdateAllDataRange();
    } else {
        attrs->AddAttribute(IG_SCALAR, m_DataType, arr);
    }

    std::cout << "[INFO] Added array '" << m_ArrayName << "' with " << count << " elements to "
              << (m_DataType == IG_POINT ? "Point" : "Cell") << " data." << std::endl;
    std::cout << "[INFO] Total attributes now: " << attrs->GetNumberOfAttributes() << std::endl;

    // 输出是尚未加入场景的新对象,这里只标记需要重映射绘制数据
    if (auto draw = DynamicCast<DrawObject>(output)) { draw->ForceReConvertToDrawableData(); }
    return true;
}

IGAME_NAMESPACE_END
