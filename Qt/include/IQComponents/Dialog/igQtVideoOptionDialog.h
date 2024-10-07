/**
 * @class   igQtVideoOptionDialog
 * @brief   igQtVideoOptionDialog's brief
 */

#pragma once
#if defined(FFMPEG_ENABLE)
#include <IQCore/igQtExportModule.h>
#include <QDialog>
#include <FFMPEG/iGameFFMPEGVideoWriter.h>

class QLineEdit ;
class IG_QT_MODULE_EXPORT igQtVideoOptionDialog : public QDialog{
Q_OBJECT
public:
    igQtVideoOptionDialog(QWidget *parent = nullptr);

    iGame::VideoInputInfo getInput();

protected:

    QLineEdit* m_Width_LineEdit, *m_Height_LineEdit;
    QLineEdit* m_frameRate_LineEdit, *m_bitRate_LineEdit;
};
#endif