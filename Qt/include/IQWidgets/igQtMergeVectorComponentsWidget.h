#pragma once

#include "MergeVectorComponents/iGameMergeVectorComponentsFilter.h"
#include "iGameDataObject.h"

#include <QWidget>

namespace Ui { class MergeVectorComponents; }

// 合并标量数组为向量(merge_vector_components) 浮动工具面板
// 选数据类型(点数据/面数据) -> X/Y/Z 三个标量下拉 -> 输出名(默认 vector) -> 执行
class igQtMergeVectorComponentsWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtMergeVectorComponentsWidget(QWidget* parent = nullptr);
    ~igQtMergeVectorComponentsWidget() override;

    // 主窗口打开面板时传入当前模型
    void SetOriginDataObject(iGame::DataObject::Pointer data);

signals:
    // 合并成功: output 为独立输出节点(输入网格的副本+合并向量), vectorName 为向量属性名
    void MergeCompleted(iGame::DataObject::Pointer output, const std::string& vectorName);

public slots:
    void OnDataTypeChanged();
    void OnExecute();

private:
    // 按当前数据类型填充 X/Y/Z 三个下拉(仅单分量标量)
    void PopulateComponents();

    Ui::MergeVectorComponents* ui;
    iGame::DataObject::Pointer m_OriginDataObject{nullptr};
};
