#ifndef iGameWarpSupport_h
#define iGameWarpSupport_h

#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <string>

IGAME_NAMESPACE_BEGIN

namespace WarpSupport {

inline CellArray::Pointer CloneCellArray(CellArray::Pointer src) {
    if (src == nullptr) { return nullptr; }
    auto dst = CellArray::New();
    dst->Reset(); // 清掉构造函数预置的 m_Offsets = {0}
    if (!dst->DeepCopy(src)) { return nullptr; }
    return dst;
}


inline void CopyAttributes(DataObject::Pointer input, DataObject::Pointer output) {
    if (!input || !output) { return; }
    auto src = input->GetAttributeSet();
    if (src == nullptr) { return; }

    auto dst = AttributeSet::New();
    dst->DeepCopy(src);
    output->SetAttributeSet(dst);
}


inline PointSet::Pointer CreateOutputCopy(DataObject::Pointer dataObject) {
    if (!dataObject) { return nullptr; }

    switch (dataObject->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto input = DynamicCast<SurfaceMesh>(dataObject);
            if (!input) { return nullptr; }
            auto output = SurfaceMesh::New();
            output->SetName(input->GetName());
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(input->GetPoints())) { return nullptr; }
            output->SetPoints(newPoints);
            auto newFaces = CloneCellArray(input->GetFaces());
            if (newFaces) { output->SetFaces(newFaces); }
            CopyAttributes(dataObject, output);
            return output;
        }
        case IG_VOLUME_MESH: {
            auto input = DynamicCast<VolumeMesh>(dataObject);
            if (!input) { return nullptr; }
            auto output = VolumeMesh::New();

            // 单元（体）数组深拷贝：输出是管线里的新数据集，不能与输入共享同一份
            auto srcVolumes = input->GetVolumes();
            if (srcVolumes) {
                auto volumes = CloneCellArray(srcVolumes);
                if (volumes) { output->SetVolumes(volumes); }
            }
            output->SetName(input->GetName());
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(input->GetPoints())) { return nullptr; }
            output->SetPoints(newPoints);
            CopyAttributes(dataObject, output);
            return output;
        }
        case IG_STRUCTURED_MESH: {
            auto input = DynamicCast<StructuredMesh>(dataObject);
            if (!input) { return nullptr; }
            StructuredMesh::Pointer output = StructuredMesh::New();
            igIndex* srcDims = input->GetDimensionSize();
            if (srcDims != nullptr) {
                igIndex newDims[3] = {srcDims[0], srcDims[1], srcDims[2]};
                output->SetDimensionSize(newDims);
            }
            output->SetName(input->GetName());

            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(input->GetPoints())) { return nullptr; }
            output->SetPoints(newPoints);

            output->GenStructuredCellConnectivities();

            CopyAttributes(dataObject, output);
            return output;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto input = DynamicCast<UnstructuredMesh>(dataObject);
            if (!input) { return nullptr; }
            auto output = UnstructuredMesh::New();

            // 单元数组 + 单元类型数组深拷贝。
            auto srcCells = input->GetCells();
            if (srcCells) {
                auto cells = CloneCellArray(srcCells);
                auto types = UnsignedIntArray::New();
                if (input->GetCellTypes() != nullptr) { types->DeepCopy(input->GetCellTypes()); }
                if (cells) { output->SetCells(cells, types); }
            }
            output->SetName(input->GetName());
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(input->GetPoints())) { return nullptr; }
            output->SetPoints(newPoints);
            CopyAttributes(dataObject, output);
            return output;
        }
        case IG_POINT_SET: {
            auto input = DynamicCast<PointSet>(dataObject);
            if (!input) { return nullptr; }
            auto output = PointSet::New();
            output->SetName(input->GetName());
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(input->GetPoints())) { return nullptr; }
            output->SetPoints(newPoints);
            CopyAttributes(dataObject, output);
            return output;
        }
        default:
            return nullptr;
    }
}

inline ArrayObject::Pointer FindPointArray(AttributeSet* attrSet, const std::string& name, IGenum typeFilter,
                                           int exactComponents) {
    if (attrSet == nullptr) { return nullptr; }

    auto buffer = attrSet->GetAllAttributes();
    if (buffer == nullptr) { return nullptr; }

    ArrayObject::Pointer firstMatch = nullptr;
    for (IGsize i = 0; i < buffer->GetNumberOfElements(); ++i) {
        auto& attr = buffer->GetElement(i);
        if (attr.IsNone()) { continue; }
        if (attr.attachmentType != IG_POINT) { continue; }
        if (!attr.pointer) { continue; }
        if (typeFilter != IG_NONE && attr.type != typeFilter) { continue; }
        if (exactComponents > 0 && attr.pointer->GetDimension() != exactComponents) { continue; }

        if (name.empty()) {
            if (!firstMatch) { firstMatch = attr.pointer; }
            continue;
        }
        if (attr.pointer->GetName() == name) { return attr.pointer; }
    }
    return firstMatch;
}

} // namespace WarpSupport

IGAME_NAMESPACE_END
#endif
