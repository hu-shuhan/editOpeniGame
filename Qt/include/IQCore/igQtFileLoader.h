/**
 * @class   igQtFileLoader
 * @brief   igQtFileLoader's brief
 */

#pragma once
#include "iGameSceneManager.h"

#include <QFileDialog>
#include <QString>
#include <IQCore/igQtExportModule.h>

using namespace iGame;

class igQtRemotePackageLoader;

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

	void FinishReading();
	void EmitMakeCurrent();
	void EmitDoneCurrent();

	void AddFileToModelList(QString file_name);

	void LoadAnimationFile(std::vector<float>& timeValues);

public slots:
    void CancelRemotePackage();

protected:
	bool TryOpenFile(const std::string& fileName);

	QList<QAction*> recentFileActionList;
	int maxFileNr = 10;
    SceneManager::Pointer m_SceneManager;
    igQtRemotePackageLoader* m_RemotePackageLoader{nullptr};
};
