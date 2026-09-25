#ifndef IGAMEVIS_LOWHIGH_AXIS_STYLE_H
#define IGAMEVIS_LOWHIGH_AXIS_STYLE_H

#include "iGameInteractorStyle.h"
#include "iGameVector.h"
#include "iGamePoints.h"
#include <functional>
#include <utility>

IGAME_NAMESPACE_BEGIN

class Interactor;
class Scene;
class Painter3D;

/**
 * @class   LowHighAxisStyle
 * @brief   高程"低点-高点"投影轴的屏幕显示与鼠标拖拽交互样式。
 *
 * 以"特殊交互器"的形式叠加在基础交互器（BasicStyle）之上
 * （见 iGameInteractor 的 m_SpecialInternals，FilterEvent 先分发特殊交互器、
 * 再分发基础交互器），因此不改变基础的旋转 / 平移 / 缩放。
 * 为避开按键冲突，所有拾取与拖动统一使用鼠标中键：基础样式中键为空操作
 * （iGameBasicStyle.cpp 的 MiddleButtonMouseMove 为空实现），与 BoxStyle 用
 * 中键拖框的既有范式保持一致（igQtPartFocusWidget 的 "SelectBox"）。
 *
 * 三种拖拽模式（屏幕空间按像素阈值命中判定）：
 *   - Low       ：拖动低点，高点不动；
 *   - High      ：拖动高点，低点不动；
 *   - Translate ：整轴平移，低点与高点同步移动。
 * 拖动中每一帧都通过 m_UpdateCallBack 把最新低 / 高点通知面板（用于回填输入框），
 * 本样式不触碰滤波器，是否真正重算模型着色由调用方（面板的"应用"按钮）决定。
 */
class LowHighAxisStyle : public InteractorStyle {
public:
    I_OBJECT(LowHighAxisStyle);
    static Pointer New() { return new LowHighAxisStyle; }

    void Initialize(SmartPointer<Interactor> interactor) override;

    void MousePressEvent(IEvent event) override;
    void MouseMoveEvent(IEvent event) override;
    void MouseReleaseEvent(IEvent event) override;

    /** 设置轴端点（模型坐标），随后按可见性重绘 */
    void SetAxisPoints(const Point& low, const Point& high);

    /** 获取当前轴端点（模型坐标） */
    Point GetLowPoint() const { return m_Low; }
    Point GetHighPoint() const { return m_High; }

    /** 显示 / 隐藏轴绘制（隐藏时丢弃旧句柄并收起拖拽状态） */
    void SetAxisVisible(bool visible);

    /** 注册拖拽更新回调（面板用其回填低 / 高点输入框，不触发射色） */
    void SetUpdateCallBack(
            std::function<void(const Point& low, const Point& high)> cb) {
        m_UpdateCallBack = std::move(cb);
    }

protected:
    LowHighAxisStyle();
    ~LowHighAxisStyle() override;

    void UpdateDraw();                    // 删除旧句柄并按当前端点重绘
    void ClearDraw();                     // 删除全部绘制句柄
    bool ResolveHit(const IEvent& event); // 屏幕空间命中判定，命中返回 true

    float ProjectedDepth(const Point& p) const; // 模型坐标 -> NDC 深度 (z/w)

    igm::vec2 WorldToScreen(const Point& p) const; // 模型坐标 -> 像素
    Point Unproject(const igm::vec2& pos) const;   // 像素(冻结深度) -> 模型坐标

    SmartPointer<Interactor> m_Interactor;
    SmartPointer<Scene> m_Scene;
    SmartPointer<Painter3D> m_Painter3D;

    Point m_Low{0.0f, 0.0f, 0.0f};  // 低点（模型坐标）
    Point m_High{1.0f, 0.0f, 0.0f}; // 高点（模型坐标）

    bool m_AxisVisible{false}; // 是否显示轴
    bool m_Dragging{false};    // 是否正在拖拽

    // 拖拽模式
    enum class DragMode { None, Low, High, Translate } m_DragMode{
            DragMode::None};

    float m_SelectedNDCZ{0.0f}; // 冻结的 NDC 深度
    Point m_LastPressWorld{0.0f, 0.0f, 0.0f}; // 平移模式下记录上一帧参考点

    igm::mat4 m_MVP{};
    igm::mat4 m_InvertedMVP{};

    IGuint m_LowHandle{0};  // 低点绘制句柄
    IGuint m_HighHandle{0}; // 高点绘制句柄
    IGuint m_LineHandle{0}; // 轴线绘制句柄

    // 屏幕空间命中阈值（像素）
    static constexpr float kPointPickPixel = 12.0f; // 端点拾取半径
    static constexpr float kLinePickPixel = 8.0f;   // 轴线拾取阈值

    std::function<void(const Point& low, const Point& high)> m_UpdateCallBack;
};
IGAME_NAMESPACE_END
#endif // IGAMEVIS_LOWHIGH_AXIS_STYLE_H
