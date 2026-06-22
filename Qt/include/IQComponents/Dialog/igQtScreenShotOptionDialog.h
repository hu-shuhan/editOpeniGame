#pragma once

#include <IQComponents/Dialog/igQtChromeFramelessDialog.h>
#include <IQCore/igQtExportModule.h>

class QLineEdit;

class IG_QT_MODULE_EXPORT igQtScreenShotOptionDialog : public igQtChromeFramelessDialog {
    Q_OBJECT
public:
    explicit igQtScreenShotOptionDialog(QWidget* parent = nullptr);

    std::pair<int, int> getInput();

protected:
    QLineEdit* m_WidthLineEdit{nullptr};
    QLineEdit* m_HeightLineEdit{nullptr};
};