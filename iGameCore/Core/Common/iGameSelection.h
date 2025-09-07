#ifndef iGameSelection_h
#define iGameSelection_h

#include "iGameObject.h"
#include "iGameElementArray.h"
#include "iGamePoints.h"
#include "iGameIdArray.h"
#include "iGameCellArray.h"
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

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
class Painter3D;
class Selection : public Object {
public:
	I_OBJECT(Selection);
	static Pointer New() { return new Selection; }
	
	struct Event {
		enum Type {
			PickPoint = 0,
			DragPoint,
			PickFace, 
			PickLine,
			Change
		};
        enum Operate { NoOperate = 0, Add, Remove };

		Type type{};
        Operate operate{};
        std::vector<IGuint> drawHandles{};
        Vector3f pos{};
        igIndex pickId{};
	};

public:
    static std::vector<Event> GenerateEvents(const std::vector<igIndex>& ids, IGenum type, Event::Operate ope,
                                             Points* points, CellArray* cellArrays, Painter3D* painter);

	void SelectionCallBackEvent(const std::vector<Event>& _events);

	void SelectionCallBackEvent(const Event& event);

	//void AddSelectedItems(const std::vector<igIndex>& ids, IGenum type);

	//void RemoveSelectedItems(const std::vector<igIndex>& ids, IGenum type);

	const std::map<Event::Type, std::map<igIndex, Event>>& GetSelectedItems() const { return m_SelectedItems; }

	void Reset();

	//template<typename Functor, typename... Args>
	//void SetFilterEvent(Functor&& functor, Args&&... args) {
	//	m_Functor = std::bind(std::forward<Functor>(functor), std::forward<Args>(args)...);
	//}

    template<typename Functor, typename... Args>
    void _SetSelectionCallBackEvent(std::string funcKey, Functor&& functor, Args&&... args) {
        std::function<void(const std::vector<Event>&)> func =
                std::bind(std::forward<Functor>(functor), std::forward<Args>(args)...);
        m_CallBackFunctor[funcKey] = func;
    }

	void _SetSelectionCallBackEvent_(const std::string& funcName,
                                     const std::function<void(const std::vector<Selection::Event>&)>& func) {
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

#define SetClearSelectionCallBackEvent(functor, ...)                                                                        \
    _SetClearSelectionCallBackEvent(std::string(__FILE__) + std::to_string(__LINE__), functor, __VA_ARGS__)

	Points* GetPoints() {
		return m_Points;
	}
	CellArray* GetCells(){
		return m_Cells;
	}
	Model* GetModel(){
		return m_Model;
	}
	void SetPoints(Points* p) {
		m_Points = p;
	}
	void SetCells(CellArray* c) {
		m_Cells = c;
	}
	void SetModel(Model* m) {
		m_Model = m;
	}
protected:
	Selection() {}
	~Selection() override = default;

	//std::function<void(Event)> m_Functor;

	//Vector3f m_Position{};
	//igIndex m_PickedId{ -1 };
	//IdArray::Pointer m_SelectedIds{};
    std::map<std::string, std::function<void(const std::vector<Event>&)>> m_CallBackFunctor;
    std::map<std::string, std::function<void()>> m_ClearSelectionCallBackFunctor;

	void AddItem(const Event& event);
    std::map<Event::Type, std::map<igIndex, Event>> m_SelectedItems;


	Points* m_Points{ nullptr };
	CellArray* m_Cells{ nullptr };
	Model* m_Model{ nullptr };

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