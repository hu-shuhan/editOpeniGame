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

class IG_QT_MODULE_EXPORT igQtFileLoader : public QObject
{
	Q_OBJECT
public:
	igQtFileLoader(QObject* parent = nullptr);
	~igQtFileLoader() override;

public:
	void LoadFile();
    void LoadOnline();
	void OpenFile(const std::string& fileName);
    void OpenSplineFile(const std::string& fileName);
	void SaveFile();
	void SaveFileAs();
    bool Compress(int, int, int, int, int, int, std::vector<std::string>*, std::vector<std::string>*, std::string);
	void SaveCurrentFileToRecentFile(QString file_name);
	void AddCurrentFileToRecentFilePath(QString lastPath);
	void InitRecentFilePaths();
	void InitRecentFileActions(std::vector<QString>);
	void UpdateRecentActionList();
	void UpdateIniFileInfo();
	QList<QAction*> GetRecentActionList() { return this->recentFileActionList; };

signals:
	void NewModel(DataObject::Pointer obj, ItemSource source);

	void FinishReading();
	void EmitMakeCurrent();
	void EmitDoneCurrent();

	void AddFileToModelList(QString file_name);

	void LoadAnimationFile(std::vector<float>& timeValues);

protected:
	QList<QAction*> recentFileActionList;
	int maxFileNr = 10;
    SceneManager::Pointer m_SceneManager;
};
