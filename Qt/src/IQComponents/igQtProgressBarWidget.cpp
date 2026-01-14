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

    progressBarLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    progressBarLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    progressBarLabel->setMinimumWidth(220);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(progressBarLabel);
    layout->addWidget(progressBar);
    layout->setStretch(0, 2);
    layout->setStretch(1, 3);
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout);

    progressObserver = iGame::ProgressObserver::Instance();

   progressObserver->AddObserver(iGame::Command::ProgressEvent,
        [&](iGame::Object*, unsigned long, void* data)-> void {
            double value = *static_cast<double*>(data);
            this->updateProgressBar(value);
        });

    progressObserver->AddObserver(iGame::Command::UpdateEvent,
        [&](iGame::Object*, unsigned long, void* data)-> void {
            const char* text = static_cast<const char*>(data);
            if (!text || text[0] == '\0') {
                resetTextMode();
                return;
            }
            hasExternalText = true;
            this->updateProgressBarLabel(text);
        });
}

void igQtProgressBarWidget::resetTextMode() {
    hasExternalText = false;
    updateProgressBarLabel(DEFAULT);
}

void igQtProgressBarWidget::updateProgressBar(double value) {
    value = std::max(value, 0.0);
    value = std::min(value, 1.0);

    int progress = value * 100;
    

    if (progress < 100) {
        if (!hasExternalText) {
            updateProgressBarLabel(PROCESSING);
        }
        progressBar->setValue(progress);
    } else {
        resetTextMode();
        progressBar->setValue(100);
        progressBar->setValue(0);
    }
}

void igQtProgressBarWidget::updateProgressBarLabel(const char* info) {
    if (!info || info[0] == '\0') {
        progressBarLabel->setText(DEFAULT);
        return;
    }
    progressBarLabel->setText(QString::fromUtf8(info));
}
