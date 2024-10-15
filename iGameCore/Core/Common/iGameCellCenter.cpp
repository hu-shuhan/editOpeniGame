#include<iGameCellCenter.h>
//volume's points
iGame::Vector3f CellCenter::GetCenter(iGame::Points::Pointer points) {
    if (function == Mean) { 
        int num = points->GetNumberOfPoints();
        iGame::Vector3f result(0.0, 0.0, 0.0);
        for (int i = 0; i < num; i++) {
            result += points->GetPoint(i);
        }
        result = result / num;
        return result;
    } 

}// volumeMesh's points
iGame::Vector3f CellCenter::GetCenter(iGame::Points::Pointer points,iGame::CellArray* allVolume,int cellId) {
    if (function == Mean) {
        igIndex p[32] = {0};
        int num = allVolume->GetCellIds(cellId, p);
        iGame::Vector3f result(0.0, 0.0, 0.0);
        for (int i = 0; i < num; i++) { result += points->GetPoint(p[i]); }
        result = result / num;
        return result;
    }
}