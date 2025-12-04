#ifndef iGameSelection_h
#define iGameSelection_h

#include "iGameCellArray.h"
#include "iGameElementArray.h"
#include "iGameIdArray.h"
#include "iGameObject.h"
#include "iGamePoints.h"
#include <functional>
#include <iGameCellFaceExtracter.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <iGameDataObject.h>

IGAME_NAMESPACE_BEGIN
//class SingleSelectionInterface {
//public:
//	struct Event : Selection::Event {
//		enum Type {
//			PickPoint = 0,
//			DragPoint,
//		};
//
//		Type type;
//		Vector3f pos;
//		igIndex pickId;
//	};
//
//	virtual void FilterEvent(SingleSelectionInterface::Event _event) = 0;
//};

class Model;
class UnstructuredMesh;
class VolumeMesh;
class SurfaceMesh;
class Painter3D;
class Selection : public Object {
public:
    I_OBJECT(Selection);
    static Pointer New() { return new Selection; }

    //struct Event {
    //    enum Type { PickPoint = 0, DragPoint, PickFace, PickLine, Change };
    //    enum Operate { NoOperate = 0, Add, Remove };

    //    Type type{};
    //    Operate operate{};
    //    std::vector<IGuint> drawHandles{};
    //    Vector3f pos{};
    //    igIndex pickId{};
    //};

    enum Operate { NoOperate = 0, Add, Remove };

public:
    //static std::vector<Event> GenerateEvents(const std::vector<igIndex>& ids, IGenum type, Event::Operate ope,
    //                                            UnstructuredMesh* mesh, Painter3D* painter = nullptr);

    //   static std::vector<Event> GeneratePointEvents(const std::vector<igIndex>& ids, Event::Operate ope,
    //                                                 UnstructuredMesh* mesh, Painter3D* painter = nullptr);

    //   static std::vector<Event> GenerateCellEvents(const std::vector<igIndex>& ids, Event::Operate ope,
    //                                                UnstructuredMesh* mesh, Painter3D* painter = nullptr);

    //void SelectionCallBackEvent(const std::vector<Event>& _events, bool letCellDrawWithExtracter = false);

    //void SelectionCallBackEvent(const Event& event, bool letCellDrawWithExtracter = false);

    void SelectionCallBackEvent(IGenum itemType, const std::vector<igIndex>& ids = {},
                                Operate ope = Operate::NoOperate);

    void SelectionCallBackEvent(IGenum itemType, const igIndex& id, Operate ope = Operate::NoOperate);

    //void AddSelectedItems(const std::vector<igIndex>& ids, IGenum type);

    //void RemoveSelectedItems(const std::vector<igIndex>& ids, IGenum type);

    const std::set<igIndex>& GetSelectedItems(IGenum itemType);

    const std::set<igIndex>& GetSelectedCells();

    const std::set<igIndex>& GetSelectedPoints();

    bool IsSelectedItem(IGenum itemType, igIndex itemId);

    void Reset();

    void ClearSelections();

    //template<typename Functor, typename... Args>
    //void SetFilterEvent(Functor&& functor, Args&&... args) {
    //	m_Functor = std::bind(std::forward<Functor>(functor), std::forward<Args>(args)...);
    //}

    template<typename Functor, typename... Args>
    void _SetSelectionCallBackEvent(std::string funcKey, Functor&& functor, Args&&... args) {
        std::function<void(IGenum itemType, const std::vector<igIndex>& ids, Operate ope)> func =
                std::bind(std::forward<Functor>(functor), std::forward<Args>(args)...);
        m_CallBackFunctor[funcKey] = func;
    }

    void _SetSelectionCallBackEvent_(
            const std::string& funcName,
            const std::function<void(IGenum itemType, const std::vector<igIndex>& ids, Operate ope)>& func) {
        m_CallBackFunctor[funcName] = func;
    }

#define SetSelectionCallBackEvent(functor, ...)                                                                        \
    _SetSelectionCallBackEvent(std::string(__FILE__) + std::to_string(__LINE__), functor, __VA_ARGS__)

    template<typename Functor, typename... Args>
    void _SetClearSelectionCallBackEvent(std::string funcKey, Functor&& functor, Args&&... args) {
        std::function<void()> func = std::bind(std::forward<Functor>(functor), std::forward<Args>(args)...);
        m_ClearSelectionCallBackFunctor[funcKey] = func;
    }

    void _SetClearSelectionCallBackEvent_(std::string funcName, const std::function<void()>& func) {
        m_ClearSelectionCallBackFunctor[funcName] = func;
    }

#define SetClearSelectionCallBackEvent(functor, ...)                                                                   \
    _SetClearSelectionCallBackEvent(std::string(__FILE__) + std::to_string(__LINE__), functor, __VA_ARGS__)

    template<typename Functor, typename... Args>
    void _SetBoxSelectInitCallBackEvent(std::string funcKey, Functor&& functor, Args&&... args) {
        std::function<void(IGenum itemType, const Point& p1, const Point& p2)> func =
                std::bind(std::forward<Functor>(functor), std::forward<Args>(args)...);
        m_BoxSelectInitCallBackFunctor[funcKey] = func;
    }

    void _SetBoxSelectInitCallBackEvent_(
            const std::string& funcName,
            const std::function<void(IGenum itemType, const Point& p1, const Point& p2)>& func) {
        m_BoxSelectInitCallBackFunctor[funcName] = func;
    }

#define SetBoxSelectInitCallBackEvent(functor, ...)                                                                    \
    _SetBoxSelectInitCallBackEvent(std::string(__FILE__) + std::to_string(__LINE__), functor, __VA_ARGS__)

    Points* GetPoints() { return m_Points; }
    CellArray* GetCells() { return m_Cells; }
    Model* GetModel() { return m_Model; }
    void SetPoints(Points* p) { m_Points = p; }
    void SetCells(CellArray* c) { m_Cells = c; }
    void SetModel(Model* m) { m_Model = m; }

    const std::vector<int>& GetSeeAbleCells(UnstructuredMesh* mesh);
    const std::vector<int>& GetSeeAbleCells(VolumeMesh* mesh);
    const std::vector<int>& GetSeeAbleCells(SurfaceMesh* mesh);

    void SetSelectItemVisable(bool visable);
    void SetSelectBoxVisable(bool visable);

    CellFaceExtracter& GetCellFaceExtracter();

protected:
    Selection();
    ~Selection() override = default;

    void SetBoxStyle(const std::pair<Point, Point>& p);

    //std::function<void(Event)> m_Functor;

    //Vector3f m_Position{};
    //igIndex m_PickedId{ -1 };
    //IdArray::Pointer m_SelectedIds{};
    std::map<std::string, std::function<void(IGenum itemType, const std::vector<igIndex>& ids, Operate ope)>>
            m_CallBackFunctor;
    std::map<std::string, std::function<void()>> m_ClearSelectionCallBackFunctor;
    std::map<std::string, std::function<void(IGenum itemType, const Point& p1, const Point& p2)>>
            m_BoxSelectInitCallBackFunctor;

    void AddItem(IGenum itemType, const igIndex& itemId, Operate ope);
    void DrawPoints();
    void DrawCellEdges();
    void DrawBoundingBox(const std::pair<Point, Point>& p);
    void DrawCellBoundingBoxs();
    //UnstructuredMesh* _GetMesh();

    std::map<IGenum, std::set<igIndex>> m_SelectedItems;
    CellFaceExtracter m_CellFaceExtracter;

    std::vector<int> m_SeeAbleCells; //pointIds,CellId

    Points* m_Points{nullptr};
    CellArray* m_Cells{nullptr};
    Model* m_Model{nullptr};
    //DataObject::Pointer m_DataObjectPointerMesh{nullptr};

    friend class Model;
};

class StreamLineSelection : public Selection {
public:
    I_OBJECT(StreamLineSelection);
    static Pointer New() { return new StreamLineSelection; }

    Vector3d Start;
    Vector3d End;
    int Selected;

protected:
    StreamLineSelection() {}
    ~StreamLineSelection() override = default;
};

class ClipSelection : public Selection {
public:
    I_OBJECT(ClipSelection);
    static Pointer New() { return new ClipSelection; }

    Vector3d PlanePoint;
    Vector3d PlaneNormal;
    bool Preview;

    void UpdatePlane() {
        if (Update) Update();
    }

    template<typename Functor, typename... Args>
    void SetUpdateFunction(Functor&& functor, Args&&... args) {
        Update = std::bind(std::forward<Functor>(functor), std::forward<Args>(args)...);
    }

protected:
    ClipSelection() {}
    ~ClipSelection() override = default;

    std::function<void()> Update;
};

IGAME_NAMESPACE_END
#endif