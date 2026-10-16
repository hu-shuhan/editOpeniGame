#include "iGameAxisAlignedReflectionFilter.h"

#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGamePoints.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <array>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

bool ResolvePlane(AxisAlignedReflectionFilter::Plane plane,
                  const BoundingBox& bounds,
                  double customCenter,
                  int& axis,
                  double& center) {
    switch (plane) {
    case AxisAlignedReflectionFilter::Plane::XMin:
        axis = 0; center = bounds.min[0]; return true;
    case AxisAlignedReflectionFilter::Plane::YMin:
        axis = 1; center = bounds.min[1]; return true;
    case AxisAlignedReflectionFilter::Plane::ZMin:
        axis = 2; center = bounds.min[2]; return true;
    case AxisAlignedReflectionFilter::Plane::XMax:
        axis = 0; center = bounds.max[0]; return true;
    case AxisAlignedReflectionFilter::Plane::YMax:
        axis = 1; center = bounds.max[1]; return true;
    case AxisAlignedReflectionFilter::Plane::ZMax:
        axis = 2; center = bounds.max[2]; return true;
    case AxisAlignedReflectionFilter::Plane::X:
        axis = 0; center = customCenter; return true;
    case AxisAlignedReflectionFilter::Plane::Y:
        axis = 1; center = customCenter; return true;
    case AxisAlignedReflectionFilter::Plane::Z:
        axis = 2; center = customCenter; return true;
    }
    return false;
}

bool ReflectCell(const igIndex* inputIds,
                 int size,
                 IGenum type,
                 igIndex pointOffset,
                 std::vector<igIndex>& outputIds) {
    outputIds.resize(size);

    switch (type) {
    case IG_VERTEX:
    case IG_LINE:
    case IG_TRIANGLE:
    case IG_QUAD:
    case IG_POLYGON:
    case IG_FACE:
        for (int i = 0; i < size; ++i) {
            outputIds[(size - i) % size] = inputIds[i] + pointOffset;
        }
        return true;

    case IG_POLY_LINE:
        for (int i = 0; i < size; ++i) {
            outputIds[i] = inputIds[size - i - 1] + pointOffset;
        }
        return true;

    case IG_TETRA:
        if (size != 4) return false;
        outputIds = {
            inputIds[3] + pointOffset,
            inputIds[1] + pointOffset,
            inputIds[2] + pointOffset,
            inputIds[0] + pointOffset
        };
        return true;

    case IG_HEXAHEDRON:
        if (size != 8) return false;
        outputIds = {
            inputIds[4] + pointOffset,
            inputIds[5] + pointOffset,
            inputIds[6] + pointOffset,
            inputIds[7] + pointOffset,
            inputIds[0] + pointOffset,
            inputIds[1] + pointOffset,
            inputIds[2] + pointOffset,
            inputIds[3] + pointOffset
        };
        return true;

    case IG_PRISM:
        if (size != 6) return false;
        outputIds = {
            inputIds[3] + pointOffset,
            inputIds[4] + pointOffset,
            inputIds[5] + pointOffset,
            inputIds[0] + pointOffset,
            inputIds[1] + pointOffset,
            inputIds[2] + pointOffset
        };
        return true;

    case IG_PYRAMID:
        if (size != 5) return false;
        outputIds = {
            inputIds[3] + pointOffset,
            inputIds[2] + pointOffset,
            inputIds[1] + pointOffset,
            inputIds[0] + pointOffset,
            inputIds[4] + pointOffset
        };
        return true;

    default:
        return false;
    }
}

std::array<int, 9> MirrorFactors(int axis, int dimension) {
    std::array<int, 9> factors{};
    factors.fill(1);

    int sign[3]{1, 1, 1};
    sign[axis] = -1;

    if (dimension == 3) {
        factors[axis] = -1;
    } else if (dimension == 6) {
        factors[0] = 1;
        factors[1] = 1;
        factors[2] = 1;
        factors[3] = sign[0] * sign[1];
        factors[4] = sign[1] * sign[2];
        factors[5] = sign[0] * sign[2];
    } else if (dimension == 9) {
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                factors[row * 3 + col] = sign[row] * sign[col];
            }
        }
    }

    return factors;
}

bool ShouldReflect(const AttributeSet::Attribute& attr,
                   bool signedArray,
                   bool flipAllInputArrays) {
    if (!attr.pointer || !signedArray) return false;

    const int dimension = attr.pointer->GetDimension();
    if (flipAllInputArrays) {
        return dimension == 3 || dimension == 6 || dimension == 9;
    }

    if (attr.type == IG_VECTOR || attr.type == IG_NORMAL) {
        return dimension == 3;
    }
    if (attr.type == IG_TENSOR) {
        return dimension == 6 || dimension == 9;
    }
    return false;
}

template<typename TArray>
ArrayObject::Pointer CopyAttributeArray(typename TArray::Pointer input,
                                        bool copyInput,
                                        bool reflect,
                                        int axis) {
    if (!input) return nullptr;

    auto output = TArray::New();
    output->SetName(input->GetName());
    output->SetDimension(input->GetDimension());

    const IGsize tupleCount = input->GetNumberOfElements();
    const int dimension = input->GetDimension();
    output->Resize(copyInput ? tupleCount * 2 : tupleCount);

    if (copyInput && input->GetNumberOfValues() > 0) {
        std::copy(input->RawPointer(),
                  input->RawPointer() + input->GetNumberOfValues(),
                  output->RawPointer());
    }

    const auto factors = MirrorFactors(axis, dimension);
    const IGsize outputOffset = copyInput ? tupleCount : 0;

    for (IGsize i = 0; i < tupleCount; ++i) {
        const auto* src = input->RawPointer(i);
        auto* dst = output->RawPointer(outputOffset + i);
        for (int component = 0; component < dimension; ++component) {
            dst[component] = reflect
                ? src[component] * factors[component]
                : src[component];
        }
    }

    return output;
}

ArrayObject::Pointer CopyAttribute(const AttributeSet::Attribute& attr,
                                   bool copyInput,
                                   bool flipAllInputArrays,
                                   int axis) {
    auto copySigned = [&](auto array) -> ArrayObject::Pointer {
        const bool reflect = ShouldReflect(attr, true, flipAllInputArrays);
        using ArrayType = typename decltype(array)::ObjectType;
        return CopyAttributeArray<ArrayType>(array, copyInput, reflect, axis);
    };

    auto copyUnsigned = [&](auto array) -> ArrayObject::Pointer {
        using ArrayType = typename decltype(array)::ObjectType;
        return CopyAttributeArray<ArrayType>(array, copyInput, false, axis);
    };

    if (auto array = DynamicCast<FloatArray>(attr.pointer)) return copySigned(array);
    if (auto array = DynamicCast<DoubleArray>(attr.pointer)) return copySigned(array);
    if (auto array = DynamicCast<IntArray>(attr.pointer)) return copySigned(array);
    if (auto array = DynamicCast<ShortArray>(attr.pointer)) return copySigned(array);
    if (auto array = DynamicCast<CharArray>(attr.pointer)) return copySigned(array);
    if (auto array = DynamicCast<LongLongArray>(attr.pointer)) return copySigned(array);

    if (auto array = DynamicCast<UnsignedIntArray>(attr.pointer)) return copyUnsigned(array);
    if (auto array = DynamicCast<UnsignedShortArray>(attr.pointer)) return copyUnsigned(array);
    if (auto array = DynamicCast<UnsignedCharArray>(attr.pointer)) return copyUnsigned(array);
    if (auto array = DynamicCast<UnsignedLongLongArray>(attr.pointer)) return copyUnsigned(array);

    return nullptr;
}

bool CopyAttributes(UnstructuredMesh::Pointer input,
                    UnstructuredMesh::Pointer output,
                    bool copyInput,
                    bool flipAllInputArrays,
                    int axis) {
    auto inputAttributes = input->GetAttributeSet();
    auto outputAttributes = AttributeSet::New();

    if (!inputAttributes) {
        output->SetAttributeSet(outputAttributes);
        return true;
    }

    auto attributes = inputAttributes->GetAllAttributes();
    for (IGsize i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto& attr = attributes->GetElement(i);
        if (attr.IsNone()) continue;

        auto copiedArray = CopyAttribute(attr, copyInput, flipAllInputArrays, axis);
        if (!copiedArray) return false;

        const IGsize index = outputAttributes->AddAttribute(
            attr.type, attr.attachmentType, copiedArray);
        if (index == static_cast<IGsize>(-1)) return false;
        outputAttributes->GetAttribute(index).UpdateAllDataRange();
    }

    output->SetAttributeSet(outputAttributes);
    return true;
}

} // namespace

AxisAlignedReflectionFilter::AxisAlignedReflectionFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool AxisAlignedReflectionFilter::Execute() {
    auto input = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (!input || !input->GetPoints() || input->GetNumberOfPoints() == 0 ||
        !input->GetCells() || !input->GetCellTypes()) {
        return false;
    }

    int axis = 0;
    double center = 0.0;
    if (!ResolvePlane(m_Plane, input->GetBoundingBox(), m_Center, axis, center)) {
        return false;
    }

    const IGsize inputPointCount = input->GetNumberOfPoints();
    const IGsize inputCellCount = input->GetNumberOfCells();

    auto output = UnstructuredMesh::New();
    auto outputPoints = Points::New();

    if (m_CopyInput) {
        outputPoints->DeepCopy(input->GetPoints());
    }

    for (IGsize i = 0; i < inputPointCount; ++i) {
        Point point = input->GetPoint(i);
        point[axis] = static_cast<float>(2.0 * center - point[axis]);
        outputPoints->AddPoint(point);
    }
    output->SetPoints(outputPoints);

    auto outputCells = CellArray::New();
    auto outputTypes = UnsignedIntArray::New();

    // 新建单元数组，避免复制输入单元的偏移数据
    if (m_CopyInput) {
        for (IGsize cellId = 0; cellId < inputCellCount; ++cellId) {
            const igIndex* inputIds = nullptr;
            const int size = input->GetCells()->GetCellIds(cellId, inputIds);
            outputCells->AddCellIds(inputIds, size);
            outputTypes->AddValue(input->GetCellType(cellId));
        }
    }

    const igIndex pointOffset = m_CopyInput
        ? static_cast<igIndex>(inputPointCount)
        : 0;

    std::vector<igIndex> reflectedIds;
    for (IGsize cellId = 0; cellId < inputCellCount; ++cellId) {
        const igIndex* inputIds = nullptr;
        const int size = input->GetCells()->GetCellIds(cellId, inputIds);
        const IGenum type = input->GetCellType(cellId);

        if (!ReflectCell(inputIds, size, type, pointOffset, reflectedIds)) {
            return false;
        }

        outputCells->AddCellIds(reflectedIds.data(), size);
        outputTypes->AddValue(type);
    }

    output->SetCells(outputCells, outputTypes);

    if (!CopyAttributes(input, output, m_CopyInput, m_FlipAllInputArrays, axis)) {
        return false;
    }

    SetOutput(0, output);
    return true;
}

IGAME_NAMESPACE_END
