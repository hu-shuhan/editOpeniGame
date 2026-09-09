#include <IQCore/igQtRemotePackageLoader.h>

#include <IQCore/igQtPackageDownloader.h>
#include <IQCore/igQtRemotePackageValidation.h>
#include <DataTransfer/iGameTarZstdArchive.h>

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QLockFile>
#include <QMetaObject>
#include <QThread>
#include <QUuid>

#include <algorithm>
#include <filesystem>

namespace
{
QString SafeDirectoryStem(const QString& packageId)
{
    QString stem;
    stem.reserve(packageId.size());
    for (const QChar c : packageId) {
        if (c.isLetterOrNumber() || c == QLatin1Char('-') || c == QLatin1Char('_')) {
            stem.append(c);
        } else {
            stem.append(QLatin1Char('_'));
        }
    }
    if (stem.isEmpty()) { stem = QStringLiteral("package"); }
    return stem.left(80);
}

bool RemoveCachePath(const QString& path, QString& errorMessage)
{
    std::error_code error;
    const std::filesystem::path nativePath(path.toStdWString());
    const std::filesystem::file_status status =
            std::filesystem::symlink_status(nativePath, error);
    if (error && status.type() != std::filesystem::file_type::not_found) {
        errorMessage = QString::fromStdString(error.message());
        return false;
    }
    if (status.type() == std::filesystem::file_type::not_found) { return true; }

    // remove_all() is safe for ordinary symbolic links, but explicitly use a
    // single-entry remove for Windows junctions/reparse points too so stale
    // cache cleanup can never recurse into their target.
    if (igQtIsLinkOrReparsePoint(path) || !std::filesystem::is_directory(status)) {
        const bool removed = std::filesystem::remove(nativePath, error);
        if (error || !removed) {
            errorMessage = error ? QString::fromStdString(error.message())
                                 : QStringLiteral("path was not removed");
            return false;
        }
        return true;
    }
    std::filesystem::remove_all(nativePath, error);
    if (error) {
        errorMessage = QString::fromStdString(error.message());
        return false;
    }
    return true;
}
}

igQtRemotePackageLoader::igQtRemotePackageLoader(QObject* parent)
    : QObject(parent)
    , m_Downloader(new igQtPackageDownloader(this))
{
    connect(m_Downloader, &igQtPackageDownloader::StatusChanged,
            this, &igQtRemotePackageLoader::StatusChanged);
    connect(m_Downloader, &igQtPackageDownloader::PackageInfoReceived,
            this,
            [this](const QString&, const QString&, const QString& versionToken, quint64) {
                m_VersionToken = versionToken;
            });
    connect(m_Downloader, &igQtPackageDownloader::DownloadProgress,
            this,
            [this](quint64 received, quint64 total) {
                const double fraction = total == 0 ? 0.0 :
                        static_cast<double>(received) / static_cast<double>(total);
                // Reserve the final 10% for archive extraction and validation.
                emit ProgressChanged(std::clamp(fraction, 0.0, 1.0) * 0.9);
            });
    connect(m_Downloader, &igQtPackageDownloader::PackageReady,
            this, &igQtRemotePackageLoader::BeginExtraction);
    connect(m_Downloader, &igQtPackageDownloader::DownloadFailed,
            this,
            [this](const QString& message) {
                emit Failed(message);
                RequestFinished();
            });
    connect(m_Downloader, &igQtPackageDownloader::DownloadCancelled,
            this,
            [this]() {
                emit StatusChanged(QStringLiteral("Remote package download cancelled"));
                RequestFinished();
            });
    connect(m_Downloader, &igQtPackageDownloader::RunningChanged,
            this, [this](bool running) {
                if (!running) { TryEmitFinished(); }
            });
}

igQtRemotePackageLoader::~igQtRemotePackageLoader()
{
    m_ShuttingDown = true;
    Cancel();
    // Keep the cache lock until the downloader can no longer write its .part
    // or metadata files.  The downloader is a QObject child, so relying on
    // normal child destruction would release the lock too early below.
    m_Downloader->Shutdown();
    // finished() is queued back to this GUI-thread object.  During
    // destruction the GUI thread is blocked in wait(), so own the final
    // cleanup here instead of relying on that queued callback.
    QThread* const extractionThread = m_ExtractThread;
    m_ExtractThread = nullptr;
    if (extractionThread != nullptr) {
        disconnect(extractionThread, nullptr, this, nullptr);
        if (extractionThread->isRunning()) { extractionThread->wait(); }
        delete extractionThread;
    }
    ReleaseCacheLock();
}

bool igQtRemotePackageLoader::Start(const QString& serverAddress,
                                    quint16 serverPort,
                                    const QString& packageId,
                                    const QString& cacheDirectory)
{
    if (IsRunning() || packageId.trimmed().isEmpty() || cacheDirectory.trimmed().isEmpty()) {
        return false;
    }
    if (!m_Downloader->SetServerEndpoint(serverAddress, serverPort)) { return false; }

    // Package ids are opaque protocol values.  Whitespace can legally occur
    // in an archive filename, so only use trimmed() for the empty check and
    // preserve the exact catalog/CLI value on the wire and in cache keys.
    m_PackageId = packageId;
    m_VersionToken.clear();
    m_CacheDirectory = QDir::cleanPath(cacheDirectory.trimmed());
    m_QuarantineDirectory.clear();
    if (!QDir().mkpath(m_CacheDirectory)) {
        emit Failed(QStringLiteral("Cannot create package cache: %1").arg(m_CacheDirectory));
        return false;
    }

    auto cacheLock = std::make_unique<QLockFile>(
            QDir(m_CacheDirectory).filePath(QStringLiteral(".package.lock")));
    if (!cacheLock->tryLock(0)) {
        qint64 pid = 0;
        QString hostName;
        QString applicationName;
        QString owner;
        if (cacheLock->getLockInfo(&pid, &hostName, &applicationName)) {
            owner = QStringLiteral(" (%1 on %2, PID %3)")
                            .arg(applicationName, hostName)
                            .arg(pid);
        }
        emit Failed(QStringLiteral("Package cache is already in use%1: %2")
                            .arg(owner, m_CacheDirectory));
        return false;
    }
    m_CacheLock = std::move(cacheLock);

    m_CancelExtraction.store(false, std::memory_order_release);
    m_FinishPending = false;
    emit StatusChanged(QStringLiteral("Connecting to local data server"));
    emit ProgressChanged(0.0);
    if (!m_Downloader->StartDownload(m_PackageId, m_CacheDirectory)) {
        ReleaseCacheLock();
        return false;
    }
    return true;
}

bool igQtRemotePackageLoader::IsRunning() const
{
    // Keep a second request from replacing m_ExtractThread during the small
    // interval between CompleteExtraction() and QThread::finished().
    return m_Downloader->IsRunning() || m_Extracting || m_ExtractThread != nullptr;
}

QString igQtRemotePackageLoader::FinalizeDatasetOpen(bool success)
{
    if (m_QuarantineDirectory.isEmpty()) { return {}; }
    if (!success) {
        return QStringLiteral("The previous invalid extracted cache was preserved at: %1")
                .arg(m_QuarantineDirectory);
    }

    const QString quarantineDirectory = m_QuarantineDirectory;
    QString cleanupError;
    if (!RemoveCachePath(quarantineDirectory, cleanupError)) {
        return QStringLiteral("The model opened, but the invalid cache quarantine could not be "
                              "removed (%1): %2")
                .arg(cleanupError, quarantineDirectory);
    }
    m_QuarantineDirectory.clear();
    return {};
}

void igQtRemotePackageLoader::Cancel()
{
    m_Downloader->Cancel();
    m_CancelExtraction.store(true, std::memory_order_release);
}

void igQtRemotePackageLoader::RequestFinished()
{
    m_FinishPending = true;
    TryEmitFinished();
}

void igQtRemotePackageLoader::TryEmitFinished()
{
    if (m_ShuttingDown || !m_FinishPending || m_Downloader->IsRunning() || m_Extracting ||
        m_ExtractThread != nullptr) {
        return;
    }
    m_FinishPending = false;
    ReleaseCacheLock();
    emit Finished();
}

void igQtRemotePackageLoader::ReleaseCacheLock()
{
    if (m_CacheLock) {
        m_CacheLock->unlock();
        m_CacheLock.reset();
    }
}

QString igQtRemotePackageLoader::FailureWithQuarantine(const QString& message) const
{
    if (m_QuarantineDirectory.isEmpty()) { return message; }
    return message + QStringLiteral(" The previous invalid extracted cache was preserved at: %1")
            .arg(m_QuarantineDirectory);
}

void igQtRemotePackageLoader::BeginExtraction(const QString& packagePath)
{
    // PackageReady is queued from the download worker.  Cancellation can win
    // the race after the download completes but before this slot executes.
    if (m_CancelExtraction.load(std::memory_order_acquire)) {
        emit StatusChanged(QStringLiteral("Package extraction cancelled"));
        RequestFinished();
        return;
    }
    if (m_Extracting) {
        emit Failed(QStringLiteral("Another package extraction is already running"));
        RequestFinished();
        return;
    }
    if (m_VersionToken.isEmpty()) {
        emit Failed(QStringLiteral("Package metadata did not contain a version token"));
        RequestFinished();
        return;
    }

    const QByteArray versionKey = QCryptographicHash::hash(
            (m_PackageId + QLatin1Char('\n') + m_VersionToken).toUtf8(),
            QCryptographicHash::Sha256).toHex().left(16);
    const QString extractedRoot = QDir(m_CacheDirectory).filePath(QStringLiteral("extracted"));
    const QString finalName = SafeDirectoryStem(m_PackageId) + QLatin1Char('-') +
                              QString::fromLatin1(versionKey);
    const QString finalDirectory = QDir(extractedRoot).filePath(finalName);
    const QString stagingDirectory = finalDirectory + QStringLiteral(".extracting");

    QString existingError;
    const QFileInfo existingFinal(finalDirectory);
    const bool finalIsLink = igQtIsLinkOrReparsePoint(finalDirectory);
    if (existingFinal.exists() || finalIsLink) {
        if (existingFinal.isDir() && !finalIsLink) {
            const QString existingVtm = FindSingleVtm(finalDirectory, existingError);
            if (!existingVtm.isEmpty()) {
                emit StatusChanged(QStringLiteral("Using the extracted package cache"));
                emit ProgressChanged(1.0);
                emit DatasetReady(existingVtm);
                RequestFinished();
                return;
            }
        } else {
            existingError = finalIsLink
                    ? QStringLiteral("Extracted cache path is a symbolic link or reparse point")
                    : QStringLiteral("Extracted cache path is not a directory");
        }

        // The archive itself has already passed the downloader's size and
        // SHA-256 verification.  Keep a bad extracted tree for diagnosis,
        // move it out of the publish path atomically, then rebuild from that
        // verified archive instead of making the cache permanently unusable.
        const QString quarantineDirectory = finalDirectory + QStringLiteral(".invalid-") +
                QUuid::createUuid().toString(QUuid::WithoutBraces);
        std::error_code quarantineError;
        std::filesystem::rename(std::filesystem::path(finalDirectory.toStdWString()),
                                std::filesystem::path(quarantineDirectory.toStdWString()),
                                quarantineError);
        if (quarantineError) {
            emit Failed(QStringLiteral("Existing extracted cache is invalid (%1), and cannot "
                                       "be quarantined for recovery at %2: %3")
                                .arg(existingError,
                                     finalDirectory,
                                     QString::fromStdString(quarantineError.message())));
            RequestFinished();
            return;
        }
        m_QuarantineDirectory = quarantineDirectory;
        emit StatusChanged(
                QStringLiteral("Invalid extracted cache quarantined; rebuilding from the "
                               "verified package archive"));
    }

    QDir root;
    if (!root.mkpath(extractedRoot)) {
        emit Failed(FailureWithQuarantine(
                QStringLiteral("Cannot create extraction cache: %1").arg(extractedRoot)));
        RequestFinished();
        return;
    }
    if (QFileInfo::exists(stagingDirectory) ||
        igQtIsLinkOrReparsePoint(stagingDirectory)) {
        QString cleanupError;
        if (!RemoveCachePath(stagingDirectory, cleanupError)) {
            emit Failed(FailureWithQuarantine(
                    QStringLiteral("Cannot clear stale extraction directory %1: %2")
                            .arg(stagingDirectory, cleanupError)));
            RequestFinished();
            return;
        }
    }

    m_Extracting = true;
    emit StatusChanged(QStringLiteral("Extracting package"));

    m_ExtractThread = QThread::create(
            [this, packagePath, stagingDirectory, finalDirectory]() {
                iGame::data_transfer::TarZstdExtractOptions options;
                options.progress = [this](const iGame::data_transfer::ArchiveProgress& progress) {
                    if (m_CancelExtraction.load(std::memory_order_acquire)) { return false; }
                    double fraction = 0.0;
                    if (progress.archiveBytesTotal != 0) {
                        fraction = static_cast<double>(progress.archiveBytesProcessed) /
                                   static_cast<double>(progress.archiveBytesTotal);
                    } else if (progress.contentBytesTotal != 0) {
                        fraction = static_cast<double>(progress.contentBytesProcessed) /
                                   static_cast<double>(progress.contentBytesTotal);
                    }
                    QMetaObject::invokeMethod(
                            this,
                            [this, fraction]() {
                                emit ProgressChanged(0.9 + std::clamp(fraction, 0.0, 1.0) * 0.1);
                            },
                            Qt::QueuedConnection);
                    return true;
                };

                const auto result = iGame::data_transfer::TarZstdArchive::Extract(
                        std::filesystem::path(packagePath.toStdWString()),
                        std::filesystem::path(stagingDirectory.toStdWString()),
                        options);
                const QString error = QString::fromStdString(result.error);
                QMetaObject::invokeMethod(
                        this,
                        [this, stagingDirectory, finalDirectory, result, error]() {
                            CompleteExtraction(stagingDirectory,
                                               finalDirectory,
                                               result.success,
                                               result.cancelled,
                                               error);
                        },
                        Qt::QueuedConnection);
            });
    QThread* const extractionThread = m_ExtractThread;
    extractionThread->setObjectName(QStringLiteral("iGameVis package extraction"));
    connect(extractionThread, &QThread::finished, this, [this, extractionThread]() {
        extractionThread->deleteLater();
        if (m_ExtractThread == extractionThread) { m_ExtractThread = nullptr; }
        TryEmitFinished();
    });
    extractionThread->start();
}

void igQtRemotePackageLoader::CompleteExtraction(const QString& stagingDirectory,
                                                 const QString& finalDirectory,
                                                 bool success,
                                                 bool cancelled,
                                                 const QString& errorMessage)
{
    m_Extracting = false;
    if (!success) {
        QString cleanupError;
        const bool cleanupSucceeded = RemoveCachePath(stagingDirectory, cleanupError);
        if (cancelled) {
            QString message = QStringLiteral("Package extraction cancelled");
            if (!cleanupSucceeded) {
                message += QStringLiteral("; stale extraction cache was not removed (%1): %2")
                        .arg(cleanupError, stagingDirectory);
            }
            emit StatusChanged(FailureWithQuarantine(message));
        } else {
            QString message = QStringLiteral("Package extraction failed: %1").arg(errorMessage);
            if (!cleanupSucceeded) {
                message += QStringLiteral(" Stale extraction cache was not removed (%1): %2")
                        .arg(cleanupError, stagingDirectory);
            }
            emit Failed(FailureWithQuarantine(message));
        }
        RequestFinished();
        return;
    }

    QString validationError;
    const QString stagingVtm = FindSingleVtm(stagingDirectory, validationError);
    if (stagingVtm.isEmpty()) {
        QString cleanupError;
        const bool cleanupSucceeded = RemoveCachePath(stagingDirectory, cleanupError);
        QString message = QStringLiteral("Extracted package validation failed: %1")
                                  .arg(validationError);
        if (!cleanupSucceeded) {
            message += QStringLiteral(" Stale extraction cache was not removed (%1): %2")
                    .arg(cleanupError, stagingDirectory);
        }
        emit Failed(FailureWithQuarantine(message));
        RequestFinished();
        return;
    }

    if (!QDir().rename(stagingDirectory, finalDirectory)) {
        QString cleanupError;
        const bool cleanupSucceeded = RemoveCachePath(stagingDirectory, cleanupError);
        QString message = QStringLiteral("Cannot publish extracted package cache: %1")
                                  .arg(finalDirectory);
        if (!cleanupSucceeded) {
            message += QStringLiteral(" Stale extraction cache was not removed (%1): %2")
                    .arg(cleanupError, stagingDirectory);
        }
        emit Failed(FailureWithQuarantine(message));
        RequestFinished();
        return;
    }

    QString finalError;
    const QString finalVtm = FindSingleVtm(finalDirectory, finalError);
    if (finalVtm.isEmpty()) {
        emit Failed(FailureWithQuarantine(
                QStringLiteral("Published package validation failed: %1").arg(finalError)));
        RequestFinished();
        return;
    }

    emit StatusChanged(QStringLiteral("Package is ready; opening VTM"));
    emit ProgressChanged(1.0);
    emit DatasetReady(finalVtm);
    RequestFinished();
}

QString igQtRemotePackageLoader::FindSingleVtm(const QString& directory, QString& errorMessage) const
{
    // A root-level VTM is the package entry point.  Nested test/LOD manifests
    // are allowed.  For legacy packages without a root manifest, accept one
    // and only one recursively discovered VTM.
    const QFileInfo rootInfo(directory);
    if (!rootInfo.isDir() || igQtIsLinkOrReparsePoint(directory)) {
        errorMessage = QStringLiteral(
                "Extracted package root is not a real directory (links and reparse points are rejected)");
        return {};
    }
    const QDir packageDirectory(directory);
    const QStringList rootVtms = packageDirectory.entryList(
            QStringList{QStringLiteral("*.vtm")}, QDir::Files | QDir::NoSymLinks, QDir::Name);
    QString found;
    if (rootVtms.size() == 1) {
        found = packageDirectory.filePath(rootVtms.front());
    } else if (rootVtms.size() > 1) {
        errorMessage = QStringLiteral("Package contains more than one root VTM entry point");
        return {};
    } else {
        QDirIterator iterator(directory,
                              QStringList{QStringLiteral("*.vtm")},
                              QDir::Files | QDir::NoSymLinks,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const QString candidate = iterator.next();
            if (!found.isEmpty()) {
                errorMessage = QStringLiteral("Package contains more than one VTM entry point");
                return {};
            }
            found = QDir::cleanPath(candidate);
        }
    }
    if (found.isEmpty()) {
        errorMessage = QStringLiteral("Package does not contain a VTM entry point");
        return {};
    }

    return igQtValidateRemoteVtmManifest(found, directory, errorMessage)
            ? found : QString();
}
