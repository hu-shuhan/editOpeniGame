#include <iostream>
#include <cmath>
#include <algorithm>
#include <iGameFileIO.h>
#include <iGameSelectionFilters/iGameSetSelectionCallBackFunc.h>

static void SelectionCallBackFunc(const std::vector<iGame::Selection::Event>& events) {
    for (auto& e: events) {
        std::cout << "Type:" << e.type << ' ' << "Operate:" << e.operate << ' ' << "Id:" << e.pickId << std::endl;
    }
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
    auto filter = iGame::iGameSetSelectionCallBackFunc::New("SelectionCallBackFunc_1", SelectionCallBackFunc);
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return 0;
    }
    /*Get results*/
    auto resultData = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput(0));
    /*Select*/
    auto selectEvents = iGame::Selection::GenerateEvents(std::vector<igIndex>{0, 1, 2}, IG_POINT,
                                                         iGame::Selection::Event::Operate::Add, mesh->GetPoints(),
                                                         mesh->GetCells());
    resultData->GetSelection()->SelectionCallBackEvent(selectEvents);
    return 0;
}