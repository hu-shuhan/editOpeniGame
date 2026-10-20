#ifndef IGQTTRANSFORMBOXCONTROLLER_H
#define IGQTTRANSFORMBOXCONTROLLER_H

#include <QObject>
#include <QPointer>
#include <array>

#include <iGameBoxStyle.h>
#include <iGameDataObject.h>
#include <iGameDynamicBox.h>
#include <iGameScene.h>

class QLineEdit;
class igQtFilterDialogDockWidget;
class igQtModelDrawWidget;

class igQtTransformBoxController : public QObject {
    Q_OBJECT
public:
    /** 参数顺序：平移 X/Y/Z、旋转 X/Y/Z（度）、缩放 X/Y/Z */
    static constexpr int kParamCount = 9;
    /** 盒子在交互器上的注册名（与"框选"的 SelectBox 区分） */
    static const char* kInteractorName() { return "TransformBox"; }
    /** 拖动回调的注册名 */
    static const char* kCallbackName() { return "igQtTransformBox"; }

    igQtTransformBoxController(iGame::DataObject::Pointer input,
                               igQtModelDrawWidget* renderWidget,
                               igQtFilterDialogDockWidget* dialog,
                               const std::array<int, kParamCount>& parameterIds,
                               QObject* parent = nullptr);
    ~igQtTransformBoxController() override;

    /** 面板已存在时把它提到最前 */
    void FocusPanel();

    /** 面板是否具备交互条件（盒子建立成功） */
    bool HasBox() const { return m_BoxStyle != nullptr; }

    /** 重置：参数回到单位变换，盒子回到初始包围盒（模型不变，需要应用才生效） */
    void Reset();

    /** 用当前参数对初始输入执行一次变换（参数与盒子保持不变） */
    void Apply();

signals:
    /** 应用成功：输出对象已按 "<原名>_Transform" 命名，交由主窗口加入模型树 */
    void Applied(iGame::DataObject::Pointer output);
    /** 应用失败（输入为空 / 过滤器执行失败） */
    void Failed();
    /** 面板关闭：主窗口应释放本控制器 */
    void Closed();

private:
    /** 内部：跑一次 TransformFilter，返回输出对象（失败返回空指针） */
    iGame::DataObject::Pointer RunFilter();

    bool BuildBox();
    void DestroyBox();

    /** 盒子 -> 参数（拖拽后调用），并刷新输入框 */
    void SyncBoxToParameters();
    /** 参数 -> 盒子（输入框改动后调用），并重绘 */
    void SyncParametersToBox();

    /** 读取初始输入对象的包围盒（中心 + 边长，带退化保护） */
    void ComputeInitialBounds();
    /** 从 9 个输入框读取参数，解析失败时返回 false（保留旧值） */
    bool ReadParametersFromDialog();
    /** 把参数写回 9 个输入框（带防循环标志） */
    void WriteParametersToDialog();

    void RedrawBox();

private:
    iGame::DataObject::Pointer m_Input{};                 // 初始模型（锁定，不随选中变化）
    iGame::Scene::Pointer m_Scene{};
    QPointer<igQtModelDrawWidget> m_RenderWidget;
    QPointer<igQtFilterDialogDockWidget> m_Dialog;
    QLineEdit* m_Edits[kParamCount]{};

    iGame::BoxStyle::Pointer m_BoxStyle{};
    iGame::DynamicBox::Pointer m_Box{};

    double m_Center0[3]{0.0, 0.0, 0.0};   // 初始包围盒中心（= 变换轴心）
    double m_Length0[3]{1.0, 1.0, 1.0};   // 初始包围盒边长（缩放基准）
    double m_Parameters[kParamCount]{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0};

    bool m_PreventSync{false};            // 防循环：程序写输入框时不触发"参数 -> 盒子"
};

#endif // IGQTTRANSFORMBOXCONTROLLER_H
