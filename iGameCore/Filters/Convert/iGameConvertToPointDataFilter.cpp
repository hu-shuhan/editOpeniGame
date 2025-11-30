#include "iGameConvertToPointDataFilter.h"
#include "iGameDataObject.h"
IGAME_NAMESPACE_BEGIN

bool ConvertToPointDataFilter::Execute() {
    if (GetInput(0) == nullptr) return false;
    auto Mesh = GetInput(0);
    auto Points = Mesh->GetPoints();
    auto Cells = Mesh->GetCellArray();
    if (Points == nullptr || Cells == nullptr) return false;
    auto Attrs = Mesh->GetAttributeSet();
    if (Attrs == nullptr || Attrs->GetNumberOfAttributes() == 0) return true;

    int Count = 0;
    for (int i = 0; i < Mesh->GetAttributeSet()->GetNumberOfAttributes(); ++i) {
        auto& attr = Mesh->GetAttributeSet()->GetAttribute(i);
        if (attr.attachmentType == IG_CELL) { Count++; }
    }
    if (Count == 0) return true;

    std::vector<unsigned char> PointDegree(Points->GetNumberOfPoints(), 0);

    igIndex ids[IGAME_CELL_MAX_SIZE]{};
    float vals[IGAME_CELL_MAX_SIZE]{};

    for (int i = 0; i < Cells->GetNumberOfCells(); ++i) {
        int Size = Cells->GetCellIds(i, ids);
        for (int j = 0; j < Size; ++j) { 
            PointDegree[ids[j]]++; 
        }
    }

    for (int i = 0; i < Mesh->GetAttributeSet()->GetNumberOfAttributes(); ++i) {
        auto& attr = Mesh->GetAttributeSet()->GetAttribute(i);
        if (attr.attachmentType == IG_CELL) {
            int dim = attr.pointer->GetDimension();

            FloatArray::Pointer arr = FloatArray::New();
            arr->SetDimension(dim);
            arr->SetName(attr.pointer->GetName());
            arr->Resize(Points->GetNumberOfPoints());

            for (int j = 0; j < Cells->GetNumberOfCells(); ++j) {
                int Size = Cells->GetCellIds(j, ids);
                attr.pointer->GetElement(j, vals);
                for (int k = 0; k < Size; ++k) {
                    for (int d = 0; d < dim; ++d) {
                        arr->SetValue(ids[k] * dim + d,
                                      arr->GetValue(ids[k] * dim + d) + vals[d] / PointDegree[ids[k]]);
                    }
                }
            }
            attr.pointer = arr;
            attr.attachmentType = IG_POINT;
        }
    }
    SetOutput(Mesh);
    return true;
}

ConvertToPointDataFilter::ConvertToPointDataFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END
