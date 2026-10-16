/**
 * @class   igQtGenerateProcessIdsWidget
 * @brief   生成进程ID filter 的工具面板（对齐提取分量/轮廓提取的交互模式）
 */

#pragma once
#include <ui_GenerateProcessIdsWidget.h>
#include "iGameDataObject.h"

class igQtGenerateProcessIdsWidget : public QWidget {
    Q_OBJECT

public:
    igQtGenerateProcessIdsWidget(QWidget* parent = nullptr);

public slots:
    void SetOriginDataObject(iGame::DataObject::Pointer data);
    void Apply();

signals:
    // 首次执行：模型树新增结果节点
    void DrawProcessIdsModel(iGame::DataObject::Pointer);
    // 再次执行：更新已有结果节点（刷新模型信息-数据统计）
    void UpdateProcessIdsModel(iGame::DataObject::Pointer);
    // 执行失败：错误消息
    void ApplyFailed(const QString& message);

private:
    std::string UniqueResultName(const std::string& inputName);
    void RebuildResultObject(iGame::DataObject::Pointer fresh);

    Ui::GenerateProcessIdsWidget* ui;
    iGame::DataObject::Pointer m_OriginDataObject{nullptr};
    iGame::DataObject::Pointer m_ResultDataObject{nullptr};
    bool m_Generated{false};
};
