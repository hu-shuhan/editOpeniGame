#pragma once

#include <IQComponents/Dialog/igQtChromeFramelessDialog.h>
#include <IQCore/igQtExportModule.h>

class IG_QT_MODULE_EXPORT igQtDarkFramelessMessageDialog : public igQtChromeFramelessDialog {
    Q_OBJECT
public:
    explicit igQtDarkFramelessMessageDialog(QWidget* parent, const QString& title, const QString& text,
                                            bool useInformationIcon = false);
};

IG_QT_MODULE_EXPORT void igQtShowDarkFramelessMessage(QWidget* parent, const QString& title, const QString& text,
                                                     bool useInformationIcon = false);
