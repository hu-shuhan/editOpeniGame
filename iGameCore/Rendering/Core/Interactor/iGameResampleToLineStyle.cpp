#include "iGameResampleToLineStyle.h"

#include "iGameInteractor.h"
#include "iGameScene.h"

#include <algorithm>
#include <cmath>

IGAME_NAMESPACE_BEGIN

namespace {
/** 屏幕空间：点到线段的距离（像素） */
float ScreenDistanceToSegment(const igm::vec2& p, const igm::vec2& a, const igm::vec2& b) {
    const igm::vec2 ab = b - a;
    const float denom = ab.x * ab.x + ab.y * ab.y;
    if (denom < 1e-8f) { return (p - a).length(); }
    float t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / denom;
    t = std::max(0.0f, std::min(1.0f, t));
    const igm::vec2 closest(a.x + ab.x * t, a.y + ab.y * t);
    return (p - closest).length();
}
} // namespace

ResampleToLineStyle::ResampleToLineStyle() {
    Origin = igm::vec3(-1.0f, 0.0f, 0.0f);
    Target = igm::vec3(1.0f, 0.0f, 0.0f);
    Center = (Origin + Target) * 0.5f;
    Center2Origin = Origin - Center;
    Center2Target = Target - Center;
}

ResampleToLineStyle::~ResampleToLineStyle() {
    if (m_Painter3D == nullptr) { return; }
    if (OrigHandle != 0) { m_Painter3D->Delete(OrigHandle); }
    if (TargetHandle != 0) { m_Painter3D->Delete(TargetHandle); }
    if (CenterHandle != 0) { m_Painter3D->Delete(CenterHandle); }
    if (LineHandle != 0) { m_Painter3D->Delete(LineHandle); }
}

/* ------------------------------------------------------------------ */
/* 初始化                                                              */
/* ------------------------------------------------------------------ */
void ResampleToLineStyle::Initialize(SmartPointer<Interactor> interactor, SmartPointer<Selection> s) {
    BasicStyle::Initialize(interactor);

    m_Selection = DynamicCast<LineSelection>(s);
    if (m_Selection == nullptr && s != nullptr) {
        m_Selection = static_cast<LineSelection*>(s.GetPointer());
    }

    m_Painter3D = interactor->GetPainter3D();
    m_DataObject = interactor->GetDataObject();
    if (m_Painter3D == nullptr || m_DataObject == nullptr) { return; }

    if (m_Interactor != nullptr && m_Interactor->GetScene() != nullptr) {
        m_Model = m_Interactor->GetScene()->GetCurrentModel();
    }

    if (m_Selection != nullptr) {
        // 反向驱动：控件通过 LineSelection::UpdateLine() 通知交互器刷新
        m_Selection->SetUpdateFunction([this] { this->UpdateLine(); });
        m_Selection->SetModel(m_Model.get());
        Origin = v(m_Selection->Orig);
        Target = v(m_Selection->Target);
    }

    Center = (Origin + Target) * 0.5f;
    Center2Origin = Origin - Center;
    Center2Target = Target - Center;

    // 拾取半径根据模型尺度自适应
    const auto& bbox = m_DataObject->GetBoundingBox();
    const double diagLength = (bbox.max - bbox.min).length();
    PickRadius = static_cast<float>(diagLength * 0.02);
    if (PickRadius <= 0.0f) { PickRadius = 0.1f; }

    m_Painter3D->SetTotallyHide(false);
    m_Painter3D->SetPen(Pen::Style::SolidLine);
    m_Painter3D->SetBrush(Brush::Style::NoBrush);

    Draw();
}

/* ------------------------------------------------------------------ */
/* 外部设置端点                                                        */
/* ------------------------------------------------------------------ */
void ResampleToLineStyle::SetLine(const Vector3d& orig, const Vector3d& target) {
    Origin = v(orig);
    Target = v(target);
    Center = (Origin + Target) * 0.5f;
    Center2Origin = Origin - Center;
    Center2Target = Target - Center;
    if (m_Selection != nullptr) {
        m_Selection->Orig = V(Origin);
        m_Selection->Target = V(Target);
    }
    Draw();
}

/* ------------------------------------------------------------------ */
/* 绘制                                                                */
/* ------------------------------------------------------------------ */
void ResampleToLineStyle::Draw() {
    if (m_Painter3D == nullptr) { return; }

    if (OrigHandle != 0) {
        m_Painter3D->Delete(OrigHandle);
        OrigHandle = 0;
    }
    if (TargetHandle != 0) {
        m_Painter3D->Delete(TargetHandle);
        TargetHandle = 0;
    }
    if (CenterHandle != 0) {
        m_Painter3D->Delete(CenterHandle);
        CenterHandle = 0;
    }
    if (LineHandle != 0) {
        m_Painter3D->Delete(LineHandle);
        LineHandle = 0;
    }

    // 采样线段本身
    m_Painter3D->SetPen(2);
    m_Painter3D->SetPen(Color::White);
    LineHandle = m_Painter3D->DrawLine(V(Origin), V(Target));

    // 可拖动的起点与终点（被拾取时高亮）
    m_Painter3D->SetPen(16);
    m_Painter3D->SetPen(Selected == 0 ? Color::Yellow : Color::Green);
    OrigHandle = m_Painter3D->DrawPoint(V(Origin));

    m_Painter3D->SetPen(Selected == 1 ? Color::Yellow : Color::Red);
    TargetHandle = m_Painter3D->DrawPoint(V(Target));

    // 整体平移时显示中点，提示可以整体拖动
    if (Selected == 2) {
        m_Painter3D->SetPen(Color::Yellow);
        CenterHandle = m_Painter3D->DrawPoint(V(Center));
    }
}

/* ------------------------------------------------------------------ */
/* 事件                                                                */
/* ------------------------------------------------------------------ */
void ResampleToLineStyle::MousePressEvent(IEvent _event) {
    BasicStyle::MousePressEvent(_event);

    if (_event.button != LeftButton || m_Interactor == nullptr) { return; }

    MVP = m_Interactor->GetMVP();
    InvertedMVP = MVP.invert();

    const igm::vec2 pos = _event.pos;

    auto pickEndpoint = [&](int which) {
        Selected = which;
        const igm::vec3& handle = (which == 0) ? Origin : Target;
        const igm::vec4 afterMVP = MVP * igm::vec4{handle, 1.0f};
        if (std::fabs(afterMVP.w) > 1e-12f) { NDC_Z = (afterMVP / afterMVP.w).z; }
        Center2Origin = Origin - Center;
        Center2Target = Target - Center;
    };

    auto pickBody = [&]() {
        Selected = 2;
        const igm::vec4 afterMVP = MVP * igm::vec4{Center, 1.0f};
        if (std::fabs(afterMVP.w) > 1e-12f) { NDC_Z = (afterMVP / afterMVP.w).z; }
        TempOrigin = Origin;
        TempTarget = Target;
        Intersection = ScreenToWorldOnDepth(pos);
    };

    igm::vec2 originScreen, targetScreen;
    const bool hasOrigin = WorldToScreen(Origin, originScreen);
    const bool hasTarget = WorldToScreen(Target, targetScreen);

    if (hasOrigin && hasTarget) {
        // 屏幕空间拾取：先端点，后线段本体
        const float dOrigin = (pos - originScreen).length();
        const float dTarget = (pos - targetScreen).length();

        if (dOrigin <= PixelPickRadius || dTarget <= PixelPickRadius) {
            pickEndpoint(dOrigin <= dTarget ? 0 : 1);
            return;
        }
        if (ScreenDistanceToSegment(pos, originScreen, targetScreen) <= PixelPickRadius) {
            pickBody();
            return;
        }
        Selected = -1;
        return;
    }

    // 端点无法投影到屏幕时的回退：使用世界空间中的视线距离判定
    const igm::vec3 nearPoint = GetNearWorldCoord(pos, InvertedMVP);
    const igm::vec3 farPoint = GetFarWorldCoord(pos, InvertedMVP);

    if (DistancePointToLine(Origin, nearPoint, farPoint) < PickRadius) {
        pickEndpoint(0);
    } else if (DistancePointToLine(Target, nearPoint, farPoint) < PickRadius) {
        pickEndpoint(1);
    } else if (TwoLineIntersection(Origin, Target, nearPoint, farPoint, Intersection)) {
        pickBody();
    } else {
        Selected = -1;
    }
}

void ResampleToLineStyle::MouseMoveEvent(IEvent _event) {
    m_NewPoint2D = _event.pos;

    switch (m_MouseMode) {
        case LeftButton: {
            if (Selected == -1) { return; }
            LeftButtonMouseMove(_event);
        } break;
        case RightButton: {
            RightButtonMouseMove();
        } break;
        case MiddleButton: {
            MiddleButtonMouseMove();
        } break;
        default:
            break;
    }

    m_OldPoint2D = m_NewPoint2D;
}

void ResampleToLineStyle::MouseReleaseEvent(IEvent _event) {
    BasicStyle::MouseReleaseEvent(_event);
    if (Selected != -1) { Emit(); }
    Selected = -1;
    Draw();
}

void ResampleToLineStyle::LeftButtonMouseMove(IEvent _event) {
    const igm::vec3 newWorld = ScreenToWorldOnDepth(_event.pos);

    if (Selected == 0) {
        // 拖动起点
        Origin = newWorld;
    } else if (Selected == 1) {
        // 拖动终点
        Target = newWorld;
    } else if (Selected == 2) {
        // 整体平移
        const igm::vec3 delta = newWorld - Intersection;
        Origin = TempOrigin + delta;
        Target = TempTarget + delta;
    } else {
        return;
    }

    Center = (Origin + Target) * 0.5f;
    Center2Origin = Origin - Center;
    Center2Target = Target - Center;

    LineUpdated = true;
    Draw();
    Emit();
}

void ResampleToLineStyle::RightButtonMouseMove() { BasicStyle::ModelRotation(); }

void ResampleToLineStyle::MiddleButtonMouseMove() { BasicStyle::ViewTranslation(); }

/* ------------------------------------------------------------------ */
/* 与 LineSelection 同步                                               */
/* ------------------------------------------------------------------ */
void ResampleToLineStyle::Emit() {
    if (m_Selection == nullptr) { return; }
    m_Selection->Orig = V(Origin);
    m_Selection->Target = V(Target);
    m_Selection->Selected = Selected;
    m_Selection->SelectionCallBackEvent(IG_CHANGE);
    LineUpdated = false;
}

void ResampleToLineStyle::UpdateLine() {
    if (m_Selection == nullptr) { return; }

    Origin = v(m_Selection->Orig);
    Target = v(m_Selection->Target);
    Center = (Origin + Target) * 0.5f;
    Center2Origin = Origin - Center;
    Center2Target = Target - Center;

    Draw();
}

/* ------------------------------------------------------------------ */
/* 坐标变换辅助                                                        */
/* ------------------------------------------------------------------ */
igm::vec3 ResampleToLineStyle::ScreenToWorldOnDepth(const igm::vec2& screenPos) const {
    if (m_Interactor == nullptr) { return Center; }

    const float width = m_Interactor->GetWidth();
    const float height = m_Interactor->GetHeight();
    if (width <= 0.0f || height <= 0.0f) { return Center; }

    const igm::vec2 ndc(2.0f * screenPos.x / width - 1.0f, 1.0f - 2.0f * screenPos.y / height);
    const igm::vec4 ndcPoint{ndc, NDC_Z, 1.0f};
    igm::vec4 world = InvertedMVP * ndcPoint;
    if (std::fabs(world.w) < 1e-12f) { return Center; }
    world = world / world.w;
    return world.xyz();
}

bool ResampleToLineStyle::WorldToScreen(const igm::vec3& worldPos, igm::vec2& screenPos) const {
    if (m_Interactor == nullptr) { return false; }

    const igm::vec4 clip = MVP * igm::vec4{worldPos, 1.0f};
    if (clip.w <= 1e-12f) { return false; }

    const igm::vec3 ndc = clip.xyz() / clip.w;
    const float width = m_Interactor->GetWidth();
    const float height = m_Interactor->GetHeight();
    if (width <= 0.0f || height <= 0.0f) { return false; }

    screenPos.x = (ndc.x + 1.0f) * 0.5f * width;
    screenPos.y = (1.0f - ndc.y) * 0.5f * height;
    return true;
}

IGAME_NAMESPACE_END
