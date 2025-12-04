#pragma once
#include "iGameBasicStyle.h"
#include "iGamePoints.h"
#include <vector>
#include <map>
#include <functional>
#include <string>
#include <iGameDynamicBox.h>
IGAME_NAMESPACE_BEGIN

class BoxStyle : public BasicStyle {
public:
    I_OBJECT(BoxStyle);
    static Pointer New() { return new BoxStyle; }

    void MousePressEvent(IEvent event) override;
    void MouseMoveEvent(IEvent event) override;

    void InitBox(const Point& p1, const Point& p2);
    void DeleteBox();

    void ToDraw();
    void ClearDraw();

    DynamicBox::Pointer GetBox();

    void _SetPointMoveCallBack(const std::string& name,
                               std::function<void()> callBack);
#define SetPointMoveCallBack(callBack)                                         \
    _SetPointMoveCallBack(std::string(__FILE__) + std::to_string(__LINE__),    \
                          callBack)

    void RemovePointMoveCallBack(const std::string& name);

private:
    void PointMoveCallBack();

protected:
    BoxStyle() = default;
    ~BoxStyle();

    int m_SelectedDirection{-1};
    IGenum m_SelectedItem{IG_NONE};
    Point m_PrePosition;
    DynamicBox::Pointer m_DynamicBox{nullptr};

    float m_MaxDis{};

    float m_SelectedNDCZ{};
    igm::mat4 m_MVP{};
    igm::mat4 m_InvertedMVP{};

    std::vector<IGuint> m_DrawHandles;
    std::map<std::string, std::function<void()>> m_PointMoveCallBacks;
};

IGAME_NAMESPACE_END