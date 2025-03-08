//
// Created by m_ky on 2024/5/22.
//

#include <IQComponents/igQtProgressBarWidget.h>
#include <QHBoxLayout>

/**
 * @class   igQtProgressBarWidget
 * @brief   igQtProgressBarWidget's brief
 */
igQtProgressBarWidget::igQtProgressBarWidget(QWidget *parent) : QWidget(parent) {
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);

    progressBarLabel = new QLabel(DEFAULT,this);
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(progressBarLabel);
    layout->addWidget(progressBar);
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout);

    progressObserver = iGame::ProgressObserver::Instance();

    progressObserver->AddObserver(iGame::Command::ProgressEvent,
        [&](iGame::Object*, unsigned long, void* data)-> void {
            double value = *static_cast<double*>(data);
            this->updateProgressBar(value);
        });
        
}

void igQtProgressBarWidget::updateProgressBar(double value) {
    value = std::max(value, 0.0);
    value = std::min(value, 1.0);

    int progress = value * 100;
    

    if (progress < 100) {
        updateProgressBarLabel("Processing ...");
        progressBar->setValue(progress);
    } else {
        updateProgressBarLabel(DEFAULT);
        progressBar->setValue(100);
        progressBar->setValue(0);
    }
}

void igQtProgressBarWidget::updateProgressBarLabel(const char* info) {
    progressBarLabel->setText(info);
}
