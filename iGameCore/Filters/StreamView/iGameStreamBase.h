#ifndef iGameStreamBase_h
#define iGameStreamBase_h
#include "iGameDrawObject.h"
#include "iGameSurfaceMesh.h"
#include "iGameFilter.h"
#include "iGameStreamTracer.h"

IGAME_NAMESPACE_BEGIN
class Scene;
class iGameStreamBase : virtual public Filter, virtual public DrawObject {
public:
    I_OBJECT(iGameStreamBase);
    static iGameStreamBase* New() { return new iGameStreamBase; }
    ~iGameStreamBase();
    iGameStreamTracer* streamFilter;
    void SetStreamLine(std::vector<std::vector<float>> streamLine, std::vector<std::vector<float>> streamLineColor) {
        auto tmp1 = std::vector<std::vector<float>>();
        m_StreamLine.swap(tmp1);
        m_StreamLine = streamLine;
        auto tmp2 = std::vector<std::vector<float>>();
        m_StreamLineColor.swap(tmp2);
        m_StreamLineColor = streamLineColor;
        isUpdate = true;
        //ConvertToDrawableData();
    }
    IGsize GetRealMemorySize() override {
        IGsize res = this->DrawObject::GetRealMemorySize();
        if (m_Points) res += m_Points->GetRealMemorySize();
        if (m_PositionColors) res += m_PositionColors->GetRealMemorySize();

        return 2*res ;
    }


protected:
    iGameStreamBase();

private:
    // Point array
    Points::Pointer m_Points;
    UnsignedIntArray::Pointer index;
    // color array
    FloatArray::Pointer m_PositionColors;
    bool isUpdate = false;

public:
    //void Draw(Scene*) override;
    void ComputeBoundingBox() override;
    void ConvertToDrawableData() override;
    bool IsUseSinglePassWireframeRendering() override;
    std::vector<std::vector<float>> m_StreamLine;
    std::vector<std::vector<float>> m_StreamLineColor;
};
IGAME_NAMESPACE_END
#endif