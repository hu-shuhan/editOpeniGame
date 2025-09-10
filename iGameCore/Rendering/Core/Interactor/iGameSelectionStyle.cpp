//
// Created by Sumzeek on 9/9/2024.
//

#include "iGameSelectionStyle.h"
#include "iGameInteractor.h"
#include "iGamePointPicker.h"

IGAME_NAMESPACE_BEGIN

SelectionStyle::SelectionStyle() {
    m_Type = SelectedType::None;

    m_Points = nullptr;
    m_Cells = nullptr;
    m_Model = nullptr;
    m_Selection = nullptr;
}

SelectionStyle::~SelectionStyle() {}

void SelectionStyle::SetSelectedType(SelectedType type) { m_Type = type; }

SelectionStyle::SelectedType SelectionStyle::GetSelectedType() const {
    return m_Type;
}


void SelectionStyle::SetSelectRadius(double selectRadius) {
    m_SelectRadius = selectRadius;
}

void SelectionStyle::SetSelectOrUnSelect(bool select) {
    m_SelectOrUnSelect = select;
}

double SelectionStyle::GetSelectRadius() const { return m_SelectRadius; }

void SelectionStyle::Initialize(SmartPointer<Interactor> interactor,
                                SmartPointer<Selection> selection) {
    BasicStyle::Initialize(interactor);
    if (selection) {
        m_Selection = selection;
        m_Points = selection->GetPoints();
        m_Cells = selection->GetCells();
        m_Model = selection->GetModel();
    }
}

void SelectionStyle::MousePressEvent(IEvent event) {
    BasicStyle::MousePressEvent(event);
}

void SelectionStyle::LeftButtonMouseMove() {}

void SelectionStyle::RightButtonMouseMove() { BasicStyle::ModelRotation(); }

void SelectionStyle::MiddleButtonMouseMove() { BasicStyle::ViewTranslation(); }

IGAME_NAMESPACE_END
