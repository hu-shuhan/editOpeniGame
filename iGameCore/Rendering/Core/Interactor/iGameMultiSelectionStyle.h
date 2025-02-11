#ifndef IGAMEVIS_MULTI_SELECTION_STYLE_H
#define IGAMEVIS_MULTI_SELECTION_STYLE_H

#include "iGameSelectionStyle.h"

IGAME_NAMESPACE_BEGIN
class MultiSelectionStyle : public SelectionStyle {
public:
    I_OBJECT(MultiSelectionStyle);
    static Pointer New() { return new MultiSelectionStyle; }

    void MousePressEvent(IEvent event) override;

    void LeftButtonMouseMove() override;

    virtual void SelectPoints(const std::vector<igm::vec4>& planes);
    virtual void SelectFaces(const std::vector<igm::vec4>& planes);

protected:
    MultiSelectionStyle();
    ~MultiSelectionStyle() override;

    bool IsPointInFrustum(const igm::vec3& p,
                          const std::vector<igm::vec4>& planes);

    igm::mat4 m_InvertedMVP;
};
IGAME_NAMESPACE_END
#endif