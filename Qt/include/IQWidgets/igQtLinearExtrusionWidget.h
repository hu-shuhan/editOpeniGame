/**
 * @class   igQtLinearExtrusionWidget
 * @brief   线性拉伸 filter 的参数面板（由主窗口以独立置顶弹窗承载，不占用左侧工具面板）
 */

#pragma once
#include <ui_LinearExtrusionWidget.h>
#include "iGameDataObject.h"

class igQtLinearExtrusionWidget : public QWidget {
    Q_OBJECT

public:
    igQtLinearExtrusionWidget(QWidget* parent = nullptr);

public slots:
    void SetOriginDataObject(iGame::DataObject::Pointer data);
    void Apply();

signals:
    // 首次执行：模型树新增结果节点
    void DrawLinearExtrusionModel(iGame::DataObject::Pointer);
    // 再次执行：更新已有结果节点（刷新模型信息-数据统计）
    void UpdateLinearExtrusionModel(iGame::DataObject::Pointer);
    // 执行失败：错误消息
    void ApplyFailed(const QString& message);

private:
    bool eventFilter(QObject* obj, QEvent* event) override;
    // 按当前拉伸模式启用/禁用输入框：Normal 只用缩放系数，Vector 追加方向向量，Point 追加基点
    void UpdateParameterState();
    std::string UniqueResultName(const std::string& inputName);
    void RebuildResultObject(iGame::DataObject::Pointer fresh);

    Ui::LinearExtrusionWidget* ui;
    iGame::DataObject::Pointer m_OriginDataObject{nullptr};
    iGame::DataObject::Pointer m_ResultDataObject{nullptr};
    bool m_Generated{false};
};
