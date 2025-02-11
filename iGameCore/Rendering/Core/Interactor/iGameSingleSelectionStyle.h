#ifndef IGAMEVIS_SINGLE_SELECTION_STYLE_H
#define IGAMEVIS_SINGLE_SELECTION_STYLE_H

#include "iGameSelectionStyle.h"

IGAME_NAMESPACE_BEGIN
class SingleSelectionStyle : public SelectionStyle {
public:
    I_OBJECT(SingleSelectionStyle);
    static Pointer New() { return new SingleSelectionStyle; }

    void MousePressEvent(IEvent _event) override;

    void SelectPoint(igm::vec2 pos);
    void SelectFace(igm::vec2 pos);

protected:
    SingleSelectionStyle();
    ~SingleSelectionStyle() override;
};
IGAME_NAMESPACE_END
#endif