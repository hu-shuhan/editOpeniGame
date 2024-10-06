#pragma once
#ifndef iGameVectorBase_h
#define iGameVectorBase_h
#include "iGameDrawObject.h"
#include "iGameSceneManager.h"
#include <iGameFilter.h>
#include <iGamePainter.h>
#include <iGameScalarsToColors.h>
#include <iGameVector.h>
#include <iGameVolumeMesh.h>
IGAME_NAMESPACE_BEGIN
class Scene;
class iGameVectorBase : public Filter, public DrawObject {
public:
    I_OBJECT(iGameVectorBase);
    static iGameVectorBase* New() { return new iGameVectorBase; }
    ~iGameVectorBase();

protected:
    iGameVectorBase();

private:
    // Point array
    Points::Pointer m_Points;
    // color array
    FloatArray::Pointer m_PositionColors;
    iGame::Model::Pointer model{};
    bool isInit = false;

public:
    void DrawVector(std::string VecName);
    std::vector<Vector3f> convertPoint2Arrow(Vector3f coord, Vector3f normal);
    //void Draw(Scene*) override;
    void ConvertToDrawableData() override;
    std::vector<float> Vector;
    float hR;
    float hL;
    float tR;
    float tL;
};
IGAME_NAMESPACE_END
#endif