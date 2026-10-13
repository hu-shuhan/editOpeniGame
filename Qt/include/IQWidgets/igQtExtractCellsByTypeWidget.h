//
// igQtExtractCellsByTypeWidget: 「按单元类型提取」左侧工具面板
// 列出当前模型中存在的单元类型（带数量），用户勾选要提取的类型，
// 点击"提取"后通过 onApply 回调（由主窗口注入）执行 ExtractCellsByTypeFilter
// 并原地替换模型树中的提取结果（不新增模型节点）。
//

#pragma once

#include <IQCore/igQtExportModule.h>
#include <ExtractCellsByType/iGameExtractCellsByTypeFilter.h>

#include <QWidget>
#include <functional>
#include <vector>

class QCheckBox;
class QLabel;
class QPushButton;

class IG_QT_MODULE_EXPORT igQtExtractCellsByTypeWidget : public QWidget {
    Q_OBJECT
public:
    explicit igQtExtractCellsByTypeWidget(QWidget* parent = nullptr);

    // 扫描数据对象，重建"单元类型"勾选列表（默认全部勾选）
    void SetDataObject(iGame::DataObject::Pointer obj);

    // 用户当前勾选的单元类型集合
    std::vector<IGenum> GetSelectedCellTypes() const;

    // 提取按钮点击回调（主窗口注入：执行 filter + 原地替换模型）
    std::function<void()> onApply;

signals:
    void applyRequested();

private:
    void rebuildCheckBoxes();
    void onApplyClicked();

    iGame::DataObject::Pointer m_DataObject; // 当前绑定的输入模型
    std::vector<IGenum> m_Types;             // 与勾选框一一对应的单元类型
    std::vector<QCheckBox*> m_CheckBoxes;    // 每个单元类型一个勾选框
    QLabel* m_infoLabel = nullptr;
    QPushButton* m_selectAllButton = nullptr;
    QPushButton* m_clearAllButton = nullptr;
    QPushButton* m_applyButton = nullptr;
};
