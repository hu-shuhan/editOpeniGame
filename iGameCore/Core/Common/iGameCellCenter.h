#include<iGameVolume.h>
#include<iGameCellArray.h>
#pragma once
class CellCenter : public iGame::Object {
public:
    enum functionSet{Mean = 0};
    iGame::Vector3f GetCenter(iGame::Points::Pointer points);
    iGame::Vector3f GetCenter(iGame::Points::Pointer points,iGame::CellArray* allVolume,int cellId);
    void SetFunction(functionSet m_function) { function = m_function;
    }

private:
    functionSet function = Mean;
};