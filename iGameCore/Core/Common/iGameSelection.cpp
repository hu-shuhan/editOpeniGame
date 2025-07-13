#include <iGameSelection.h>
#include <iGameModel.h>
IGAME_NAMESPACE_BEGIN

void Selection::SelectionCallBackEvent(const std::vector<Event>& _events) {
    for (auto& _event: _events) { AddItem(_event); }
    for (auto& callBackFunc: m_CallBackFunctor) { callBackFunc.second(_events); }
}

void Selection::SelectionCallBackEvent(const Event& event) {
    AddItem(event);
    for (auto& callBackFunc: m_CallBackFunctor) { callBackFunc.second({event}); }
}

void Selection::Reset() {
    if (m_Model != nullptr) {
        auto painter = m_Model->GetPainter3D();
        for (auto& selectedItem: m_SelectedItems) {
            for (auto& _event: selectedItem.second) {
                for (auto& drawHandle: _event.second.drawHandles) { painter->Delete(drawHandle); }
            }
        }
    }
    m_SelectedItems.clear();
    for (auto& callBackFunc: m_ClearSelectionCallBackFunctor) { callBackFunc.second(); }
}

void Selection::AddItem(const Event& event) {
    if (m_SelectedItems[event.type].count(event.pickId) != 0) {
        auto painter = m_Model->GetPainter3D();
        auto& handles = m_SelectedItems[event.type][event.pickId].drawHandles;
        for (auto& handle: handles) { painter->Delete(handle); }
    }
    if (event.operate == Event::Operate::Remove) {
        m_SelectedItems[event.type].erase(event.pickId);
        return;
    }
    m_SelectedItems[event.type][event.pickId] = event;
}

IGAME_NAMESPACE_END

