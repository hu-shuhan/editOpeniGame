//
// Created by m_ky on 2024/5/22.
//

/**
 * @class   igQtProgressBarWidget
 * @brief   igQtProgressBarWidget's brief
 */
#pragma once

#include <iGameProgressObserver.h>

#include <QProgressBar>
#include <QLabel>
#include <QWidget>
#include <IQCore/igQtExportModule.h>

class IG_QT_MODULE_EXPORT igQtProgressBarWidget : public QWidget{
public:
    static constexpr const char* DEFAULT = "Ready for";
    static constexpr const char* PROCESSING = "Processing ...";
    explicit igQtProgressBarWidget(QWidget *parent = nullptr);

    void updateProgressBar(double value);

    void updateProgressBarLabel(const char* info);
private:
    void resetTextMode();

    QProgressBar* progressBar;
    QLabel *progressBarLabel;
    iGame::ProgressObserver* progressObserver;
    bool hasExternalText{false};
};