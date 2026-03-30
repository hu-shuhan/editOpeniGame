#include "iGameConvertToPointCloudFilter.h"

IGAME_NAMESPACE_BEGIN

bool ConvertToPointCloudFilter::Execute() {
    if (GetInput(0) == nullptr) return false;
    auto mesh = GetInput(0);
    PointSet::Pointer NewMesh = PointSet::New();
    NewMesh->SetName(mesh->GetName());

    SetOutput(NewMesh);
    if (DynamicCast<SurfaceMesh>(GetInput(0))) {
        return ExecuteWithSurfaceMesh(DynamicCast<SurfaceMesh>(GetInput(0)), NewMesh);
    } else if (DynamicCast<VolumeMesh>(GetInput(0))) {
        return ExecuteWithVolumeMesh(DynamicCast<VolumeMesh>(GetInput(0)), NewMesh);
    } else if (DynamicCast<UnstructuredMesh>(GetInput(0))) {
        return ExecuteWithUnstructuredMesh(DynamicCast<UnstructuredMesh>(GetInput(0)), NewMesh);
    }

    return false;
}


bool ConvertToPointCloudFilter::ExecuteWithSurfaceMesh(SurfaceMesh::Pointer OldMesh, PointSet::Pointer NewMesh) {
    if (OldMesh == nullptr) return false;

    auto OldAttrs = OldMesh->GetAttributeSet();
    auto NewAttrs = NewMesh->GetAttributeSet();

    auto NewPoints = NewMesh->GetPoints();
    NewPoints->Reset();
    for (int i = 0; i < OldMesh->GetNumberOfPoints(); i++) { NewPoints->AddPoint(OldMesh->GetPoint(i)); }

    for (int i = 0; i < OldAttrs->GetNumberOfAttributes(); i++) {
        auto& attr = OldAttrs->GetAttribute(i);
        if (attr.attachmentType == IG_POINT) {
            FloatArray::Pointer arr = FloatArray::New();

            arr->SetName(attr.pointer->GetName());
            arr->SetDimension(attr.pointer->GetDimension());
            arr->Reserve(attr.pointer->GetNumberOfElements());
            for (IGsize i = 0; i < attr.pointer->GetNumberOfValues(); i++) { arr->AddValue(attr.pointer->GetValue(i)); }
            NewAttrs->AddAttribute(attr.type, attr.attachmentType, arr);
        }
    }
    return true;
}


bool ConvertToPointCloudFilter::ExecuteWithVolumeMesh(VolumeMesh::Pointer OldMesh, PointSet::Pointer NewMesh) {
    if (OldMesh == nullptr) return false;

    auto OldAttrs = OldMesh->GetAttributeSet();
    auto NewAttrs = NewMesh->GetAttributeSet();

    auto NewPoints = NewMesh->GetPoints();
    NewPoints->Reset();
    for (int i = 0; i < OldMesh->GetNumberOfPoints(); i++) { NewPoints->AddPoint(OldMesh->GetPoint(i)); }

    for (int i = 0; i < OldAttrs->GetNumberOfAttributes(); i++) {
        auto& attr = OldAttrs->GetAttribute(i);
        if (attr.attachmentType == IG_POINT) {
            FloatArray::Pointer arr = FloatArray::New();

            arr->SetName(attr.pointer->GetName());
            arr->SetDimension(attr.pointer->GetDimension());
            arr->Reserve(attr.pointer->GetNumberOfElements());
            for (IGsize i = 0; i < attr.pointer->GetNumberOfValues(); i++) { arr->AddValue(attr.pointer->GetValue(i)); }
            NewAttrs->AddAttribute(attr.type, attr.attachmentType, arr);
        }
    }
    return true;
}


bool ConvertToPointCloudFilter::ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer OldMesh,
                                                            PointSet::Pointer NewMesh) {
    if (OldMesh == nullptr) return false;

    auto OldAttrs = OldMesh->GetAttributeSet();
    auto NewAttrs = NewMesh->GetAttributeSet();

    auto NewPoints = NewMesh->GetPoints();
    NewPoints->Reset();
    for (int i = 0; i < OldMesh->GetNumberOfPoints(); i++) { NewPoints->AddPoint(OldMesh->GetPoint(i)); }

    for (int i = 0; i < OldAttrs->GetNumberOfAttributes(); i++) {
        auto& attr = OldAttrs->GetAttribute(i);
        if (attr.attachmentType == IG_POINT) {
            FloatArray::Pointer arr = FloatArray::New();

            arr->SetName(attr.pointer->GetName());
            arr->SetDimension(attr.pointer->GetDimension());
            arr->Reserve(attr.pointer->GetNumberOfElements());
            for (IGsize i = 0; i < attr.pointer->GetNumberOfValues(); i++) { arr->AddValue(attr.pointer->GetValue(i)); }
            NewAttrs->AddAttribute(attr.type, attr.attachmentType, arr);
        }
    }
    return true;
}


ConvertToPointCloudFilter::ConvertToPointCloudFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END
