#include "iGameConvertPolyhedralCells.h"

IGAME_NAMESPACE_BEGIN


bool ConvertPolyhedralCells::Execute() {
    if (DynamicCast<VolumeMesh>(GetInput(0))) {
        VolumeMesh::Pointer OldMesh = DynamicCast<VolumeMesh>(GetInput(0));
        VolumeMesh::Pointer NewMesh = VolumeMesh::New();
        NewMesh->SetName(OldMesh->GetName());
        SetOutput(NewMesh);
        return ConvertToTetra(OldMesh, NewMesh);
    } else if (DynamicCast<UnstructuredMesh>(GetInput(0))) {
        UnstructuredMesh::Pointer OldMesh = DynamicCast<UnstructuredMesh>(GetInput(0));
        UnstructuredMesh::Pointer NewMesh = UnstructuredMesh::New();
        NewMesh->SetName(OldMesh->GetName());
        SetOutput(NewMesh);
        return ConvertToTetra(OldMesh, NewMesh);
    }
    return false;
}

bool ConvertPolyhedralCells::ConvertToTetra(VolumeMesh::Pointer OldMesh, VolumeMesh::Pointer NewMesh) {
    if (OldMesh == nullptr || OldMesh->GetIsPolyhedronType()) return false;

    auto OldAttrs = OldMesh->GetAttributeSet();
    auto NewAttrs = NewMesh->GetAttributeSet();
    auto NewCells = NewMesh->GetCells();
    NewCells->Reset();

    std::vector<std::pair<FloatArray::Pointer, ArrayObject::Pointer>> cellArray;
    for (int i = 0; i < OldAttrs->GetNumberOfAttributes(); i++) {
        auto& attr = OldAttrs->GetAttribute(i);
        if (attr.attachmentType == IG_POINT) {
            FloatArray::Pointer arr = FloatArray::New();

            arr->SetName(attr.pointer->GetName());
            arr->SetDimension(attr.pointer->GetDimension());
            arr->Reserve(attr.pointer->GetNumberOfElements());
            for (IGsize i = 0; i < attr.pointer->GetNumberOfValues(); i++) { arr->AddValue(attr.pointer->GetValue(i)); }
            NewAttrs->AddAttribute(attr.type, attr.attachmentType, arr);
        } else {

            FloatArray::Pointer arr = FloatArray::New();

            arr->SetName(attr.pointer->GetName());
            arr->SetDimension(attr.pointer->GetDimension());

            cellArray.emplace_back(arr, attr.pointer);
            NewAttrs->AddAttribute(attr.type, attr.attachmentType, arr);
        }
    }

    double ids[IGAME_CELL_MAX_SIZE];
    for (int i = 0; i < OldMesh->GetNumberOfVolumes(); i++) {
        auto Cell = OldMesh->GetVolume(i);
        std::vector<iGame::Cell::Pointer> tetras = Cell->clipCelltoTetra();
        for (auto tetra: tetras) {
            NewCells->AddCellIds(tetra->m_PointIds);
            for (int j = 0; j < cellArray.size(); j++) {
                auto& [n, o] = cellArray[j];
                o->GetElement(i, ids);
                n->AddElement(ids);
            }
        }
    }

    auto NewPoints = NewMesh->GetPoints();
    NewPoints->Reset();
    for (int i = 0; i < OldMesh->GetNumberOfPoints(); i++) { NewPoints->AddPoint(OldMesh->GetPoint(i)); }
    return true;
}

bool ConvertPolyhedralCells::ConvertToTetra(UnstructuredMesh::Pointer OldMesh, UnstructuredMesh::Pointer NewMesh) {
    if (OldMesh == nullptr) return false;

    auto OldAttrs = OldMesh->GetAttributeSet();
    auto NewAttrs = NewMesh->GetAttributeSet();
    auto NewCells = NewMesh->GetCells();
    auto NewCellTypes = NewMesh->GetCellTypes();
    NewCells->Reset();

    std::vector<std::pair<FloatArray::Pointer, ArrayObject::Pointer>> cellArray;
    for (int i = 0; i < OldAttrs->GetNumberOfAttributes(); i++) {
        auto& attr = OldAttrs->GetAttribute(i);
        if (attr.attachmentType == IG_POINT) {
            FloatArray::Pointer arr = FloatArray::New();

            arr->SetName(attr.pointer->GetName());
            arr->SetDimension(attr.pointer->GetDimension());
            arr->Reserve(attr.pointer->GetNumberOfElements());
            for (IGsize i = 0; i < attr.pointer->GetNumberOfValues(); i++) { arr->AddValue(attr.pointer->GetValue(i)); }
            NewAttrs->AddAttribute(attr.type, attr.attachmentType, arr);
        } else {
            FloatArray::Pointer arr = FloatArray::New();

            arr->SetName(attr.pointer->GetName());
            arr->SetDimension(attr.pointer->GetDimension());

            cellArray.emplace_back(arr, attr.pointer);
            NewAttrs->AddAttribute(attr.type, attr.attachmentType, arr);
        }
    }

    double ids[IGAME_CELL_MAX_SIZE];
    for (int i = 0; i < OldMesh->GetNumberOfCells(); i++) {
        Volume* cell = dynamic_cast<Volume*>(OldMesh->GetCell(i));
        if (!cell) continue;
        std::vector<iGame::Cell::Pointer> tetras = cell->clipCelltoTetra();
        for (auto tetra: tetras) {
            NewCells->AddCellIds(tetra->m_PointIds);
            NewCellTypes->AddValue(IG_TETRA);

            for (int j = 0; j < cellArray.size(); j++) {
                auto& [n, o] = cellArray[j];
                o->GetElement(i, ids);
                n->AddElement(ids);
            }
        }
    }

    auto NewPoints = NewMesh->GetPoints();
    NewPoints->Reset();
    for (int i = 0; i < OldMesh->GetNumberOfPoints(); i++) { NewPoints->AddPoint(OldMesh->GetPoint(i)); }
    return true;
}


ConvertPolyhedralCells::ConvertPolyhedralCells() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

IGAME_NAMESPACE_END