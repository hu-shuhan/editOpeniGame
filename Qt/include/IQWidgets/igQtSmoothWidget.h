#pragma once

#include "iGameDataObject.h"

#include <QWidget>

class QDockWidget;

namespace iGame
{
class Model;
}

namespace Ui
{
class igQtSmooth;
}

class igQtSmoothWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtSmoothWidget(QWidget* parent = nullptr);
    ~igQtSmoothWidget() override;

    static QDockWidget* createDockWidget(QWidget* parent);
    void setCurrentModel(iGame::Model* model);
    void resetForm();

signals:
    void cancelRequested();
    void smoothGenerated(iGame::DataObject::Pointer output);

private slots:
    void apply();
    void cancel();

private:
    Ui::igQtSmooth* ui;
    iGame::DataObject::Pointer m_currentInput;
};
