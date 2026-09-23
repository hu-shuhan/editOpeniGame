#include "iGameRemoveGhostInformationFilter.h"

#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGamePoints.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace
{

// 判断属性名是不是 vtkGhostType.因此统一转小写比较。
bool IsGhostAttributeName(const std::string& name) {

    std::string lower = name;

    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return lower == "vtkghosttype";
}


// 判断Cell是否应该被Remove
bool ShouldRemoveGhostCell(unsigned char ghostValue) {

    constexpr unsigned char DUPLICATECELL = 1;
    constexpr unsigned char HIDDENCELL = 32;

    constexpr unsigned char removeMask = DUPLICATECELL | HIDDENCELL;

    return (ghostValue & removeMask) != 0;
}


// 按Point映射复制属性
template<typename ArrayType>
ArrayObject::Pointer CopyPointAttribute(ArrayObject::Pointer inputArray, const std::vector<igIndex>& pointLookup,
                                        IGsize outputPointCount) {

    auto typedInput = DynamicCast<ArrayType>(inputArray);

    if (typedInput.IsNull()) { return nullptr; }

    auto outputArray = ArrayType::New();

    outputArray->SetName(inputArray->GetName());

    const int dimension = inputArray->GetDimension();

    outputArray->SetDimension(dimension);

    outputArray->Resize(outputPointCount);

    if (outputPointCount == 0 || dimension <= 0) { return outputArray; }

    const auto* inputValues = typedInput->RawPointer();
    auto* outputValues = outputArray->RawPointer();

    if (inputValues == nullptr || outputValues == nullptr) { return nullptr; }

    for (IGsize oldPointId = 0; oldPointId < static_cast<IGsize>(pointLookup.size()); ++oldPointId) {

        const igIndex newPointId = pointLookup[static_cast<size_t>(oldPointId)];

        if (newPointId < 0) { continue; }

        const size_t inputOffset = static_cast<size_t>(oldPointId) * static_cast<size_t>(dimension);
        const size_t outputOffset = static_cast<size_t>(newPointId) * static_cast<size_t>(dimension);

        std::copy_n(inputValues + inputOffset, dimension, outputValues + outputOffset);
    }

    return outputArray;
}

// 按Cell映射复制属性
template<typename ArrayType>
ArrayObject::Pointer CopyCellAttribute(ArrayObject::Pointer inputArray, const std::vector<igIndex>& originCells) {

    auto typedInput = DynamicCast<ArrayType>(inputArray);

    if (typedInput.IsNull()) { return nullptr; }

    auto outputArray = ArrayType::New();

    outputArray->SetName(inputArray->GetName());

    const int dimension = inputArray->GetDimension();

    outputArray->SetDimension(dimension);

    outputArray->Resize(static_cast<IGsize>(originCells.size()));

    if (originCells.empty() || dimension <= 0) { return outputArray; }

    const auto* inputValues = typedInput->RawPointer();
    auto* outputValues = outputArray->RawPointer();

    if (inputValues == nullptr || outputValues == nullptr) { return nullptr; }

    for (IGsize newCellId = 0; newCellId < static_cast<IGsize>(originCells.size()); ++newCellId) {

        const IGsize oldCellId = static_cast<IGsize>(originCells[static_cast<size_t>(newCellId)]);

        const size_t inputOffset = static_cast<size_t>(oldCellId) * static_cast<size_t>(dimension);
        const size_t outputOffset = static_cast<size_t>(newCellId) * static_cast<size_t>(dimension);

        std::copy_n(inputValues + inputOffset, dimension, outputValues + outputOffset);
    }

    return outputArray;
}

// 根据原数组类型复制Point Attribute
ArrayObject::Pointer CopyPointAttributeByType(ArrayObject::Pointer inputArray, const std::vector<igIndex>& pointLookup,
                                              IGsize outputPointCount) {

    if (inputArray == nullptr) { return nullptr; }

    switch (inputArray->GetArrayType()) {

        case IG_FloatArray:
            return CopyPointAttribute<FloatArray>(inputArray, pointLookup, outputPointCount);

        case IG_DoubleArray:
            return CopyPointAttribute<DoubleArray>(inputArray, pointLookup, outputPointCount);

        case IG_IntArray:
            return CopyPointAttribute<IntArray>(inputArray, pointLookup, outputPointCount);

        case IG_UnsignedIntArray:
            return CopyPointAttribute<UnsignedIntArray>(inputArray, pointLookup, outputPointCount);

        case IG_CharArray:
            return CopyPointAttribute<CharArray>(inputArray, pointLookup, outputPointCount);

        case IG_UnsignedCharArray:
            return CopyPointAttribute<UnsignedCharArray>(inputArray, pointLookup, outputPointCount);

        case IG_ShortArray:
            return CopyPointAttribute<ShortArray>(inputArray, pointLookup, outputPointCount);

        case IG_UnsignedShortArray:
            return CopyPointAttribute<UnsignedShortArray>(inputArray, pointLookup, outputPointCount);

        case IG_LongLongArray:
            return CopyPointAttribute<LongLongArray>(inputArray, pointLookup, outputPointCount);

        case IG_UnsignedLongLongArray:
            return CopyPointAttribute<UnsignedLongLongArray>(inputArray, pointLookup, outputPointCount);

        default:
            return nullptr;
    }
}

// 根据原数组类型复制Cell Attribute
ArrayObject::Pointer CopyCellAttributeByType(ArrayObject::Pointer inputArray, const std::vector<igIndex>& originCells) {

    if (inputArray == nullptr) { return nullptr; }

    switch (inputArray->GetArrayType()) {

        case IG_FloatArray:
            return CopyCellAttribute<FloatArray>(inputArray, originCells);

        case IG_DoubleArray:
            return CopyCellAttribute<DoubleArray>(inputArray, originCells);

        case IG_IntArray:
            return CopyCellAttribute<IntArray>(inputArray, originCells);

        case IG_UnsignedIntArray:
            return CopyCellAttribute<UnsignedIntArray>(inputArray, originCells);

        case IG_CharArray:
            return CopyCellAttribute<CharArray>(inputArray, originCells);

        case IG_UnsignedCharArray:
            return CopyCellAttribute<UnsignedCharArray>(inputArray, originCells);

        case IG_ShortArray:
            return CopyCellAttribute<ShortArray>(inputArray, originCells);

        case IG_UnsignedShortArray:
            return CopyCellAttribute<UnsignedShortArray>(inputArray, originCells);

        case IG_LongLongArray:
            return CopyCellAttribute<LongLongArray>(inputArray, originCells);

        case IG_UnsignedLongLongArray:
            return CopyCellAttribute<UnsignedLongLongArray>(inputArray, originCells);

        default:
            return nullptr;
    }
}

} // namespace

RemoveGhostInformationFilter::RemoveGhostInformationFilter() {

    SetNumberOfInputs(1);

    SetNumberOfOutputs(1);
}

bool RemoveGhostInformationFilter::Execute() {

    m_WasModified = false;

    auto inputObject = GetInput(0);

    if (inputObject.IsNull()) { return false; }

    // 当前版本处理UnstructuredMesh
    auto input = DynamicCast<UnstructuredMesh>(inputObject);

    if (input.IsNull()) { return false; }


    const IGsize numberOfPoints = input->GetNumberOfPoints();

    const IGsize numberOfCells = input->GetNumberOfCells();


    auto inputAttributes = input->GetAttributeSet();

    if (inputAttributes == nullptr) { return true; }


    // 查找Point和Cell级vtkGhostType
    ArrayObject::Pointer pointGhostArray = nullptr;

    ArrayObject::Pointer cellGhostArray = nullptr;


    const IGsize numberOfAttributes = static_cast<IGsize>(inputAttributes->GetNumberOfAttributes());


    for (IGsize i = 0; i < numberOfAttributes; ++i) {

        auto& attr = inputAttributes->GetAttribute(i);


        if (attr.isDeleted || attr.pointer == nullptr) { continue; }


        if (!IsGhostAttributeName(attr.pointer->GetName())) { continue; }


        if (attr.attachmentType == IG_POINT && pointGhostArray == nullptr) {

            pointGhostArray = attr.pointer;
        }

        else if (attr.attachmentType == IG_CELL && cellGhostArray == nullptr) {

            cellGhostArray = attr.pointer;
        }
    }


    if (pointGhostArray == nullptr && cellGhostArray == nullptr) { return true; }


    // Ghost数组长度检查
    if (pointGhostArray != nullptr && pointGhostArray->GetNumberOfElements() < numberOfPoints) { return false; }

    if (cellGhostArray != nullptr && cellGhostArray->GetNumberOfElements() < numberOfCells) { return false; }


    // 遍历Cell，确定需要保留的Cell
    std::vector<igIndex> originCells;

    originCells.reserve(static_cast<size_t>(numberOfCells));

    std::vector<unsigned char> usedPoints(static_cast<size_t>(numberOfPoints), 0);


    igIndex ids[IGAME_CELL_MAX_SIZE]{};


    for (IGsize cellId = 0; cellId < numberOfCells; ++cellId) {

        if (cellGhostArray != nullptr) {

            const unsigned char ghostValue = static_cast<unsigned char>(cellGhostArray->GetValue(cellId));


            // Ghost Cell直接跳过
            if (ShouldRemoveGhostCell(ghostValue)) { continue; }
        }


        // 记录原Cell ID
        originCells.push_back(static_cast<igIndex>(cellId));


        if (cellGhostArray == nullptr) { continue; }


        // 标记该Cell使用到的Points
        const int pointCount = input->GetCellPointIds(cellId, ids);


        for (int j = 0; j < pointCount; ++j) {

            const igIndex pointId = ids[j];


            if (pointId >= 0 && pointId < static_cast<igIndex>(numberOfPoints)) {

                usedPoints[static_cast<size_t>(pointId)] = 1;
            }
        }
    }


    // 建立 oldPointId -> newPointId
    std::vector<igIndex> pointLookup(static_cast<size_t>(numberOfPoints), -1);


    IGsize outputPointCount = 0;


    if (cellGhostArray == nullptr) {

        outputPointCount = numberOfPoints;


        for (IGsize oldPointId = 0; oldPointId < numberOfPoints; ++oldPointId) {

            pointLookup[static_cast<size_t>(oldPointId)] = static_cast<igIndex>(oldPointId);
        }
    }

    else {

        for (IGsize oldPointId = 0; oldPointId < numberOfPoints; ++oldPointId) {

            if (!usedPoints[static_cast<size_t>(oldPointId)]) { continue; }


            pointLookup[static_cast<size_t>(oldPointId)] = static_cast<igIndex>(outputPointCount);


            ++outputPointCount;
        }
    }


    const IGsize outputCellCount = static_cast<IGsize>(originCells.size());

    // 创建新的Output Mesh
    auto output = UnstructuredMesh::New();

    output->SetName(input->GetName());


    // 创建新的Points
    auto outputPoints = Points::New();

    outputPoints->Resize(outputPointCount);


    for (IGsize oldPointId = 0; oldPointId < numberOfPoints; ++oldPointId) {

        const igIndex newPointId = pointLookup[static_cast<size_t>(oldPointId)];


        if (newPointId < 0) { continue; }


        outputPoints->SetPoint(static_cast<IGsize>(newPointId), input->GetPoint(oldPointId));
    }


    // 重建Cells
    auto outputCells = CellArray::New();


    auto outputTypes = UnsignedIntArray::New();


    for (IGsize newCellId = 0; newCellId < outputCellCount; ++newCellId) {

        const IGsize oldCellId = static_cast<IGsize>(originCells[static_cast<size_t>(newCellId)]);


        const int pointCount = input->GetCellPointIds(oldCellId, ids);


        igIndex newIds[IGAME_CELL_MAX_SIZE]{};


        for (int j = 0; j < pointCount; ++j) {

            const igIndex oldPointId = ids[j];


            if (oldPointId < 0 || oldPointId >= static_cast<igIndex>(numberOfPoints)) { return false; }


            const igIndex newPointId = pointLookup[static_cast<size_t>(oldPointId)];


            if (newPointId < 0) { return false; }


            newIds[j] = newPointId;
        }


        outputCells->AddCellIds(newIds, pointCount);


        outputTypes->AddValue(input->GetCellType(oldCellId));
    }


    // 设置Points和Cells
    output->SetPoints(outputPoints);

    output->SetCells(outputCells, outputTypes);


    // 创建新的AttributeSet
    auto outputAttributes = AttributeSet::New();


    // 复制普通属性
    for (IGsize i = 0; i < numberOfAttributes; ++i) {

        auto& attr = inputAttributes->GetAttribute(i);


        if (attr.isDeleted || attr.pointer == nullptr) { continue; }


        if (IsGhostAttributeName(attr.pointer->GetName())) { continue; }


        ArrayObject::Pointer outputArray = nullptr;


        if (attr.attachmentType == IG_POINT) {

            outputArray = CopyPointAttributeByType(attr.pointer, pointLookup, outputPointCount);
        }

        else if (attr.attachmentType == IG_CELL) {

            outputArray = CopyCellAttributeByType(attr.pointer, originCells);
        }

        else {

            continue;
        }


        if (outputArray == nullptr) { continue; }


        outputAttributes->AddAttribute(attr.type, attr.attachmentType, outputArray);
    }


    // 设置AttributeSet
    output->SetAttributeSet(outputAttributes);

    // 设置输出
    SetOutput(output);


    m_WasModified = true;


    return true;
}

IGAME_NAMESPACE_END
