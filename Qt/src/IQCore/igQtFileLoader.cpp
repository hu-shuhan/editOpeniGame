//
// Created by m_ky on 2024/4/22.
//
/**
 * @class   igQtFileLoader
 * @brief   igQtFileLoader's brief
 */
#include <cstring>

#include "iGameFileIO.h"
#include "iGameProgressObserver.h"
//#include "CSTest.h"
#include "iGamePointSet.h"
#include "iGameScene.h"
#if defined(GPSCUDA_ENABLE)
#include "Spline XML/iGameSplineReaderGPU.h"
#endif
#include "Abaqus/iGameODBReader.h"
#if defined(_WIN32) || defined(_WIN64)
#include "Client.h"
#include "Sever.h"
#endif
#include "Nastran/iGameNastranReader.h"
#include "Spline XML/iGameSplineReaderCPU.h"

#include <IQComponents/Dialog/igQtBasicListOptionDialog.h>
#include <IQComponents/Dialog/igQtSplineOptionDialog.h>
#include <IQCore/igQtFileLoader.h>
#include <IQCore/igQtFileType.h>
#include <iGameType.h>

#include <QCoreApplication>
#include <QMetaObject>
#include <QMessageBox>
#include <QPointer>
#include <QThread>
#include <filesystem>
#include <iostream>
#include <memory>
#include <utility>
#include <qaction.h>
#include <qdebug.h>
#include <qsettings.h>
#include <IQCore/igQtAttributeDataSourceManager.h>
#include <IQCore/igQtDataCodecDecodeSettings.h>
#include <IGDC/iGameIGDCReader.h>

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
                           "*.vtm *.cgns *.odb *.igc *.cas)",
                           "VTK file(*.vtk)",
                           "CGNS file(*.cgns)",
                           "ABAQUS file(*.odb)",
                           "Spline file(*.xml)",
                           "Compression file(*.igc)",
                           "Fluent file(*.cas)"};
    QString selectedFilter;
    std::string filePath =
            QFileDialog::getOpenFileName(nullptr, "Load file", "", filters.join(";;"), &selectedFilter).toStdString();
    auto selected_idx = static_cast<FileType>(filters.indexOf(selectedFilter));
    std::cout << filePath << std::endl;
    // 关键修改：用packaged_task获取线程返回值
    // 1. 创建任务对象，绑定clientThread和参数
    std::packaged_task<std::string(int, std::string)> task(clientThread);
    // 2. 获取future对象，用于接收线程返回值
    std::future<std::string> fut = task.get_future();
    // 3. 启动线程（将task转移到线程中）
    std::thread client_thread(std::move(task), selected_idx, filePath);
    // 4. 等待线程结束，并获取返回值（localFileName）
    client_thread.join();
    std::string localFileName = fut.get();  // 获取接收后的文件名

    // 5. 传递文件名给OpenFile（判断是否为空，避免传无效值）
    if (!localFileName.empty()) {
        this->OpenFile(localFileName);  // 替换空字符串为实际文件名
    } else {
        std::cerr << "[LoadOnlineC] Failed to receive file, OpenFile skipped\n";
        this->OpenFile("");  // 失败时仍传空，保持原有逻辑
    }

#endif
}
void igQtFileLoader::LoadFile() {
    QStringList filters = {
        "ALL File(*.obj *.off *.stl *.ply *.vtk *.mesh *.pvd *.vts *.vtu "
        "*.vtm *.cgns *.igc *.cas *.xml"
#if defined(AbqSDK_ENABLE)
        " *.odb"
#endif
#if defined(NASTRAN_ENABLE)
        " *.bdf *.op2"
#endif
        ")",
        "VTK file(*.vtk)",
        "CGNS file(*.cgns)",
#if defined(AbqSDK_ENABLE)
        "ABAQUS file(*.odb)",
#endif
        "Spline file(*.xml)",
#if defined(NASTRAN_ENABLE)
        "Nastran file(*.bdf *.op2)",
#endif
        "Compression file(*.igc)",
        "Fluent file(*.cas)"
    };
    QString selectedFilter;
    QStringList filePath = QFileDialog::getOpenFileNames(nullptr, "Load file", "", filters.join(";;"), &selectedFilter);

    if (filePath.isEmpty()) { return; }

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
            this->OpenFiles(filePath);
            break;
    }
}



//static DataObject::Pointer _obj;
void igQtFileLoader::OpenFile(const std::string& filePath) {
    using namespace iGame;
    if (filePath.empty() || strrchr(filePath.data(), '.') == nullptr) return;

    if (FileIO::GetFileType(filePath) == FileIO::IGDC) {
        OpenDataCodecFileAsync(filePath);
        return;
    }

    auto obj = iGame::FileIO::ReadFile(filePath);
    //_obj = obj;
    if (obj == nullptr) {
        igDebug("This file read error.");
        return;
    }
    auto filename = filePath.substr(filePath.find_last_of('/') + 1);
    obj->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
    obj->GetProperties()->AddProperty(Variant::String, "FilePath")->SetValue(filePath);
    //Q_EMIT AddFileToModelList(QString(filePath.substr(filePath.find_last_of('/') + 1).c_str()));

    this->SaveCurrentFileToRecentFile(QString::fromStdString(filePath));


    //return;
    emit NewModel(obj, ItemSource::File);
    emit FinishReading();
}

void igQtFileLoader::OpenFiles(const QStringList& filePaths) {
    using namespace iGame;
    if (filePaths.empty()) return;

    const std::string& first_file_path = filePaths[0].toStdString();
    if(strrchr(first_file_path.data(), '.') == nullptr) return;

    std::vector<std::string> dataCodecPaths;
    dataCodecPaths.reserve(filePaths.size());
    const bool allDataCodecFiles = std::all_of(filePaths.begin(), filePaths.end(), [&](const auto& path) {
        const auto nativePath = path.toStdString();
        if (FileIO::GetFileType(nativePath) != FileIO::IGDC) { return false; }
        dataCodecPaths.push_back(nativePath);
        return true;
    });
    if (allDataCodecFiles) {
        OpenDataCodecFilesAsync(std::move(dataCodecPaths));
        return;
    }

    // 检测 XML 后缀，使用 Spline 弹窗处理
    const char* ext = strrchr(first_file_path.data(), '.');
    if (ext != nullptr) {
        std::string suffix(ext + 1);
        // 转换为小写进行比较
        std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
        if (suffix == "xml") {
            this->OpenSplineFile(first_file_path);
            return;
        }
    }

    auto obj = iGame::FileIO::ReadFile(first_file_path);
    //_obj = obj;
    if (obj == nullptr) {
        igDebug("This file read error.");
        return;
    }

    //Q_EMIT AddFileToModelList(QString(filePath.substr(filePath.find_last_of('/') + 1).c_str()));

    this->SaveCurrentFileToRecentFile(QString::fromStdString(first_file_path));

    /* Add left FilePaths to be as SubDataObject. */
    if(filePaths.size() > 1)
    {
        iGame::DataObject::Pointer outerObj = iGame::DrawObject::New();


        double dataRange_max[64], dataRange_min[64];
        for(int k = 0; k < obj->GetAttributeSet()->GetAllAttributes()->GetNumberOfElements(); k ++){
            auto& attr = obj->GetAttributeSet()->GetAttribute(k);
            int dim = attr.pointer->GetDimension();

            DoubleArray::Pointer array = DoubleArray::New();
            array->SetName(attr.pointer->GetName());
            array->SetDimension(dim);
            attr.UpdateAllDataRange();
            const auto& ScalarDataRange = attr.GetDataRange();

            std::fill(dataRange_min, dataRange_min + 64, DBL_MAX);
            std::fill(dataRange_max, dataRange_max + 64, DBL_MIN);
            for(int j = 0; j < dim + 1; j ++){
                dataRange_min[j] = std::min(dataRange_min[j], ScalarDataRange->GetValue(2 * j + 0));
                dataRange_max[j] = std::max(dataRange_max[j], ScalarDataRange->GetValue(2 * j + 1));
            }

            DoubleArray::Pointer parent_dataRange = DoubleArray::New();
            parent_dataRange->SetDimension(2);
            parent_dataRange->Resize(dim + 1);
            for(int j = 0; j < dim + 1; j ++){
                parent_dataRange->SetElement(j, {dataRange_min[j], dataRange_max[j]});
            }
            switch (attr.type) {
                case IG_SCALAR:
                    outerObj->GetAttributeSet()->AddScalar(attr.attachmentType, array, parent_dataRange);
                    break;
                case IG_VECTOR:
                    outerObj->GetAttributeSet()->AddVector(attr.attachmentType, array, parent_dataRange);
                    break;
                default:
                    break;
            }
        }
        outerObj->AddSubDataObject(obj);
        outerObj->UpdateSubDataObjectDataRange();

        auto filename = first_file_path.substr(first_file_path.find_last_of('/') + 1);
        outerObj->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
        outerObj->GetProperties()->AddProperty(Variant::String, "FilePath")->SetValue(first_file_path);

        auto timeFrame = outerObj->GetTimeFrames();
        for(int i = 0; i < filePaths.size(); i ++){
            iGame::StringArray::Pointer subFileNameArray = iGame::StringArray::New();
            subFileNameArray->AddElement(filePaths[i].toStdString());
            timeFrame->AddTimeStep((float) (i + 1) / filePaths.size(), subFileNameArray, StreamingType::MultiSubFiles);
        }
        //return;
        emit NewModel(outerObj, ItemSource::File);
        emit FinishReading();
        return ;
    }

    auto filename = first_file_path.substr(first_file_path.find_last_of('/') + 1);
    obj->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
    obj->GetProperties()->AddProperty(Variant::String, "FilePath")->SetValue(first_file_path);
    //Q_EMIT AddFileToModelList(QString(filePath.substr(filePath.find_last_of('/') + 1).c_str()));


    //return;
    emit NewModel(obj, ItemSource::File);
    emit FinishReading();
}

void igQtFileLoader::OpenDataCodecFileAsync(std::string filePath) {
    OpenDataCodecFilesAsync({std::move(filePath)});
}

void igQtFileLoader::OpenDataCodecFilesAsync(std::vector<std::string> filePaths) {
    if (filePaths.empty()) { return; }
    const bool decodeAttributesOnDemand =
        igQtDataCodecDecodeSettingsStore::Load().decodeAttributesOnDemand;
    if (auto* progress = iGame::ProgressObserver::Instance(); progress != nullptr) {
        progress->UpdateProgress(0.0);
        progress->UpdateText("");
    }

    QPointer<igQtFileLoader> self(this);
    auto* worker = QThread::create([
        self,
        filePaths = std::move(filePaths),
        decodeAttributesOnDemand]() mutable {
        iGame::AttributeDataSourcePointer source;
        std::string error;
        iGame::DataObject::Pointer object;
        auto filePath = filePaths.front();
        if (filePaths.size() > 1u) {
            auto reader = iGame::IGDCReader::New();
            reader->SetFilePath(filePath);
            reader->SetLoadAllAvailableAttributes(!decodeAttributesOnDemand);
            reader->SetSelectedFramePaths(filePaths);
            if (reader->Execute()) {
                object = reader->GetOutput();
                if (decodeAttributesOnDemand) {
                    source = reader->GetAttributeDataSource();
                }
            }
            if (object == nullptr) {
                error.clear();
                for (const auto& message: reader->GetMessages()) {
                    if (!error.empty()) { error += "; "; }
                    error += message.text;
                }
            }
        } else if (decodeAttributesOnDemand) {
            auto reader = iGame::IGDCReader::New();
            reader->SetFilePath(filePath);
            reader->SetLoadAllAvailableAttributes(false);
            if (reader->Execute()) {
                object = reader->GetOutput();
                source = reader->GetAttributeDataSource();
            }
            if (object == nullptr) {
                for (const auto& message: reader->GetMessages()) {
                    if (!error.empty()) { error += "; "; }
                    error += message.text;
                }
            }
        } else {
            auto reader = iGame::IGDCReader::New();
            reader->SetFilePath(filePath);
            if (reader->Execute()) { object = reader->GetOutput(); }
            if (object == nullptr) {
                for (const auto& message: reader->GetMessages()) {
                    if (!error.empty()) { error += "; "; }
                    error += message.text;
                }
            }
        }
        if (!self) return;

        QMetaObject::invokeMethod(self, [self, source, object, filePath, error]() {
            if (!self) return;
            if (object == nullptr) {
                igDebug("DataCodec file read error: {}", error);
                if (error.find("版本不符合") != std::string::npos) {
                    QMessageBox::warning(nullptr, "错误", "版本不符合");
                }
                emit self->FinishReading();
                return;
            }

            auto filename = filePath.substr(filePath.find_last_of('/') + 1);
            object->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
            object->GetProperties()->AddProperty(iGame::Variant::String, "FilePath")->SetValue(filePath);
            if (source != nullptr) {
                igQtAttributeDataSourceManager::Instance()->RegisterSource(object, source);
            }
            self->SaveCurrentFileToRecentFile(QString::fromStdString(filePath));
            emit self->NewModel(object, ItemSource::File);
            emit self->FinishReading();
        }, Qt::QueuedConnection);
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
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
            obj->GetProperties()->AddProperty(Variant::String, "FilePath")->SetValue(filePath);
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
    } else {
        return;
    }

#if defined(GPSCUDA_ENABLE)
    DataObject::Pointer obj = nullptr;
    switch (readerType) {
        case SplineType::BSplineSurfaceCPU: {
            SplineReaderCPU::Pointer reader = SplineReaderCPU::New();
            reader->SetFilePath(filePath);
            reader->SetSurfaceRenderForVolume(true);
            reader->Execute();
            obj = reader->GetOutput();
            break;
        }
        case SplineType::BSplineVolumeCPU: {
            SplineReaderCPU::Pointer reader = SplineReaderCPU::New();
            reader->SetFilePath(filePath);
            reader->SetSurfaceRenderForVolume(false);
            reader->Execute();
            obj = reader->GetOutput();
            break;
        }
        case SplineType::BSplineSurfaceGPU: {
            SplineReaderGPU::Pointer reader = SplineReaderGPU::New();
            reader->SetFilePath(filePath);
            reader->SetSurfaceRenderForVolume(true);
            reader->Execute();
            obj = reader->GetOutput();
            break;
        }
        case SplineType::BSplineVolumeGPU: {
            SplineReaderGPU::Pointer reader = SplineReaderGPU::New();
            reader->SetFilePath(filePath);
            reader->SetSurfaceRenderForVolume(false);
            reader->Execute();
            obj = reader->GetOutput();
            break;
        }
        default:
            igDebug("Spline file type not process.");
    }
    if (obj == nullptr) {
        igDebug("This file read error.");
        return;
    }
    auto filename = filePath.substr(filePath.find_last_of('/') + 1);
    obj->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
    obj->GetProperties()->AddProperty(Variant::String, "FilePath")->SetValue(filePath);
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
    obj->GetProperties()->AddProperty(Variant::String, "FilePath")->SetValue(filePath);

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
        QMessageBox::warning(nullptr, "文件过多",
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
        } else if (suffix == "op2") {
            // 错误：只选了一个 op2 文件
            QMessageBox::warning(nullptr, "文件选择错误",
                                 "您只选择了一个 OP2 (结果) 文件。请至少选择一个 BDF (模型) 文件。");
            return;
        } else {
            // 错误：选了其他类型的文件
            QMessageBox::warning(nullptr, "文件类型错误", "您必须选择一个 .bdf 文件。");
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
        } else if (suffix1 == "op2" && suffix2 == "bdf") {
            bdfPath = fileNames[1];
            op2Path = fileNames[0];
        } else {
            // 错误：不是 bdf + op2 的组合 (例如 两个bdf, 或一个bdf一个txt)
            QMessageBox::warning(nullptr, "文件组合错误",
                                 "您选择了两个文件，但它们必须是一个 .bdf 文件和一个 .op2 文件。");
            return;
        }
    }

    // --- 执行阶段 ---
    iGame::NastranReader::Pointer reader = iGame::NastranReader::New();
    reader->SetBDFFileName(bdfPath.toStdString());
    if (!op2Path.isEmpty()) reader->SetOP2FileName(op2Path.toStdString());
    reader->Execute();
    DataObject::Pointer obj = reader->GetOutput();

    if (obj == nullptr) {
        igDebug("This file read error.");
        return;
    }
    auto filePath = bdfPath.toStdString();
    auto filename = filePath.substr(filePath.find_last_of('/') + 1);
    obj->SetName(filename.substr(0, filename.find_last_of('.')).c_str());
    obj->GetProperties()->AddProperty(Variant::String, "FilePath")->SetValue(filePath);

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
    connect(recentFileAction, &QAction::triggered, this, [=, this]() { this->OpenFile(filePath.toStdString()); });
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
        connect(recentFileAction, &QAction::triggered, this, [=, this]() { this->OpenFile(FilePaths[i].toStdString()); });
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
