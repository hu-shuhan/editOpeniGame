//
// Created by m_ky on 2024/5/22.
//

#include <IQComponents/igQtProgressBarWidget.h>
#include <IQWidgets/igQtRenderWidget.h>
#include <QHBoxLayout>
#include <QColor>
#include <QEvent>
#include <QMetaObject>
#include <QThread>
#include <string>

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

    this->hide();

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, [this]() { this->hide(); });

    applyThemeStyle();

    progressObserver = iGame::ProgressObserver::Instance();

    // 事件回调发生在“发事件的那个线程”：PVD 读第一帧、切帧读播放帧都是在
    // ThreadPool 工作线程里读的（iGamePVDReader.cpp / iGameStreamingData.cpp），
    // 而 FileReader::UpdateReadProgress() 会把进度直接发给全局 ProgressObserver。
    // 在工作线程里直接 setValue()/setText() 属于跨线程操作 QWidget（UB），
    // 外部 UIA 客户端查询状态栏无障碍树时会撞在 Qt 内部崩溃。
    // 这里统一编组：GUI 线程直调，其它线程投递回本控件所在线程。
    auto dispatch = [this](auto action) {
        if (QThread::currentThread() == this->thread()) {
            action();
            return;
        }
        QMetaObject::invokeMethod(this, action, Qt::QueuedConnection);
    };

    m_ProgressObserverTag = progressObserver->AddObserver(iGame::Command::ProgressEvent,
        [this, dispatch](iGame::Object*, unsigned long, void* data)-> void {
            if (!data) { return; }
            const double value = *static_cast<double*>(data);
            dispatch([this, value]() { this->updateProgressBar(value); });
        });

    m_TextObserverTag = progressObserver->AddObserver(iGame::Command::UpdateEvent,
        [this, dispatch](iGame::Object*, unsigned long, void* data)-> void {
            const char* text = static_cast<const char*>(data);
            const std::string info = text ? std::string(text) : std::string();
            dispatch([this, info]() {
                if (info.empty()) {
                    this->resetTextMode();
                    return;
                }
                this->hasExternalText = true;
                this->updateProgressBarLabel(info.c_str());
            });
        });
}

igQtProgressBarWidget::~igQtProgressBarWidget() {
    // ProgressObserver 是全局单例：不摘掉观察者，控件析构后它还会回调到悬空的 this。
    if (progressObserver) {
        if (m_ProgressObserverTag != kInvalidObserverTag) {
            progressObserver->RemoveObserver(m_ProgressObserverTag);
        }
        if (m_TextObserverTag != kInvalidObserverTag) {
            progressObserver->RemoveObserver(m_TextObserverTag);
        }
    }
}

void igQtProgressBarWidget::resetTextMode() {
    hasExternalText = false;
    updateProgressBarLabel(DEFAULT);
}

void igQtProgressBarWidget::applyThemeStyle() {
    if (m_applyingTheme) return;
    m_applyingTheme = true;

    const bool light = igQtRenderWidget::globalLightBackground();
    const QString textColor = light ? QStringLiteral("#4A5568") : QStringLiteral("#A5ADB8");
    const QString bgColor   = light ? QStringLiteral("#FFFFFF") : QStringLiteral("#22262B");
    const QString borderCol = light ? QStringLiteral("#CBD2DC") : QStringLiteral("#343B43");
    const QString chunkCol  = light ? QStringLiteral("#2B7CD3") : QStringLiteral("#4DD0E1");

    if (progressBarLabel) {
        progressBarLabel->setStyleSheet(QStringLiteral("color: %1; background: transparent;").arg(textColor));
    }
    if (progressBar) {
        progressBar->setStyleSheet(QStringLiteral(
                "QProgressBar {"
                " color: %1;"
                " background-color: %2;"
                " border: 1px solid %3;"
                " border-radius: 4px;"
                " min-height: 8px;"
                " text-align: center;"
                "}"
                "QProgressBar::chunk { background-color: %4; }")
                .arg(textColor, bgColor, borderCol, chunkCol));
    }

    m_applyingTheme = false;
}

void igQtProgressBarWidget::changeEvent(QEvent* e) {
    if (e && e->type() == QEvent::StyleChange) {
        applyThemeStyle();
        update();
    }
    QWidget::changeEvent(e);
}

void igQtProgressBarWidget::showEvent(QShowEvent* e) {
    applyThemeStyle();
    QWidget::showEvent(e);
}

void igQtProgressBarWidget::showWithAutoHide() {
    this->show();
    if (m_hideTimer) {
        m_hideTimer->start(1500);
    }
}

void igQtProgressBarWidget::updateProgressBar(double value) {
    value = std::max(value, 0.0);
    value = std::min(value, 1.0);

    int progress = value * 100;

    if (progress < 100) {
        showWithAutoHide();
        if (!hasExternalText) {
            updateProgressBarLabel(PROCESSING);
        }
        progressBar->setValue(progress);
    } else {
        resetTextMode();
        progressBar->setValue(100);
        progressBar->setValue(0);
        this->hide();
        if (m_hideTimer) m_hideTimer->stop();
    }
}

void igQtProgressBarWidget::updateProgressBarLabel(const char* info) {
    if (!info || info[0] == '\0') {
        progressBarLabel->setText(DEFAULT);
        this->hide();
        if (m_hideTimer) m_hideTimer->stop();
        return;
    }
    showWithAutoHide();
    progressBarLabel->setText(QString::fromUtf8(info));
}
