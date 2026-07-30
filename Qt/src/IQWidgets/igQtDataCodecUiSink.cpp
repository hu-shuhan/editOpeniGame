#include <IQWidgets/igQtDataCodecUiSink.h>

#include <QMetaObject>
#include <QObject>
#include <QThread>

#include <utility>

igQtDataCodecUiSink::igQtDataCodecUiSink(
    QObject* context,
    Callback callback)
    : m_context(context),
      m_callback(std::move(callback)) {}

void igQtDataCodecUiSink::SubmitUiStatus(
    const ::datacodec::DataCodecStatusRecord& status) {
    if (m_context == nullptr || !m_callback) {
        return;
    }
    const auto utf8Text = ::datacodec::FormatDataCodecStatusText(status);
    QString text = QString::fromUtf8(
        utf8Text.data(),
        static_cast<int>(utf8Text.size()));
    if (text.isEmpty()) {
        return;
    }

    const auto context = m_context;
    const auto callback = m_callback;
    if (QThread::currentThread() == context->thread()) {
        callback(text, status.severity);
        return;
    }
    QMetaObject::invokeMethod(
        context,
        [context, callback, text = std::move(text), severity = status.severity]() {
            if (context != nullptr) {
                callback(text, severity);
            }
        },
        Qt::QueuedConnection);
}
