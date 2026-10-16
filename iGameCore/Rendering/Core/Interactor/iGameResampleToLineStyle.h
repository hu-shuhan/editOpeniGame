#ifndef IGAMEVIS_RESAMPLETOLINE_STYLE_H
#define IGAMEVIS_RESAMPLETOLINE_STYLE_H

#include "iGameBasicStyle.h"

#include "iGameSelection.h"

IGAME_NAMESPACE_BEGIN
class Model;
class DataObject;
class Painter3D;

/**
 * @brief 重采样至直线交互风格（ResampleToLineStyle）
 *
 * 在场景中显示可拖动的起点与终点，支持鼠标拾取两个端点：
 *   Selected == 0 : 拖动起点 Origin
 *   Selected == 1 : 拖动终点 Target
 *   Selected == 2 : 整体平移线段
 *   Selected == -1: 未拾取
 * 拖动过程中通过 LineSelection 的 IG_CHANGE 事件把线段同步给控件；
 * 控件侧（igQtResampleToLine）也可以通过 LineSelection::UpdateLine() 反向驱动本风格刷新。
 */
class ResampleToLineStyle : public BasicStyle {
public:
    I_OBJECT(ResampleToLineStyle);
    static Pointer New() { return new ResampleToLineStyle; }

    void Initialize(SmartPointer<Interactor> interactor, SmartPointer<Selection> s);

    void MousePressEvent(IEvent _event) override;
    void MouseMoveEvent(IEvent _event) override;
    void MouseReleaseEvent(IEvent _event) override;

    /** 由控件/外部直接设置端点（世界坐标） */
    void SetLine(const Vector3d& orig, const Vector3d& target);

protected:
    ResampleToLineStyle();
    ~ResampleToLineStyle() override;

    void LeftButtonMouseMove(IEvent _event);
    virtual void RightButtonMouseMove() override;
    virtual void MiddleButtonMouseMove() override;

    void Draw();

    /** LineSelection 的更新回调：把选择对象中的端点同步到当前风格 */
    void UpdateLine();

    void Emit();

    SmartPointer<Model> m_Model;
    SmartPointer<DataObject> m_DataObject;
    SmartPointer<Painter3D> m_Painter3D;
    SmartPointer<LineSelection> m_Selection;

private:
    /** 屏幕坐标反投影到指定深度（NDC_Z）上的世界坐标 */
    igm::vec3 ScreenToWorldOnDepth(const igm::vec2& screenPos) const;
    /** 世界坐标投影到屏幕像素坐标 */
    bool WorldToScreen(const igm::vec3& worldPos, igm::vec2& screenPos) const;

    Vector3Tovec3 v;
    vec3ToVector3d V;

    bool LineUpdated = false;
    igm::vec3 Origin, Target, Center;
    igm::vec3 Center2Origin, Center2Target;
    igm::vec3 Intersection, TempOrigin, TempTarget;
    IGuint OrigHandle{}, TargetHandle{}, CenterHandle{};
    IGuint LineHandle{};
    int Selected = -1; // 0:起点 1:终点 2:整体 -1:未拾取

    float NDC_Z{};
    float PickRadius{0.1f}; // 世界空间拾取半径（用于端点无法投影到屏幕时的回退判定）
    float PixelPickRadius{12.0f}; // 屏幕空间拾取半径（像素）

    igm::mat4 MVP;
    igm::mat4 InvertedMVP;
};
IGAME_NAMESPACE_END
#endif
