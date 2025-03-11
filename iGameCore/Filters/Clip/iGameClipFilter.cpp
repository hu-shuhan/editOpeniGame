#include "iGameClipFilter.h"
IGAME_NAMESPACE_BEGIN
ClipFilter::ClipFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
    m_Clipper = QuickModelClip::New();
}
ClipFilter::~ClipFilter() { m_Clipper = nullptr; }
bool ClipFilter::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) { return false; }
    auto input = m_Inputs->GetElement(0);
    if (!input) { return false; }
    bool res = false;
    switch (input->GetDataObjectType()) {
        case IG_NONE:
            return true;
        case IG_VOLUME_MESH:
            m_Clipper->SetInput(input);
            res = m_Clipper->Execute();
            break;

        case IG_SURFACE_MESH:
            m_Clipper->SetInput(input);
            res = m_Clipper->Execute();
            break;

        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(input);
            bool couldQuickClip = true;
            auto Types = mesh->GetCellTypes();
            auto types = Types->RawPointer();
            int CellNum = mesh->GetNumberOfCells();
            for (int i = 0; i < CellNum && couldQuickClip; i++) {
                switch (types[i]) {
                    case IG_TETRA:
                    case IG_PYRAMID:
                    case IG_PRISM:
                    case IG_HEXAHEDRON:
                    case IG_TRIANGLE:
                    case IG_QUAD:
                    case IG_LINE:
                    case IG_VERTEX:
                        break;
                    default:
                        couldQuickClip = false;
                        break;
                }
            }
            m_Clipper->SetInput(input);
            if (couldQuickClip) res = m_Clipper->Execute();
            else
                res = m_Clipper->ModelClip::Execute();
            break;
        }
        case IG_STRUCTURED_MESH:
            m_Clipper->SetInput(input);
            res = m_Clipper->Execute();
            break;
        default:
            return false;
    }
    if (res) {
        auto out = m_Clipper->GetOutput();
        this->SetOutput(out);
    }
    return res;
}
IGAME_NAMESPACE_END
