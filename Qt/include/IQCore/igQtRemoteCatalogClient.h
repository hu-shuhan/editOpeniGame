/**
 * @class igQtRemoteCatalogClient
 * @brief Fetches the remote package catalog without blocking the GUI thread.
 */

#pragma once

#include <IQCore/igQtExportModule.h>

#include <QObject>
#include <QString>
#include <QVector>

#include <memory>

class QThread;
struct igQtRemoteCatalogCancellationState;

struct IG_QT_MODULE_EXPORT igQtRemoteCatalogEntry
{
    QString packageId;
    QString displayName;
    QString fileName;
    QString versionToken;
    quint64 fileSize{0};
    qint64 mtimeTicks{0};
};

Q_DECLARE_METATYPE(igQtRemoteCatalogEntry)
Q_DECLARE_METATYPE(QVector<igQtRemoteCatalogEntry>)

/**
 * Returns the isolated package cache directory used by the GUI.
 *
 * The namespace is a SHA-256 of the normalized endpoint and logical package
 * id, so two servers (or two packages with the same archive filename) cannot
 * overwrite each other's partial downloads or extracted data.
 */
IG_QT_MODULE_EXPORT QString igQtRemotePackageCacheDirectory(const QString& cacheRoot,
                                                            const QString& host,
                                                            quint16 port,
                                                            const QString& packageId);

class IG_QT_MODULE_EXPORT igQtRemoteCatalogClient final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(igQtRemoteCatalogClient)

public:
    explicit igQtRemoteCatalogClient(QObject* parent = nullptr);
    ~igQtRemoteCatalogClient() override;

    /** Starts one force-refreshed, automatically paged catalog request. */
    bool Fetch(const QString& serverAddress, quint16 serverPort);
    bool IsRunning() const { return m_Running; }

public slots:
    /** Thread-safe: also shuts down the active socket to unblock receive(). */
    void Cancel();

signals:
    void RunningChanged(bool running);
    void StatusChanged(const QString& message);
    void CatalogReady(const QVector<igQtRemoteCatalogEntry>& entries);
    void Failed(const QString& message);
    void Cancelled();

private slots:
    void OnThreadFinished();

private:
    QThread* m_Thread{nullptr};
    std::shared_ptr<igQtRemoteCatalogCancellationState> m_CancelState;
    bool m_Running{false};
};
