#ifndef IGAMEVIS_STREAMLINE_STYLE_H
#define IGAMEVIS_STREAMLINE_STYLE_H

#include "iGameBasicStyle.h"
#include "iGamePoints.h"
#include "iGamePainter3D.h"
#include "iGameDataObject.h"

IGAME_NAMESPACE_BEGIN
class StreamLineStyle : public BasicStyle {
public:
    I_OBJECT(StreamLineStyle);
    static Pointer New() { return new StreamLineStyle; }

    void Initialize(SmartPointer<Interactor> interactor) override;

    void MousePressEvent(IEvent _event) override;
    void MouseMoveEvent(IEvent _event) override;

protected:
    StreamLineStyle();
    ~StreamLineStyle() override;

    SmartPointer<DataObject> m_DataObject;
    SmartPointer<Painter3D> m_Painter3D;
    int StartHandle, EndHandle;
    int LineHandle;

    int Selected = -1;
    igm::vec3 Start, End;
    float NDC_Z;
    igm::mat4 MVP;
    igm::mat4 InvertedMVP;
};
IGAME_NAMESPACE_END
#endif