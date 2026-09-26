/**
 * @class   igQtExtractComponentWidget
 * @brief   提取分量 filter 的参数面板（由主窗口以独立置顶弹窗承载，不占用左侧工具面板）
 */

#pragma once
#include <ui_ExtractComponentWidget.h>
#include "iGameDataObject.h"

class igQtExtractComponentWidget : public QWidget {
    Q_OBJECT

public:
    igQtExtractComponentWidget(QWidget* parent = nullptr);

public slots:
    void SetOriginDataObject(iGame::DataObject::Pointer data);
    void Apply();

signals:
    // 首次执行：模型树新增结果节点
    void DrawExtractComponentModel(iGame::DataObject::Pointer);
    // 再次执行：更新已有结果节点（刷新模型信息-数据统计）
    void UpdateExtractComponentModel(iGame::DataObject::Pointer);
    // 执行失败：错误消息
    void ApplyFailed(const QString& message);

private:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void InitInputArrayList();
    void UpdateComponentOptions();
    int GetSelectedInputDimension();
    std::string UniqueResultName(const std::string& base);
    void RebuildResultObject(iGame::DataObject::Pointer fresh);

    Ui::ExtractComponentWidget* ui;
    iGame::DataObject::Pointer m_OriginDataObject{nullptr};
    iGame::DataObject::Pointer m_ResultDataObject{nullptr};
    bool m_Generated{false};
    // Parallel to the input-array combo items: raw array name and attachment type
    // (IG_POINT / IG_CELL) so same-named Point/Cell arrays can be disambiguated.
    std::vector<QString> m_InputArrayNames;
    std::vector<int> m_InputArrayAttachments;
};
