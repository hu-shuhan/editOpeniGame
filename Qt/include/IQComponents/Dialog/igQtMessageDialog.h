#pragma once

#include "IQComponents/Dialog/igQtFramelessDialogBase.h"

class QLabel;
class QPushButton;

class IG_QT_MODULE_EXPORT igQtMessageDialog : public igQtFramelessDialogBase {
    Q_OBJECT
public:
    explicit igQtMessageDialog(QWidget* parent = nullptr);

    void setMessage(const QString& text);
    static void information(QWidget* parent, const QString& title, const QString& text);

private:
    QLabel* m_messageLabel{nullptr};
    QPushButton* m_okButton{nullptr};
};

