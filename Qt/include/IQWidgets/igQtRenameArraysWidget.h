//
// Created by Ayanami on 2026/9/30.
//
#pragma once

#include "RenameArrays/iGameRenameArrays.h"

#include <ui_RenameArrays.h>

#include <QWidget>

class igQtModelDialogWidget;

/**
 * @brief 重命名数组面板（对标 ParaView 的 Rename Arrays）
 *
 * 左列只读显示输入模型的 Point / Cell 数组原名，右列可编辑新名（默认等于原名）。
 * 点「执行」后由 RenameArrays filter 生成一个**全新的网格**（输入对象不被修改）
 * 并加入模型树。
 */
class igQtRenameArrays : public QWidget {
    Q_OBJECT
public:
    igQtRenameArrays(igQtModelDialogWidget* modelTreeWidget, QWidget* parent = nullptr);
    ~igQtRenameArrays() override;

    // 在模型树中选中进行输入：按当前模型刷新两个列表
    void SetPointArrays();
    void SetCellArrays();

public slots:
    void BindCurrentModel();
    void Execute();

protected:
    void showEvent(QShowEvent* event) override;

private:
    // 检查窗口中的模型树控件
    igQtModelDialogWidget* ResolveModelTreeWidget();

    Ui::RenameArraysWidget* ui;

    igQtModelDialogWidget* m_modelTreeWidget{nullptr};

    iGame::DataObject::Pointer m_currentModel{nullptr};
    iGame::AttributeSet::Pointer m_AttributeSet{nullptr};
};
