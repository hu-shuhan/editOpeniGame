#ifndef iGameStreamBase_h
#define iGameStreamBase_h
#include "iGameDrawObject.h"
#include "iGameSurfaceMesh.h"
#include <iGameFilter.h>
#include <iGameStreamTracer.h>
IGAME_NAMESPACE_BEGIN
class Scene;
class iGameStreamBase : public Filter, public DrawObject {
public:
    I_OBJECT(iGameStreamBase);
    static iGameStreamBase* New() { return new iGameStreamBase; }
    ~iGameStreamBase();
    iGameStreamTracer* streamFilter;
    void SetStreamLine(std::vector<std::vector<float>> streamLine) {
        auto tmp = std::vector<std::vector<float>>();
        m_StreamLine.swap(tmp);
        m_StreamLine = streamLine;
    }

protected:
    iGameStreamBase();

private:
    // Point array
    Points::Pointer m_Points;
    // color array
    FloatArray::Pointer m_PositionColors;

public:
    //void Draw(Scene*) override;
    void ConvertToDrawableData() override;
    std::vector<std::vector<float>> m_StreamLine;
};
IGAME_NAMESPACE_END
#endif