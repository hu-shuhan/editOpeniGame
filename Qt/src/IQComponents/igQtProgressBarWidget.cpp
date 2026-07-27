//
// Created by m_ky on 2024/5/22.
//

#include <IQComponents/igQtProgressBarWidget.h>
#include <QHBoxLayout>
#include <QMetaObject>
#include <QPointer>
#include <QThread>

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
    QPointer<igQtProgressBarWidget> self(this);

   progressObserver->AddObserver(iGame::Command::ProgressEvent,
        [self](iGame::Object*, unsigned long, void* data)-> void {
            if (!self) return;
            double value = *static_cast<double*>(data);
            self->postProgressBarUpdate(value);
        });

    progressObserver->AddObserver(iGame::Command::UpdateEvent,
        [self](iGame::Object*, unsigned long, void* data)-> void {
            if (!self) return;
            const char* text = static_cast<const char*>(data);
            self->postProgressTextUpdate(text != nullptr ? QString::fromUtf8(text) : QString());
        });
}

void igQtProgressBarWidget::postProgressBarUpdate(double value) {
    if (QThread::currentThread() == thread()) {
        if (value <= 0.0) invalidatePendingUpdates();
        updateProgressBar(value);
        return;
    }

    QPointer<igQtProgressBarWidget> self(this);
    const std::uint64_t generation = updateGeneration.load(std::memory_order_acquire);
    QMetaObject::invokeMethod(this, [self, value, generation]() {
        if (self && generation != self->updateGeneration.load(std::memory_order_acquire)) return;
        if (self) self->updateProgressBar(value);
    }, Qt::QueuedConnection);
}

void igQtProgressBarWidget::postProgressTextUpdate(const QString& info) {
    if (QThread::currentThread() == thread()) {
        if (info.isEmpty()) {
            invalidatePendingUpdates();
            resetTextMode();
            return;
        }
        hasExternalText = true;
        updateProgressBarLabel(info);
        return;
    }

    QPointer<igQtProgressBarWidget> self(this);
    const std::uint64_t generation = updateGeneration.load(std::memory_order_acquire);
    QMetaObject::invokeMethod(this, [self, info, generation]() {
        if (!self) return;
        if (generation != self->updateGeneration.load(std::memory_order_acquire)) return;
        if (info.isEmpty()) {
            self->resetTextMode();
            return;
        }
        self->hasExternalText = true;
        self->updateProgressBarLabel(info);
    }, Qt::QueuedConnection);
}

void igQtProgressBarWidget::invalidatePendingUpdates() {
    updateGeneration.fetch_add(1, std::memory_order_acq_rel);
}

void igQtProgressBarWidget::resetTextMode() {
    hasExternalText = false;
    updateProgressBarLabel(QString::fromUtf8(DEFAULT));
}

void igQtProgressBarWidget::updateProgressBar(double value) {
    value = std::max(value, 0.0);
    value = std::min(value, 1.0);

    int progress = value * 100;
    

    if (progress < 100) {
        if (!hasExternalText) {
            updateProgressBarLabel(QString::fromUtf8(PROCESSING));
        }
        progressBar->setValue(progress);
    } else {
        resetTextMode();
        progressBar->setValue(100);
        progressBar->setValue(0);
    }
}

void igQtProgressBarWidget::updateProgressBarLabel(const QString& info) {
    if (info.isEmpty()) {
        progressBarLabel->setText(QString::fromUtf8(DEFAULT));
        return;
    }
    progressBarLabel->setText(info);
}
