#ifndef igQtDataCodecUiSink_h
#define igQtDataCodecUiSink_h

#include "DataCodec/API/Output/DataCodecOutputSinks.h"
#include <IQCore/igQtExportModule.h>

#include <QPointer>
#include <QString>

#include <functional>

class QObject;

class IG_QT_MODULE_EXPORT igQtDataCodecUiSink final
    : public ::datacodec::IDataCodecUiSink {
public:
    using Callback = std::function<void(
        const QString&,
        ::datacodec::DataCodecStatusSeverity)>;

    igQtDataCodecUiSink(QObject* context, Callback callback);

    void SubmitUiStatus(
        const ::datacodec::DataCodecStatusRecord& status) override;

private:
    QPointer<QObject> m_context;
    Callback m_callback;
};

#endif
