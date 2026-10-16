#pragma once

#include "iGameDataObject.h"
#include "iGameModel.h"

#include <QWidget>

class QDockWidget;

namespace iGame
{
class Model;
}

namespace Ui
{
class igQtPointAndCellIds;
}

class igQtPointAndCellIdsWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtPointAndCellIdsWidget(QWidget* parent = nullptr);
    ~igQtPointAndCellIdsWidget() override;

    static QDockWidget* createDockWidget(QWidget* parent);

    void setCurrentModel(iGame::Model* model);
    void resetForm();

signals:
    void cancelRequested();
    void idsGenerated(iGame::DataObject::Pointer output);

private slots:
    void apply();
    void cancel();

private:
    // 初始化页面交互
    void initConnections();

    Ui::igQtPointAndCellIds* ui;
    iGame::Model::Pointer m_currentModel;
};
