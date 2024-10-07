#pragma once

#include <IQCore/igQtExportModule.h>
#include <QDialog>


class QLineEdit ;
class IG_QT_MODULE_EXPORT igQtScreenShotOptionDialog : public QDialog{
    Q_OBJECT
public:
    igQtScreenShotOptionDialog(QWidget *parent = nullptr);

    std::pair<int, int> getInput();

protected:

    QLineEdit* m_WidthLineEdit, *m_HeightLineEdit;
};