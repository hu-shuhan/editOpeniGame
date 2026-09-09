/**
 * @class igQtPackageDownloader
 * @brief Asynchronously downloads a versioned package from the local iGameVis
 *        data server, with resumable .part files.
 *
 * The downloader only transfers the package.  Extraction and OpenFile() are
 * intentionally left to the receiver of packageReady().
 */

#pragma once

#include <IQCore/igQtExportModule.h>

#include <QObject>
#include <QString>

#include <memory>

class QThread;
struct igQtPackageDownloadCancellationState;

class IG_QT_MODULE_EXPORT igQtPackageDownloader final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(igQtPackageDownloader)

public:
    explicit igQtPackageDownloader(QObject* parent = nullptr);
    ~igQtPackageDownloader() override;

    /**
     * Starts a download on a dedicated worker thread.
     *
     * @param packageId      Server-side package identifier (not a path).
     * @param cacheDirectory Existing or creatable local directory, normally on D:.
     * @return false when the request is invalid or a transfer is already running.
     */
    bool StartDownload(const QString& packageId,
                       const QString& cacheDirectory = QStringLiteral("D:/iGameVis-cs-cache"));

    bool IsRunning() const { return m_Running; }
    QString ServerAddress() const { return m_ServerAddress; }
    quint16 ServerPort() const { return m_ServerPort; }

    /** Endpoint changes are rejected while a transfer is running. */
    bool SetServerEndpoint(const QString& address, quint16 port = 34567);

    /**
     * Cancels any active transfer and waits until its worker thread has
     * completely stopped.  Call this from the downloader's owning thread.
     */
    void Shutdown();

public slots:
    /** Thread-safe: also shuts down the active socket to unblock recv(). */
    void Cancel();

signals:
    void RunningChanged(bool running);
    void StatusChanged(const QString& message);
    void PackageInfoReceived(const QString& packageId,
                             const QString& fileName,
                             const QString& versionToken,
                             quint64 fileSize);
    void DownloadStarted(quint64 resumeOffset, quint64 totalBytes);
    void DownloadProgress(quint64 receivedBytes, quint64 totalBytes);
    void PackageReady(const QString& packagePath);
    void DownloadFailed(const QString& message);
    void DownloadCancelled();

private slots:
    void OnThreadFinished();

private:
    QString m_ServerAddress{QStringLiteral("127.0.0.1")};
    quint16 m_ServerPort{34567};
    QThread* m_Thread{nullptr};
    std::shared_ptr<igQtPackageDownloadCancellationState> m_CancelState;
    bool m_Running{false};
};
