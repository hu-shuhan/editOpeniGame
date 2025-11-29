#ifndef iGameStreamBase_h
#define iGameStreamBase_h
#include "iGameDrawObject.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameFilter.h"
#include "iGameStreamTracer.h"

IGAME_NAMESPACE_BEGIN
class Scene;
class iGameStreamBase :    virtual public DrawObject {
public:
    I_OBJECT(iGameStreamBase);
    static iGameStreamBase* New() { return new iGameStreamBase; }
    ~iGameStreamBase();
    iGameStreamTracer* streamFilter;

    void SetUpdate(bool flag) {
        isUpdate = flag;
        ConvertToDrawableData();
    }

    IGsize GetRealMemorySize() {
        IGsize res = this->DrawObject::GetRealMemorySize();
        if (m_Points) res += m_Points->GetRealMemorySize();
        if (m_PositionColors) res += m_PositionColors->GetRealMemorySize();
        if (streamFilter && streamFilter->GetOutput()) res += streamFilter->GetOutput()->GetRealMemorySize();

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
};
IGAME_NAMESPACE_END
#endif