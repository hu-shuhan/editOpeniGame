#include "iGameReverseSenseFilter.h"

#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGamePoints.h"
#include "iGameSurfaceMesh.h"

#include <vector>
#include <limits>
#include <type_traits>

IGAME_NAMESPACE_BEGIN

namespace {

// Copy through the native value type: converting integer IDs through double loses precision.
template<class Array>
ArrayObject::Pointer CopyTypedAttribute(ArrayObject* source, bool negate) {
    auto input = DynamicCast<Array>(source);
    if (!input) return nullptr;
    auto output = Array::New();
    output->DeepCopy(input);
    if (negate) {
        using Value = std::remove_reference_t<decltype(output->RawPointer()[0])>;
        for (IGsize i = 0; i < output->GetNumberOfValues(); ++i) {
            Value& value = output->RawPointer()[i];
            if constexpr (std::is_integral_v<Value>) {
                if constexpr (std::is_unsigned_v<Value>) {
                    if (value != 0) return nullptr;
                } else {
                    if (value == std::numeric_limits<Value>::lowest()) return nullptr;
                }
            }
            value = -value;
        }
    }
    return output;
}

ArrayObject::Pointer CopyAttribute(ArrayObject* input, bool negate) {
    switch (input->GetArrayType()) {
        case IG_DoubleArray: return CopyTypedAttribute<DoubleArray>(input, negate);
        case IG_IntArray: return CopyTypedAttribute<IntArray>(input, negate);
        case IG_INTARRAY: return CopyTypedAttribute<IntArray>(input, negate);
        case IG_UnsignedIntArray: return CopyTypedAttribute<UnsignedIntArray>(input, negate);
        case IG_CharArray: return CopyTypedAttribute<CharArray>(input, negate);
        case IG_UnsignedCharArray: return CopyTypedAttribute<UnsignedCharArray>(input, negate);
        case IG_ShortArray: return CopyTypedAttribute<ShortArray>(input, negate);
        case IG_UnsignedShortArray: return CopyTypedAttribute<UnsignedShortArray>(input, negate);
        case IG_LongLongArray: return CopyTypedAttribute<LongLongArray>(input, negate);
        case IG_UnsignedLongLongArray: return CopyTypedAttribute<UnsignedLongLongArray>(input, negate);
        case IG_FloatArray: return CopyTypedAttribute<FloatArray>(input, negate);
        default: return nullptr;
    }
}

} // namespace

ReverseSenseFilter::ReverseSenseFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool ReverseSenseFilter::Execute() {
    m_Message.clear();
    SetOutput(nullptr);

    auto input = DynamicCast<SurfaceMesh>(GetInput(0));
    if (input == nullptr) {
        m_Message = "ReverseSenseFilter only supports SurfaceMesh input";
        return false;
    }
    const IGsize pointNum = input->GetNumberOfPoints();
    const IGsize faceNum = input->GetNumberOfFaces();
    if (pointNum == 0 || faceNum == 0) {
        m_Message = "Input surface mesh is empty";
        return false;
    }

    auto output = SurfaceMesh::New();
    output->SetName(input->GetName());

    // 点集：独立拷贝，几何本身保持不变。
    auto outPoints = Points::New();
    outPoints->DeepCopy(input->GetPoints());
    output->SetPoints(outPoints);

    // 面：逐面写入新 CellArray；ReverseCells 时反转顶点环序。
    // 边与面-邻接关系由面环序推导，因此反转后统一重建即可保持一致。
    auto inFaces = input->GetFaces();
    auto outFaces = CellArray::New();
    std::vector<igIndex> ids;
    for (IGsize faceId = 0; faceId < faceNum; ++faceId) {
        const igIndex* cell = nullptr;
        const int n = inFaces->GetCellIds(faceId, cell);
        if (n <= 0) { continue; }
        ids.resize(n);
        if (m_ReverseCells) {
            for (int i = 0; i < n; ++i) { ids[i] = cell[n - 1 - i]; }
        } else {
            for (int i = 0; i < n; ++i) { ids[i] = cell[i]; }
        }
        outFaces->AddCellIds(ids.data(), n);
        UpdateProgress(0.5 * static_cast<double>(faceId + 1) / faceNum);
    }
    output->SetFaces(outFaces);

    // 属性：全部按原类型搬运；ReverseNormals 时把 IG_NORMAL 逐分量取反。
    auto inAttrs = input->GetAttributeSet();
    auto outAttrs = AttributeSet::New();
    if (inAttrs != nullptr) {
        auto all = inAttrs->GetAllAttributes();
        if (all) {
            const IGsize attrNum = all->GetNumberOfElements();
            for (IGsize a = 0; a < attrNum; ++a) {
                auto& attr = all->GetElement(a);
                if (attr.isDeleted || attr.pointer.IsNull()) { continue; }

                auto inArray = attr.pointer;
                const bool negate = m_ReverseNormals && attr.type == IG_NORMAL;
                auto outArray = CopyAttribute(inArray, negate);
                if (!outArray) {
                    m_Message = "Unsupported attribute type or normal value cannot be negated: " + inArray->GetName();
                    return false;
                }

                const IGsize index =
                    outAttrs->AddAttribute(attr.type, attr.attachmentType, outArray);
                if (index != static_cast<IGsize>(-1)) {
                    outAttrs->GetAttribute(index).UpdateAllDataRange();
                }
            }
        }
    }
    output->SetAttributeSet(outAttrs);

    // 面环序已改变：重建边与点→面/边→面邻接，使下游（选择/连通性）拿到的面连接关系与新环序一致。
    output->BuildEdges();
    output->BuildFaceEdgeLinks();
    output->BuildFaceLinks();

    UpdateProgress(1.0);
    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
