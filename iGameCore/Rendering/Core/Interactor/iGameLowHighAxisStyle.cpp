#include "iGameLowHighAxisStyle.h"
#include "iGameInteractor.h"
#include "iGamePainter3D.h"
#include "iGameScene.h"
#include <algorithm>
#include <cmath>

IGAME_NAMESPACE_BEGIN

// 屏幕空间"点 P 到线段 AB"的距离（像素），用于轴线命中判定
static float PointToScreenSegmentDistance(const igm::vec2& p, const igm::vec2& a,
                                          const igm::vec2& b) {
    const float abX = b.x - a.x;
    const float abY = b.y - a.y;
    const float len2 = abX * abX + abY * abY;
    float t = 0.0f;
    if (len2 > 1e-9f) {
        t = ((p.x - a.x) * abX + (p.y - a.y) * abY) / len2;
        t = std::max(0.0f, std::min(1.0f, t)); // 夹取到线段范围内
    }
    const float projX = a.x + abX * t - p.x;
    const float projY = a.y + abY * t - p.y;
    return std::sqrt(projX * projX + projY * projY);
}

LowHighAxisStyle::LowHighAxisStyle() {
    m_Interactor = nullptr;
    m_Scene = nullptr;
    m_Painter3D = nullptr;
    m_UpdateCallBack = nullptr;
}

LowHighAxisStyle::~LowHighAxisStyle() { ClearDraw(); }

void LowHighAxisStyle::Initialize(SmartPointer<Interactor> interactor) {
    m_Interactor = interactor;
    m_Scene = interactor ? interactor->GetScene() : nullptr;
    if (m_Scene) { m_Painter3D = m_Scene->GetPainter3D(); }
}

void LowHighAxisStyle::SetAxisPoints(const Point& low, const Point& high) {
    m_Low = low;
    m_High = high;
    UpdateDraw();
}

void LowHighAxisStyle::SetAxisVisible(bool visible) {
    m_AxisVisible = visible;
    if (!visible) {
        // 隐藏时丢弃旧句柄并收起拖拽状态，避免残留绘制
        ClearDraw();
        m_DragMode = DragMode::None;
        m_Dragging = false;
    } else {
        UpdateDraw();
    }
}

float LowHighAxisStyle::ProjectedDepth(const Point& p) const {
    igm::vec4 clip = m_MVP * igm::vec4{p[0], p[1], p[2], 1.0f};
    const float w = (clip.w != 0.0f) ? clip.w : 1.0f;
    return clip.z / w;
}

igm::vec2 LowHighAxisStyle::WorldToScreen(const Point& p) const {
    igm::vec4 clip = m_MVP * igm::vec4{p[0], p[1], p[2], 1.0f};
    const float w = (clip.w != 0.0f) ? clip.w : 1.0f;
    const float ndcX = clip.x / w;
    const float ndcY = clip.y / w;
    const float width = m_Interactor->GetWidth();
    const float height = m_Interactor->GetHeight();
    // NDC -> 像素（与 BasicStyle 的 GetNearWorldCoord 换算互逆）
    return igm::vec2{(ndcX + 1.0f) * 0.5f * width, (1.0f - ndcY) * 0.5f * height};
}

Point LowHighAxisStyle::Unproject(const igm::vec2& pos) const {
    igm::vec2 ndc(2.0f * pos.x / m_Interactor->GetWidth() - 1.0f,
                  1.0f - (2.0f * pos.y / m_Interactor->GetHeight()));
    // 沿用冻结深度（与 SingleDragStyle 的反投影一致）
    igm::vec4 pointNDC{ndc.x, ndc.y, m_SelectedNDCZ, 1.0f};
    igm::vec4 world = m_InvertedMVP * pointNDC;
    if (world.w != 0.0f) world /= world.w;
    return Point{world.x, world.y, world.z};
}

bool LowHighAxisStyle::ResolveHit(const IEvent& event) {
    m_DragMode = DragMode::None;
    // 命中判定与绘制必须使用同一投影：含模型矩阵，保证屏幕拾取与模型显示一致
    m_MVP = m_Interactor->GetMVP();
    m_InvertedMVP = m_MVP.invert();

    const igm::vec2 sLow = WorldToScreen(m_Low);
    const igm::vec2 sHigh = WorldToScreen(m_High);
    const igm::vec2 pos = event.pos;

    const float dLow = std::sqrt((pos.x - sLow.x) * (pos.x - sLow.x) +
                                 (pos.y - sLow.y) * (pos.y - sLow.y));
    const float dHigh = std::sqrt((pos.x - sHigh.x) * (pos.x - sHigh.x) +
                                  (pos.y - sHigh.y) * (pos.y - sHigh.y));

    // 1) 端点优先：命中低点或高点 -> 单点拖拽，冻结该点深度
    if (dLow <= kPointPickPixel) {
        m_DragMode = DragMode::Low;
        m_SelectedNDCZ = ProjectedDepth(m_Low);
        return true;
    }
    if (dHigh <= kPointPickPixel) {
        m_DragMode = DragMode::High;
        m_SelectedNDCZ = ProjectedDepth(m_High);
        return true;
    }
    // 2) 轴线整体：光标在屏幕线段上的距离判定 -> 整轴平移
    if (PointToScreenSegmentDistance(pos, sLow, sHigh) <= kLinePickPixel) {
        m_DragMode = DragMode::Translate;
        // 用线段中点深度作为平移参考面，使轴随视角平面同步移动
        Point mid{(m_Low[0] + m_High[0]) * 0.5f, (m_Low[1] + m_High[1]) * 0.5f,
                  (m_Low[2] + m_High[2]) * 0.5f};
        m_SelectedNDCZ = ProjectedDepth(mid);
        return true;
    }
    return false;
}

void LowHighAxisStyle::MousePressEvent(IEvent event) {
    if (!m_AxisVisible) return;
    if (event.button != MouseButton::MiddleButton) return; // 仅中键拾取/拖动
    if (ResolveHit(event)) {
        m_Dragging = true;
        m_LastPressWorld = Unproject(event.pos);
    }
}

void LowHighAxisStyle::MouseMoveEvent(IEvent event) {
    if (!m_AxisVisible || !m_Dragging) return;
    if (m_DragMode == DragMode::None) return;

    const Point newWorld = Unproject(event.pos);

    if (m_DragMode == DragMode::Low) {
        m_Low = newWorld; // 拖动低点，高点不动
    } else if (m_DragMode == DragMode::High) {
        m_High = newWorld; // 拖动高点，低点不动
    } else if (m_DragMode == DragMode::Translate) {
        // 整轴平移：按参考点位移同步两个端点
        const Point delta{newWorld[0] - m_LastPressWorld[0],
                          newWorld[1] - m_LastPressWorld[1],
                          newWorld[2] - m_LastPressWorld[2]};
        m_Low = Point{m_Low[0] + delta[0], m_Low[1] + delta[1],
                      m_Low[2] + delta[2]};
        m_High = Point{m_High[0] + delta[0], m_High[1] + delta[1],
                       m_High[2] + delta[2]};
        m_LastPressWorld = newWorld;
    }

    UpdateDraw(); // 就地重绘轴（着色由面板"应用"时统一重算）
    if (m_UpdateCallBack) { m_UpdateCallBack(m_Low, m_High); }
}

void LowHighAxisStyle::MouseReleaseEvent(IEvent event) {
    (void)event;
    m_Dragging = false;
    m_DragMode = DragMode::None;
}

void LowHighAxisStyle::ClearDraw() {
    if (!m_Painter3D) return;
    if (m_LowHandle) {
        m_Painter3D->Delete(m_LowHandle);
        m_LowHandle = 0;
    }
    if (m_HighHandle) {
        m_Painter3D->Delete(m_HighHandle);
        m_HighHandle = 0;
    }
    if (m_LineHandle) {
        m_Painter3D->Delete(m_LineHandle);
        m_LineHandle = 0;
    }
}

void LowHighAxisStyle::UpdateDraw() {
    if (!m_Painter3D && m_Scene) { m_Painter3D = m_Scene->GetPainter3D(); }
    ClearDraw();
    if (!m_AxisVisible || !m_Painter3D) return;

    // 低点与高点重合时退化为单点，只绘点不绘线（避免零长度线段）
    if (m_Low == m_High) {
        m_Painter3D->SetPen(10.0f);
        m_Painter3D->SetPen(Color::Yellow);
        m_LowHandle = m_Painter3D->DrawPoint(m_Low);
        return;
    }

    // 轴线本体（低点 -> 高点）
    m_Painter3D->SetPen(2.5f);
    m_Painter3D->SetPen(Color::Green);
    m_LineHandle = m_Painter3D->DrawLine(m_Low, m_High);

    // 两个端点的可拖拽标记
    m_Painter3D->SetPen(9.0f);
    m_Painter3D->SetPen(Color::Yellow);
    m_LowHandle = m_Painter3D->DrawPoint(m_Low);
    m_HighHandle = m_Painter3D->DrawPoint(m_High);
}

IGAME_NAMESPACE_END
