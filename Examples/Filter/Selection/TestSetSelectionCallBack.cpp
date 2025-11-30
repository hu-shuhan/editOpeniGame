#include <Selection/iGameSetSelectionCallBackFuncFilter.h>
#include <algorithm>
#include <cmath>
#include <iGameFileIO.h>
#include <iostream>

static void SelectionCallBackFunc(IGenum itemType, const std::vector<igIndex>& ids, iGame::Selection::Operate ope) {
    std::cout << "Type:" << itemType << ' ' << "Operate:" << ope << ' ' << "Id:" << std::endl;
    for (auto& id: ids) { std::cout << id << ','; }
    std::cout << std::endl;
}

int main() {
    /*Read data*/
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    iGame::UnstructuredMesh::Pointer mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 0;
    }
    /*Set filter*/
    auto filter = iGame::SetSelectionCallBackFuncFilter::New("SelectionCallBackFunc_1", SelectionCallBackFunc);
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return 0;
    }
    /*Get results*/
    auto resultData = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput(0));
    /*Select*/
    auto selectItems = std::vector<igIndex>{0, 1, 2};
    resultData->GetSelection()->SelectionCallBackEvent(IG_POINT, selectItems, iGame::Selection::Operate::Add);
    return 0;
}
