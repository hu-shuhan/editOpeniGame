/**
 * @class igQtRemotePackageLoader
 * @brief Downloads, verifies and extracts a versioned data package before
 *        exposing its VTM entry point to the normal iGameVis file loader.
 */

#pragma once

#include <IQCore/igQtExportModule.h>

#include <QObject>
#include <QString>

#include <atomic>
#include <memory>

class QLockFile;
class QThread;
class igQtPackageDownloader;

class IG_QT_MODULE_EXPORT igQtRemotePackageLoader final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(igQtRemotePackageLoader)

public:
    explicit igQtRemotePackageLoader(QObject* parent = nullptr);
    ~igQtRemotePackageLoader() override;

    bool Start(const QString& serverAddress,
               quint16 serverPort,
               const QString& packageId,
               const QString& cacheDirectory);
    bool IsRunning() const;
    QString FinalizeDatasetOpen(bool success);

public slots:
    void Cancel();

signals:
    void StatusChanged(const QString& message);
    void ProgressChanged(double progress);
    void DatasetReady(const QString& vtmPath);
    void Failed(const QString& message);
    void Finished();

private:
    void RequestFinished();
    void TryEmitFinished();
    void ReleaseCacheLock();
    QString FailureWithQuarantine(const QString& message) const;
    void BeginExtraction(const QString& packagePath);
    void CompleteExtraction(const QString& stagingDirectory,
                            const QString& finalDirectory,
                            bool success,
                            bool cancelled,
                            const QString& errorMessage);
    QString FindSingleVtm(const QString& directory, QString& errorMessage) const;

    igQtPackageDownloader* m_Downloader{nullptr};
    std::unique_ptr<QLockFile> m_CacheLock;
    QThread* m_ExtractThread{nullptr};
    std::atomic_bool m_CancelExtraction{false};
    QString m_PackageId;
    QString m_VersionToken;
    QString m_CacheDirectory;
    QString m_QuarantineDirectory;
    bool m_Extracting{false};
    bool m_FinishPending{false};
    bool m_ShuttingDown{false};
};
