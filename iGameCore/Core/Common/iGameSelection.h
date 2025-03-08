#ifndef iGameSelection_h
#define iGameSelection_h

#include "iGameObject.h"
#include "iGameElementArray.h"
#include "iGamePoints.h"
#include "iGameIdArray.h"
#include "iGameCellArray.h"
#include <functional>

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

		Type type;
		Vector3f pos;
		igIndex pickId;
	};

	void FilterEvent(Event _event) {
		if (m_Functor) {
			m_Functor(_event);
		}
	}

	void SetPoistion(Vector3f pos) {
		m_Position = pos;
	}
	Vector3f GetPoistion() {
		return m_Position;
	}
	void SetPickedId(igIndex id) {
		m_PickedId = id;
	}
	igIndex GetPickedId() {
		return m_PickedId;
	}
	void AddSelectedId(igIndex id) {
		m_SelectedIds->AddId(id);
	}
	IdArray::Pointer GetSelectedPointIds() const {
		return m_SelectedIds;
	}
	int GetSelectedIdsSize() const {
		return m_SelectedIds->GetNumberOfIds();
	}

	void Reset() {
		m_PickedId = -1;
		m_SelectedIds->Reset();
	}

	template<typename Functor, typename... Args>
	void SetFilterEvent(Functor&& functor, Args&&... args) {
		m_Functor = std::bind(std::forward<Functor>(functor), std::forward<Args>(args)...);
	}

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

	std::function<void(Event)> m_Functor;

	Vector3f m_Position{};
	igIndex m_PickedId{ -1 };
	IdArray::Pointer m_SelectedIds{};

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
IGAME_NAMESPACE_END
#endif