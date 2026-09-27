/**
 * @class   igQtFileLoader
 * @brief   igQtFileLoader's brief
 */

#pragma once
#include "iGameSceneManager.h"

#include <QFileDialog>
#include <QString>
#include <memory>
#include <IQCore/igQtExportModule.h>

using namespace iGame;

class igQtRemotePackageLoader;
class igQtRenderWidget;
struct igQtResidentRemoteState;

class IG_QT_MODULE_EXPORT igQtFileLoader : public QObject
{
	Q_OBJECT
public:
	igQtFileLoader(QObject* parent = nullptr);
	~igQtFileLoader() override;

public:
	void LoadFile();
    void LoadOnlineS();
    void LoadOnlineC();
	// Core file paths are UTF-8 encoded. File-system boundaries perform the
	// native conversion (UTF-16 on Windows).
	void OpenFile(const std::string& fileName);
    bool OpenRemotePackage(const QString& serverAddress,
                           quint16 serverPort,
                           const QString& packageId,
                           const QString& cacheDirectory = QStringLiteral("D:/iGameVis-cs-cache"));
    bool IsRemotePackageRunning() const;
    bool PreloadRemotePackage(const QString& serverAddress, quint16 serverPort,
                              const QString& packageId, const QString& cacheDirectory);
    QString RemoteMemoryCacheStatus() const;
    quint64 RemoteMemoryCacheBytes() const;
    quint64 RemoteMemoryCacheLimitBytes() const;
    bool HasRemoteMemoryCache() const;
    void SetRemoteMemoryCacheLimitBytes(quint64 bytes);
    void SetRemoteCpuOnlyCacheEnabled(bool enabled);
    void ReleaseDetachedRemoteGpuResources();
    // Automatic, single-entry static-data cache for C/S opens. Owns the parsed
    // data independently of Scene/tree membership; never duplicates mesh arrays.
    // Same process/context only. Restart falls back to the existing disk cache.
    void SetRemoteMemoryCacheEnabled(bool enabled);
    void SetRemoteCacheRenderWidget(igQtRenderWidget* widget);
    void SetRemoteCacheStrictValidation(bool strict);
    void InvalidateRemoteMemoryCache(const QString& reason);
    bool DetachResidentRemoteModelForBenchmark();
    void OpenFiles(const QStringList& fileNames);
    void OpenSplineFile(const std::string& fileName);
    void OpenODBFile(const std::string& fileName);
    void OpenNastranFile(const QStringList& fileNames);
	void SaveFile();
	void SaveFileAs();
	void SaveCurrentFileToRecentFile(QString file_name);
	void AddCurrentFileToRecentFilePath(QString lastPath);
	void InitRecentFilePaths();
	void InitRecentFileActions(std::vector<QString>);
	void UpdateRecentActionList();
	void UpdateIniFileInfo();
	QList<QAction*> GetRecentActionList() { return this->recentFileActionList; };



signals:
	void NewModel(DataObject::Pointer obj, ItemSource source);

    void RemotePackageRunningChanged(bool running);
    void RemotePackageStatusChanged(const QString& message);
    void RemotePackageProgressChanged(double progress);
    void RemotePackageFailed(const QString& message);
    void RemotePackageDatasetOpened(const QString& vtmPath);
    void RemotePackageFinished();
    void RemoteMemoryCacheChanged();
    void RemotePackagePreloaded(const QString& datasetPath);
    void RemotePreloadMeasured(quint64 requestId, bool memoryHit, qint64 elapsedMs,
                              quint64 bytes, const QString& detail);
    void RemoteCachedModelSelected(DataObject::Pointer object);
    // GUI-thread direct connections: reattach through the normal tree builder,
    // or remove through the normal deletion path, without reading/remapping.
    void RemoteCachedDatasetReattach(DataObject::Pointer object);
    void RemoteCachedDatasetDetach(DataObject::Pointer object, bool* detached);
    void RemoteRenderRequested(quint64 requestId);
    void RemoteRenderCancelled(quint64 requestId);
    void RemoteOpenMeasured(quint64 requestId, bool memoryHit, qint64 elapsedMs,
                            bool rendered, const QString& detail);
    void RemoteOpenStarted(quint64 requestId, const QString& address, quint16 port,
                           const QString& packageId);

	void FinishReading();
	void EmitMakeCurrent();
	void EmitDoneCurrent();

	void AddFileToModelList(QString file_name);

	void LoadAnimationFile(std::vector<float>& timeValues);

public slots:
    void CancelRemotePackage();
    void NotifyRemoteFrameCompleted(quint64 requestId, bool success, const QString& detail);

protected:
	bool TryOpenFile(const std::string& fileName, bool remoteRendering = false);
    void OpenSplineFile(const std::string& fileName, bool remoteRendering);
    void OpenODBFile(const std::string& fileName, bool remoteRendering);
    void OpenNastranFile(const QStringList& fileNames, bool remoteRendering);
    bool TryOpenRemoteDataset(const std::string& fileName);
    void InitializeResidentRemoteSupport();
    bool StartResidentRemoteRequest(const QString& address, quint16 port,
                                    const QString& packageId, const QString& cacheDirectory,
                                    bool preloadOnly = false);
    bool ReadResidentRemotePreload(const QString& datasetPath);
    bool ResidentRemotePreloadOnly() const;
    void FinishResidentRemotePreload();
    bool ResidentRemoteEnabled() const;
    bool ResidentRemoteActive() const;
    bool ResidentRemoteAcceptsDataset() const;
    void ContinueResidentRemoteRequest();
    void PrepareResidentRemoteFrame(const QString& datasetPath);
    void FailResidentRemoteRequest(const QString& reason);
    void CancelResidentRemoteRequest();
    bool ClearResidentRemoteCache();
    void ReleaseRetiredRemoteResources(bool includeDisplayed = false);

	QList<QAction*> recentFileActionList;
	int maxFileNr = 10;
    SceneManager::Pointer m_SceneManager;
    igQtRemotePackageLoader* m_RemotePackageLoader{nullptr};
    std::shared_ptr<igQtResidentRemoteState> m_ResidentRemote;
};
