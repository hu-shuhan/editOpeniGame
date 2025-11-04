//
// Created by m_ky on 2024/4/22.
//
/**
 * @class   igQtFileLoader
 * @brief   igQtFileLoader's brief
 */
#include <cstring>

#include "iGameFileIO.h"
//#include "CSTest.h"
//#include "iGameMeshCodec/iGameMeshEncoder.h"
//#include "iGameMeshCodec/iGameMeshDecoder.h"
#include "iGamePointSet.h"
#include "iGameScene.h"
#if defined(GPSCUDA_ENABLE)
#include "Spline XML/iGameSplineSurfaceReader.h"
#include "Spline XML/iGameSplineVolumeReader.h"
#endif
#include "Nastran/iGameNastranReader.h"
#include "Abaqus/iGameODBReader.h"
#include "Client.h"
#include "Sever.h"
#include "Spline XML/iGameSplineReaderCPU.h"

#include <IQComponents/Dialog/igQtBasicListOptionDialog.h>
#include <IQComponents/Dialog/igQtSplineOptionDialog.h>
#include <IQCore/igQtFileLoader.h>
#include <IQCore/igQtFileType.h>

#include <QCoreApplication>
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
void igQtFileLoader::LoadOnlineS() {
#if defined(_WIN32) || defined(_WIN64)
    std::thread server_thread(serverThread);
    server_thread.join();
#endif
}
void igQtFileLoader::LoadOnlineC() {
#if defined(_WIN32) || defined(_WIN64)
    QStringList filters = {"ALL FIle(*.obj *.off *.stl *.ply *.vtk *.mesh *.pvd *.vts *.vtu "
                           "*.vtm *.cgns *.odb *.igc)",
                           "VTK file(*.vtk)",
                           "CGNS file(*.cgns)",
                           "ABAQUS file(*.odb)",
                           "Spline file(*.xml)",
                           "Compression file(*.igc)"};
    QString selectedFilter;
    std::string filePath =
            QFileDialog::getOpenFileName(nullptr, "Load file", "", filters.join(";;"), &selectedFilter).toStdString();
    auto selected_idx = static_cast<FileType>(filters.indexOf(selectedFilter));
    std::cout << filePath << std::endl;
    std::thread client_thread(clientThread, selected_idx, filePath);
    client_thread.join();
    this->OpenFile("./ReceivedFile.igc");
#endif
}
void igQtFileLoader::LoadFile() {
    QStringList filters = {"ALL FIle(*.obj *.off *.stl *.ply *.vtk *.mesh *.pvd *.vts *.vtu "
                            "*.vtm *.cgns *.odb *.igc)",
                            "VTK file(*.vtk)",
                            "CGNS file(*.cgns)",
#if defined(AbqSDK_ENABLE)
                            "ABAQUS file(*.odb)",
#endif
                            "Spline file(*.xml)",
#if defined(NASTRAN_ENABLE)
                            "Nastran file(*.bdf *.op2)",
#endif
                            "Compression file(*.igc)"};
    QString selectedFilter;
    QStringList filePath =
            QFileDialog::getOpenFileNames(nullptr, "Load file", "", filters.join(";;"), &selectedFilter);
    auto selected_idx = static_cast<FileType>(filters.indexOf(selectedFilter));
    if(filePath.empty()) return ;
    switch (selected_idx) {
        case FileType::Spline:
            this->OpenSplineFile(filePath[0].toStdString());
            break;
#if defined(AbqSDK_ENABLE)
        case FileType::ABAQUS:
            this->OpenODBFile(filePath[0].toStdString());
            break;
#endif
#if defined(NASTRAN_ENABLE)
        case FileType::BDF:
            this->OpenNastranFile(filePath);
            break;
#endif
        default:
            this->OpenFile(filePath[0].toStdString());
            break;
    }
}

//static DataObject::Pointer _obj;
void igQtFileLoader::OpenFile(const std::string& filePath) {
    using namespace iGame;
    if (filePath.empty() || strrchr(filePath.data(), '.') == nullptr) return;

    auto obj = iGame::FileIO::ReadFile(filePath);
    //_obj = obj;
    if (obj == nullptr) {
        igDebug("This file read error.");
        return;
    }
    auto filename = filePath.substr(filePath.find_last_of('/') + 1);
    obj->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
    obj->GetPropertys()->AddProperty(Variant::String, "FilePath")->SetValue(filePath);
    //Q_EMIT AddFileToModelList(QString(filePath.substr(filePath.find_last_of('/') + 1).c_str()));

    this->SaveCurrentFileToRecentFile(QString::fromStdString(filePath));


    //return;
    emit NewModel(obj, ItemSource::File);
    emit FinishReading();
}
void igQtFileLoader::OpenODBFile(const std::string& filePath) {
#if defined(AbqSDK_ENABLE)
    using namespace iGame;
    if (filePath.empty() || strrchr(filePath.data(), '.') == nullptr) return;
    igQtBasicListOptionDialog dialog;
    auto stepNames = ODBReader::ReadOdbAllStep(filePath);
    dialog.setInfoList(stepNames);
    auto filename = filePath.substr(filePath.find_last_of('/') + 1);
    dialog.setWindowTitle("ODB Reader Info");
    dialog.setLabelName(filename, "More than one Step for \" %s \".Please choose one:");
    if (dialog.exec() == QDialog::Accepted) {
        int stepIdx = -1;
        stepIdx = dialog.getDialogOutput();
        if (~stepIdx) {
            auto reader = iGame::ODBReader::New();
            DataObject::Pointer obj = reader->ReadOdbFirstFrameMesh(filePath, stepNames[stepIdx]);
            obj->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
            obj->GetPropertys()->AddProperty(Variant::String, "FilePath")->SetValue(filePath);
            //Q_EMIT AddFileToModelList(QString(filePath.substr(filePath.find_last_of('/') + 1).c_str()));

            this->SaveCurrentFileToRecentFile(QString::fromStdString(filePath));
            emit NewModel(obj, ItemSource::File);
            emit FinishReading();
        }
    }

#endif
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
            SplineReaderCPU::Pointer reader = SplineReaderCPU::New();
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
    obj->GetPropertys()->AddProperty(Variant::String, "FilePath")->SetValue(filePath);
    //Q_EMIT AddFileToModelList(QString(filePath.substr(filePath.find_last_of('/') + 1).c_str()));

    this->SaveCurrentFileToRecentFile(QString::fromStdString(filePath));
    emit NewModel(obj, ItemSource::File);
    emit FinishReading();

#else
    DataObject::Pointer obj = nullptr;
    /* Without Cuda version, only support Nurbs Reader.*/
    SplineReaderCPU::Pointer reader = SplineReaderCPU::New();
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
    obj->GetPropertys()->AddProperty(Variant::String, "FilePath")->SetValue(filePath);

    this->SaveCurrentFileToRecentFile(QString::fromStdString(filePath));
    emit NewModel(obj, ItemSource::File);
    emit FinishReading();
#endif
}
void igQtFileLoader::OpenNastranFile(const QStringList& fileNames) {
#if defined(NASTRAN_ENABLE)
    // --- 校验阶段 ---

    // 规则：检查是否为空
    if (fileNames.isEmpty()) {
        return; // 没有文件，安静地返回
    }

    // 规则：检查是否多于两个文件
    if (fileNames.size() > 2) {
        // QWidget* parent = this; // 如果 igQtFileLoader 继承自 QWidget，使用 'this' 作为父窗口更好
        QMessageBox::warning(nullptr,
                             "文件过多",
                             "您一次最多只能选择两个文件（一个 .bdf 模型文件和一个 .op2 结果文件）。");
        return;
    }

    QString bdfPath = "";
    QString op2Path = "";

    // 规则：检查只有一个文件的情况
    if (fileNames.size() == 1) {
        const QString& filePath = fileNames.first();
        QFileInfo fileInfo(filePath);
        // 使用 .toLower() 确保后缀名比较时不区分大小写 (例如 .BDF 也能识别)
        QString suffix = fileInfo.suffix().toLower();

        if (suffix == "bdf") {
            // 唯一允许的情况：只有一个文件，且是 bdf
            bdfPath = filePath;
            // op2Path 保持为空
        }
        else if (suffix == "op2") {
            // 错误：只选了一个 op2 文件
            QMessageBox::warning(nullptr,
                                 "文件选择错误",
                                 "您只选择了一个 OP2 (结果) 文件。请至少选择一个 BDF (模型) 文件。");
            return;
        }
        else {
            // 错误：选了其他类型的文件
            QMessageBox::warning(nullptr,
                                 "文件类型错误",
                                 "您必须选择一个 .bdf 文件。");
            return;
        }
    }
    // 规则：检查有两个文件的情况
    else if (fileNames.size() == 2) {
        QFileInfo file1(fileNames[0]);
        QFileInfo file2(fileNames[1]);

        QString suffix1 = file1.suffix().toLower();
        QString suffix2 = file2.suffix().toLower();

        // 检查是否为一个 bdf 和一个 op2 的组合
        if (suffix1 == "bdf" && suffix2 == "op2") {
            bdfPath = fileNames[0];
            op2Path = fileNames[1];
        }
        else if (suffix1 == "op2" && suffix2 == "bdf") {
            bdfPath = fileNames[1];
            op2Path = fileNames[0];
        }
        else {
            // 错误：不是 bdf + op2 的组合 (例如 两个bdf, 或一个bdf一个txt)
            QMessageBox::warning(nullptr,
                                 "文件组合错误",
                                 "您选择了两个文件，但它们必须是一个 .bdf 文件和一个 .op2 文件。");
            return;
        }
    }

    // --- 执行阶段 ---
    iGame::NastranReader::Pointer reader = iGame::NastranReader::New();
    reader->SetBDFFileName(bdfPath.toStdString());
    if(!op2Path.isEmpty()) reader->SetOP2FileName(op2Path.toStdString());
    reader->Execute();
    DataObject::Pointer obj = reader->GetOutput();

    if (obj == nullptr) {
        igDebug("This file read error.");
        return;
    }
    auto filePath = bdfPath.toStdString();
    auto filename = filePath.substr(filePath.find_last_of('/') + 1);
    obj->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
    obj->GetPropertys()->AddProperty(Variant::String, "FilePath")->SetValue(filePath);

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
    std::string filePath = QFileDialog::getSaveFileName(nullptr, "Save file as ", "",
                                                        "Surface Mesh(*.obj *.off *.stl *.vtk);;Volume Mesh(*.mesh "
                                                        "*.vtk *.ex2 *.e *.pvd *.vts *.vtm)")
                                   .toStdString();
    if (filePath.empty()) {
        igDebug("Could not save file with error file path\n");
        return;
    }
    if (!iGame::FileIO::WriteFile(filePath, obj)) { igDebug("Save File Error\n"); }
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
    connect(recentFileAction, &QAction::triggered, this, [=]() { this->OpenFile(filePath.toStdString()); });
    this->recentFileActionList.append(recentFileAction);
    UpdateRecentActionList();
}
void igQtFileLoader::UpdateIniFileInfo() {
    //为了能记住上次打开的路径
    QSettings setting(QCoreApplication::applicationDirPath() + "/config/savePath.ini", QSettings::IniFormat);
    int num = this->recentFileActionList.size();
    int idx = 0;
    for (int i = 0; i < num; i++) {
        if (recentFileActionList.at(i)->isVisible()) {
            idx++;
            QString name = "LastFilePath" + QString::fromStdString(std::to_string(idx));
            setting.setValue(name, this->recentFileActionList[i]->data());
        }
    }
}


void igQtFileLoader::InitRecentFilePaths() {
    QString path = QCoreApplication::applicationDirPath() + "/config/savePath.ini";
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
        connect(recentFileAction, &QAction::triggered, this, [=]() { this->OpenFile(FilePaths[i].toStdString()); });
        this->recentFileActionList.append(recentFileAction);
    }
    UpdateRecentActionList();
    return;
}

void igQtFileLoader::UpdateRecentActionList() {
    int st = this->recentFileActionList.size() - 1;
    ;
    int ed = std::max(st - maxFileNr + 1, 0);
    for (int i = st; i >= ed; i--) { this->recentFileActionList.at(i)->setVisible(true); }
    for (int i = ed - 1; i >= 0; i--) { this->recentFileActionList.at(i)->setVisible(false); }
    return;
}
