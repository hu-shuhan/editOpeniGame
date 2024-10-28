//
// Created by m_ky on 2024/4/22.
//
/**
 * @class   igQtFileLoader
 * @brief   igQtFileLoader's brief
 */
#include <cstring>

#include "iGameFileIO.h"
#include "iGamePointSet.h"
#include "iGameScene.h"
#if defined(GPSCUDA_ENABLE)
#include "Spline XML/iGameSplineSurfaceReader.h"
#include "Spline XML/iGameSplineVolumeReader.h"
#endif
#include "Spline XML/iGameNurbsReader.h"

#include <IQComponents/Dialog/igQtSplineOptionDialog.h>
#include <IQCore/igQtFileLoader.h>
#include <IQCore/igQtFileType.h>

#include <QCoreApplication.h>
#include <QMessageBox>
#include <iostream>
#include <qaction.h>
#include <qdebug.h>
#include <qsettings.h>

igQtFileLoader::igQtFileLoader(QObject* parent) : QObject(parent) {
    InitRecentFilePaths();
    m_SceneManager = SceneManager::Instance();
}

igQtFileLoader::~igQtFileLoader() {}

void igQtFileLoader::LoadFile() {
    QStringList filters = {
            "ALL FIle(*.obj *.off *.stl *.ply *.vtk *.mesh *.pvd *.vts *.vtu "
            "*.vtm *.cgns *.odb)",
            "VTK file(*.vtk)",
            "CGNS file(*.cgns)",
            "ABAQUS file(*.odb)",
            "Spline file(*.xml)",
    };
    QString selectedFilter;
    std::string filePath =
            QFileDialog::getOpenFileName(nullptr, "Load file", "",
                                         filters.join(";;"), &selectedFilter)
                    .toStdString();
    auto selected_idx = static_cast<FileType>(filters.indexOf(selectedFilter));

    switch (selected_idx) {
        case FileType::Spline:
            this->OpenSplineFile(filePath);
            break;

        default:
            this->OpenFile(filePath);
            break;
    }
}

void igQtFileLoader::OpenFile(const std::string& filePath) {
    using namespace iGame;
    if (filePath.empty() || strrchr(filePath.data(), '.') == nullptr) return;

    auto obj = iGame::FileIO::ReadFile(filePath);
    if (obj == nullptr) {
        igDebug("This file read error.");
        return;
    }
    auto filename = filePath.substr(filePath.find_last_of('/') + 1);
    obj->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
    obj->GetPropertys()
            ->AddProperty(Variant::String, "FilePath")
            ->SetValue(filePath);
    //Q_EMIT AddFileToModelList(QString(filePath.substr(filePath.find_last_of('/') + 1).c_str()));

    this->SaveCurrentFileToRecentFile(QString::fromStdString(filePath));
    emit NewModel(obj, ItemSource::File);
    emit FinishReading();
}

void igQtFileLoader::OpenSplineFile(const std::string& filePath) {

    using namespace iGame;
    if (filePath.empty() || strrchr(filePath.data(), '.') == nullptr) return;
    igQtSplineOptionDialog dialog;
    dialog.setFileName(QString(filePath.c_str()));

    SplineType readerType;
    if (dialog.exec() == QDialog::Accepted) {
        readerType = dialog.getDialogOutput();
    } else
        return;

#if defined(GPSCUDA_ENABLE)
    DataObject::Pointer obj = nullptr;
    switch (readerType) {
        case SplineType::BSplineSurface: {
            SplineSurfaceReader::Pointer reader = SplineSurfaceReader::New();
            reader->SetFilePath(filePath);
            reader->Execute();
            obj = reader->GetOutput();
            break;
        }
        case SplineType::BSplineVolume: {
            SplineVolumeReader::Pointer reader = SplineVolumeReader::New();
            reader->SetFilePath(filePath);
            reader->Execute();
            obj = reader->GetOutput();
            break;
        }
        default:
            NurbsReader::Pointer reader = NurbsReader::New();
            reader->SetNurbsType(readerType);
            reader->SetFilePath(filePath);
            reader->Execute();
            obj = reader->GetOutput();
            break;
    }
    if (obj == nullptr) {
        igDebug("This file read error.");
        return;
    }
    auto filename = filePath.substr(filePath.find_last_of('/') + 1);
    obj->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
    obj->GetPropertys()
            ->AddProperty(Variant::String, "FilePath")
            ->SetValue(filePath);
    //Q_EMIT AddFileToModelList(QString(filePath.substr(filePath.find_last_of('/') + 1).c_str()));

    this->SaveCurrentFileToRecentFile(QString::fromStdString(filePath));
    emit NewModel(obj, ItemSource::File);
    emit FinishReading();

#else
    DataObject::Pointer obj = nullptr;
    /* Without Cuda version, only support Nurbs Reader.*/
    NurbsReader::Pointer reader = NurbsReader::New();
    //reader->SetNurbsType(readerType);
    reader->SetFilePath(filePath);
    reader->Execute();
    obj = reader->GetOutput();

    if (obj == nullptr) {
        igDebug("This file read error.");
        return;
    }
    auto filename = filePath.substr(filePath.find_last_of('/') + 1);
    obj->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
    obj->GetPropertys()
            ->AddProperty(Variant::String, "FilePath")
            ->SetValue(filePath);

    this->SaveCurrentFileToRecentFile(QString::fromStdString(filePath));
    emit NewModel(obj, ItemSource::File);
    emit FinishReading();
#endif
}

void igQtFileLoader::SaveFile() {
    //auto currentModel = iGame::iGameManager::Instance()->GetCurrentModel();
    //if (currentModel == nullptr)return;
    //if (!iGame::iGameFileIO::WriteDataSetToFile(currentModel->DataSet, currentModel->filePath)) {
    //	igDebug("Save File Error\n");
    //}
}

void igQtFileLoader::SaveFileAs() {

    auto sceneManager = iGame::SceneManager::Instance();
    auto scene = sceneManager->GetCurrentScene();
    if (!scene) return;
    auto currentModel = scene->GetCurrentModel();
    if (!currentModel) return;
    auto obj = currentModel->GetDataObject();
    if (!obj) return;
    std::string filePath =
            QFileDialog::getSaveFileName(
                    nullptr, "Save file as ", "",
                    "Surface Mesh(*.obj *.off *.stl *.vtk);;Volume Mesh(*.mesh "
                    "*.vtk *.ex2 *.e *.pvd *.vts)")
                    .toStdString();
    if (filePath.empty()) {
        igDebug("Could not save file with error file path\n");
        return;
    }
    if (!iGame::FileIO::WriteFile(filePath, obj)) {
        igDebug("Save File Error\n");
    }
}
void igQtFileLoader::SaveCurrentFileToRecentFile(QString path) {
    if (path.isEmpty()) return;
    for (int i = 0; i < recentFileActionList.size(); i++) {
        if (recentFileActionList.at(i)->data() == path) {
            delete recentFileActionList.at(i);
            recentFileActionList.removeAt(i);
            break;
        }
    }
    AddCurrentFileToRecentFilePath(path);
    UpdateIniFileInfo();
    return;
}
void igQtFileLoader::AddCurrentFileToRecentFilePath(QString filePath) {
    auto recentFileActions = this->GetRecentActionList();
    QAction* recentFileAction = nullptr;
    recentFileAction = new QAction(this);
    recentFileAction->setText(filePath);
    recentFileAction->setData(filePath);
    recentFileAction->setVisible(true);
    connect(recentFileAction, &QAction::triggered, this,
            [=]() { this->OpenFile(filePath.toStdString()); });
    this->recentFileActionList.append(recentFileAction);
    UpdateRecentActionList();
}
void igQtFileLoader::UpdateIniFileInfo() {
    //为了能记住上次打开的路径
    QSettings setting(QCoreApplication::applicationDirPath() +
                              "/config/savePath.ini",
                      QSettings::IniFormat);
    int num = this->recentFileActionList.size();
    int idx = 0;
    for (int i = 0; i < num; i++) {
        if (recentFileActionList.at(i)->isVisible()) {
            idx++;
            QString name = "LastFilePath" +
                           QString::fromStdString(std::to_string(idx));
            setting.setValue(name, this->recentFileActionList[i]->data());
        }
    }
}


void igQtFileLoader::InitRecentFilePaths() {
    QString path =
            QCoreApplication::applicationDirPath() + "/config/savePath.ini";
    QFile* file = new QFile(this);
    std::vector<QString> FilePaths;
    file->setFileName(path);
    if (!file->open(QIODevice::ReadOnly)) { return; }
    while (!file->atEnd()) {
        QString str = file->readLine();
        //std::cout << str.toStdString()<< std::endl;
        if (str.toStdString().find('=') == std::string::npos) continue;
        QStringList list = str.split("=");
        if (!list.isEmpty()) { FilePaths.emplace_back(list.at(1).trimmed()); }
    }
    file->close();
    delete file;
    InitRecentFileActions(FilePaths);
}

void igQtFileLoader::InitRecentFileActions(std::vector<QString> FilePaths) {
    QAction* recentFileAction = nullptr;
    for (int i = 0; i < FilePaths.size(); i++) {
        recentFileAction = new QAction(this);
        recentFileAction->setText(FilePaths[i]);
        recentFileAction->setData(FilePaths[i]);
        recentFileAction->setVisible(false);
        connect(recentFileAction, &QAction::triggered, this,
                [=]() { this->OpenFile(FilePaths[i].toStdString()); });
        this->recentFileActionList.append(recentFileAction);
    }
    UpdateRecentActionList();
    return;
}

void igQtFileLoader::UpdateRecentActionList() {
    int st = this->recentFileActionList.size() - 1;
    ;
    int ed = std::max(st - maxFileNr + 1, 0);
    for (int i = st; i >= ed; i--) {
        this->recentFileActionList.at(i)->setVisible(true);
    }
    for (int i = ed - 1; i >= 0; i--) {
        this->recentFileActionList.at(i)->setVisible(false);
    }
    return;
}
