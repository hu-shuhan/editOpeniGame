#pragma once

#include <AxisAlignedReflection/iGameAxisAlignedReflectionFilter.h>
#include <IQCore/igQtExportModule.h>
#include <QWidget>

class QDockWidget;

namespace Ui {
class igQtAxisAlignedReflection;
}

class IG_QT_MODULE_EXPORT igQtAxisAlignedReflectionWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtAxisAlignedReflectionWidget(QWidget* parent = nullptr);
    ~igQtAxisAlignedReflectionWidget() override;

    static QDockWidget* createDockWidget(QWidget* parent);

    iGame::AxisAlignedReflectionFilter::Plane plane() const;
    double center() const;
    bool copyInput() const;
    bool flipAllInputArrays() const;
    void resetParameters();
    void setInput(iGame::DataObject::Pointer input) { m_input = input; }
    iGame::DataObject::Pointer input() const { return m_input; }

signals:
    void applyRequested();

private:
    void updateCenterEnabled();

    Ui::igQtAxisAlignedReflection* ui;
    iGame::DataObject::Pointer m_input;
};
