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
class iGameVectorBase : public DrawObject {
public:
    I_OBJECT(iGameVectorBase);
    static iGameVectorBase* New() { return new iGameVectorBase; }
    ~iGameVectorBase();

protected:
    iGameVectorBase();

private:
    // Point array
    Points::Pointer m_Triangles;
    // color array
    FloatArray::Pointer m_PositionColors;
    UnsignedIntArray::Pointer index;
    iGame::Model::Pointer model{};
    bool isInit = false;
    float hR;
    float hL;
    float tR;
    float tL;
    unsigned int count;

public:
    void SetArrow(float _hR, float _hL, float _tR, float _tL);
    void DrawVector(std::string VecName);
    void convertPoint2Arrow(Vector3f coord, Vector3f normal, Vector3f RGB);
    //void Draw(Scene*) override;
    void ConvertToDrawableData() override;
    std::vector<float> Vector;
};
IGAME_NAMESPACE_END
#endif