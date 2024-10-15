//
// Created by Sumzeek on 9/9/2024.
//

#include "iGameSelectionStyle.h"
#include "iGameInteractor.h"
#include "iGamePointPicker.h"

IGAME_NAMESPACE_BEGIN
void SelectionStyle::SetSelectedType(SelectedType type) { m_Type = type; }

SelectionStyle::SelectedType SelectionStyle::GetSelectedType() const {
    return m_Type;
}

void SelectionStyle::Initialize(Interactor* a, Selection* s) {
    BasicStyle::Initialize(a);
    if (s) { 
        m_Selection = s; 
        m_Points = s->GetPoints();
        m_Cells = s->GetCells();
        m_Model = s->GetModel();
    }
}

void SelectionStyle::MousePressEvent(IEvent _event) { 
	BasicStyle::MousePressEvent(_event);
}

void SelectionStyle::LeftButtonMouseMove() {}

void SelectionStyle::RightButtonMouseMove() { BasicStyle::ModelRotation(); }

void SelectionStyle::MiddleButtonMouseMove() { BasicStyle::ViewTranslation(); }

IGAME_NAMESPACE_END


