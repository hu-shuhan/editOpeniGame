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
#include <QString>
#include <QWidget>
#include <IQCore/igQtExportModule.h>

#include <atomic>
#include <cstdint>

class IG_QT_MODULE_EXPORT igQtProgressBarWidget : public QWidget{
public:
    static constexpr const char* DEFAULT = "进度条";
    static constexpr const char* PROCESSING = "Processing ...";
    explicit igQtProgressBarWidget(QWidget *parent = nullptr);

    void updateProgressBar(double value);

    void updateProgressBarLabel(const QString& info);
private:
    void postProgressBarUpdate(double value);
    void postProgressTextUpdate(const QString& info);
    void invalidatePendingUpdates();
    void resetTextMode();

    QProgressBar* progressBar;
    QLabel *progressBarLabel;
    iGame::ProgressObserver* progressObserver;
    std::atomic_uint64_t updateGeneration{0};
    bool hasExternalText{false};
};
