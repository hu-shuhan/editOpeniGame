#pragma once
#include <utility>

#include "Meshleter/iGameSurfaceMeshMeshleter.h"
#include "iGameDrawObject.h"
#include "iGameObject.h"
#include "iGamePainter2D.h"
#include "iGamePainter3D.h"
#include "iGamePoints.h"
#include "iGameSelection.h"

IGAME_NAMESPACE_BEGIN
class Scene;
class Filter;
class Model : public Object {
public:
    I_OBJECT(Model);
    static Pointer New() { return new Model; }

    void Draw(Scene*);
    void DrawWithTransparency(Scene*);
    void DrawWithVolume(Scene*);
    void DrawPhase1(Scene*);
    void DrawPhase2(Scene*);
    void TestOcclusionResults(Scene*);

    DataObject::Pointer GetDataObject();
    bool GetVisibility();
    Filter* GetModelFilter();
    Painter3D::Pointer GetPainter3D();

    void SetModelFilter(SmartPointer<Filter> _filter);
    void DeleteModelFilter();
    void SetDataObject(DataObject::Pointer dataObject);
    void Modified() { m_DataObject->Modified(); }

    void Show();
    void Hide();
    void SetBoundingBoxSwitch(bool action);
    void SetPickedItemSwitch(bool action);
    void SetViewPointsSwitch(bool action);
    void SetViewWireframeSwitch(bool action);
    void SetViewFillSwitch(bool action);

    void Update();

    void ViewCloudPicture(int index, int dimension = -1);
    void SetFilePath(std::string filePath);
    std::string GetFilePath();

    Selection* GetSelection();
    void RequestPointSelection(Points* p, Selection* s);
    void RequestDragPoint(Points* p, Selection* s);

    void SetMeshleter(Meshleter::Pointer meshleter);

protected:
    Model();
    ~Model() override;

    enum ViewSwitch { BoundingBox = 0, PickedItem };

    void SwitchOn(ViewSwitch type) { m_Switch |= (1ull << type); }
    void SwitchOff(ViewSwitch type) { m_Switch &= ~(1ull << type); }
    bool GetSwitch(ViewSwitch type) { return m_Switch & (1ull << type); }

    Meshleter::Pointer m_Meshleter;

    Selection::Pointer m_Selection;
    SmartPointer<Filter> m_Filter;
    DataObject::Pointer m_DataObject;
    std::string m_FilePath;
    Scene* m_Scene;
    Painter3D::Pointer m_Painter3D;
    IGuint m_BboxHandle;
    unsigned long long m_Switch;

    friend class Scene;
};

IGAME_NAMESPACE_END
