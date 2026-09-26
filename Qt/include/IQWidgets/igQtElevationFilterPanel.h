/**
 * @class   igQtElevationFilterPanel
 * @brief   高程 (Elevation) 实时参数面板（交互对齐 ParaView Properties 面板）
 *
 * 入口对话框首次执行 Elevation 后，由主窗口调用 BindSession 绑定会话并显示
 * 本面板。用户可在面板中实时调整参数并点击「应用」重新渲染：
 * - X/Y/Z 轴按钮：按输入模型包围盒一键铺满该轴的低/高点（互斥选中）；
 * - 低点/高点 x、y、z：标尺线段两端（t = 0 / t = 1 锚点）；
 * - 标量范围：输出值域下限/上限；
 * - 应用：校验后回写滤波器并 Execute，成功后发出 elevationApplied。
 *
 * 滤波器内部复用同一输出对象（模型树不堆叠节点），取色范围只扩不缩。
 */

#pragma once

#include <QDockWidget>

#include "Elevation/iGameElevationFilter.h"
#include "iGameDataObject.h"

class QDoubleSpinBox;
class QPushButton;
class QButtonGroup;

class igQtElevationFilterPanel : public QDockWidget {

    Q_OBJECT

public:
    explicit igQtElevationFilterPanel(QWidget* parent = nullptr);

    /** 绑定会话：记录输入模型与滤波器，回填当前参数并显示面板 */
    void BindSession(iGame::DataObject::Pointer input, iGame::ElevationFilter::Pointer filter);

    /** 解绑会话：清空记录并隐藏面板 */
    void UnbindSession();

    /** 模型树删除回调：被删对象为本会话输出节点时自动关闭面板（主窗口转接 ModelDeleted 信号） */
    void onModelDeleted(const std::string& modelName);

signals:
    /** 「应用」执行成功后发出，携带输出对象（主窗口就地刷新渲染） */
    void elevationApplied(iGame::DataObject::Pointer output);

    /** 参数校验失败或执行失败时发出，携带用户可读原因 */
    void applyFailed(const QString& reason);

private slots:
    void onApply();   // 应用：校验参数 -> 回写滤波器 -> Execute
    void onAxisX();   // X 轴：包围盒沿 X 铺满低/高点并立即应用
    void onAxisY();   // Y 轴：包围盒沿 Y 铺满低/高点并立即应用
    void onAxisZ();   // Z 轴：包围盒沿 Z 铺满低/高点并立即应用

private:
    void buildUi();                 // 构建界面（轴按钮行 + 低/高点 + 标量范围 + 应用）
    void syncFromFilter();          // 从滤波器回填参数到输入框
    void fillRangeByAxis(int axis); // 被选轴取包围盒 min/max，其余两轴取中心
    bool checkSession() const;      // 会话有效性检查（未绑定时返回 false）

    iGame::DataObject::Pointer m_Input{};        // 输入模型
    iGame::ElevationFilter::Pointer m_Filter{};  // 高程滤波器

    QDoubleSpinBox* m_LowPointSpin[3]{};      // 低点 x / y / z
    QDoubleSpinBox* m_HighPointSpin[3]{};     // 高点 x / y / z
    QDoubleSpinBox* m_RangeLowSpin{nullptr};  // 标量范围下限
    QDoubleSpinBox* m_RangeHighSpin{nullptr}; // 标量范围上限
    QPushButton* m_ApplyButton{nullptr};      // 应用按钮
    QButtonGroup* m_AxisGroup{nullptr};       // X/Y/Z 互斥选中组
};
