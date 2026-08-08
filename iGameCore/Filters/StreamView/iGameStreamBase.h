#ifndef StreamBase_h
#define StreamBase_h
#include "iGameDrawObject.h"
#include "iGameFilter.h"
#include "iGameStreamTracer.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
IGAME_NAMESPACE_BEGIN
class Scene;
class StreamBase : virtual public DrawObject {
public:
    I_OBJECT(StreamBase);
    static StreamBase* New() { return new StreamBase; }
    ~StreamBase();
    StreamTracer* streamFilter;
    Painter3D::Pointer m_Painter=Painter3D::New();
    std::vector<Vector3f> seeds;
    //std::string vecName;
    void SetSeeds(std::vector<Vector3f> _seeds) {
        seeds = _seeds;
    }
    //void SetVecName(std::string vecN) { vecName = vecN;
    //}
    void SetUpdate(bool flag) {
        isUpdate = flag;
        ConvertToDrawableData();
    }
    IGsize GetRealMemorySize() {
        IGsize res = this->DrawObject::GetRealMemorySize();
        if (m_Points) res += m_Points->GetRealMemorySize();
        if (m_PositionColors) res += m_PositionColors->GetRealMemorySize();
        if (streamFilter && streamFilter->GetOutput()) res += streamFilter->GetOutput()->GetRealMemorySize();

        return 2 * res;
    }


protected:
    StreamBase();

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
};
IGAME_NAMESPACE_END
#endif
