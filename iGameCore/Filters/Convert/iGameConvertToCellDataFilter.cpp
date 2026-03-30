#include "iGameConvertToCellDataFilter.h"

IGAME_NAMESPACE_BEGIN

bool ConvertToCellDataFilter::Execute() {
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
        if (attr.attachmentType == IG_POINT) { Count++; }
    }
    if (Count == 0) return true;

    igIndex ids[IGAME_CELL_MAX_SIZE]{};
    float vals[IGAME_CELL_MAX_SIZE]{};

    for (int i = 0; i < Mesh->GetAttributeSet()->GetNumberOfAttributes(); ++i) {
        auto& attr = Attrs->GetAttribute(i);
        if (attr.attachmentType == IG_POINT) {
            int dim = attr.pointer->GetDimension();

            FloatArray::Pointer arr = FloatArray::New();
            arr->SetDimension(dim);
            arr->SetName(attr.pointer->GetName());
            arr->Resize(Cells->GetNumberOfCells());

            for (int j = 0; j < Cells->GetNumberOfCells(); ++j) {
                int Size = Cells->GetCellIds(j, ids);
                for (int k = 0; k < Size; ++k) {
                    attr.pointer->GetElement(ids[k], vals);
                    for (int d = 0; d < dim; ++d) {
                        arr->SetValue(j * dim + d, arr->GetValue(j * dim + d) + vals[d] / Size);
                    }
                }
            }

            attr.pointer = arr;
            attr.attachmentType = IG_CELL;
        }
    }
    SetOutput(Mesh);
    return true;
}

ConvertToCellDataFilter::ConvertToCellDataFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END
