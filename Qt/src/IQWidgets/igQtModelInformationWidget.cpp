#include "IQWidgets/igQtModelInformationWidget.h"
#include "iGameSceneManager.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <filesystem>
igQtModelInformationWidget::igQtModelInformationWidget(QWidget* parent) : QWidget(parent) {
    // Layout for the main widget
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // Creating frames and adding them to the main layout
    this->informationFrame = new QFrame();
    this->informationFrame->setFrameShape(QFrame::StyledPanel);
    this->frameLayout = new QVBoxLayout(this->informationFrame);
    mainLayout->addWidget(this->informationFrame);
}

void igQtModelInformationWidget::CreateDataObjectLayoutInfo(iGame::DataObject::Pointer obj) {
    switch (obj->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto mesh = iGame::DynamicCast<iGame::SurfaceMesh>(obj);
            frameLayout->addWidget(createPropertyLabel("Type", "Surface Mesh"));
            frameLayout->addWidget(createPropertyLabel("# of Faces", QString::number(mesh->GetNumberOfFaces())));
            frameLayout->addWidget(createPropertyLabel("# of Points", QString::number(mesh->GetNumberOfPoints())));
        } break;
        case IG_VOLUME_MESH: {
            auto mesh = iGame::DynamicCast<iGame::VolumeMesh>(obj);
            frameLayout->addWidget(createPropertyLabel("Type", "Volume Mesh"));
            frameLayout->addWidget(createPropertyLabel("# of Volumes", QString::number(mesh->GetNumberOfVolumes())));
            frameLayout->addWidget(createPropertyLabel("# of Points", QString::number(mesh->GetNumberOfPoints())));
        } break;
        case IG_STRUCTURED_MESH: {
            auto mesh = iGame::DynamicCast<iGame::StructuredMesh>(obj);
            auto size = mesh->GetDimensionSize();
            frameLayout->addWidget(createPropertyLabel("Type", "Structured Mesh"));
            frameLayout->addWidget(createPropertyLabel("# of Dimesion X ", QString::number(size[0])));
            frameLayout->addWidget(createPropertyLabel("# of Dimesion Y ", QString::number(size[1])));
            frameLayout->addWidget(createPropertyLabel("# of Dimesion Z ", QString::number(size[2])));
        } break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
            frameLayout->addWidget(createPropertyLabel("Type", "Unstructured Mesh"));
            frameLayout->addWidget(createPropertyLabel("# of Cells", QString::number(mesh->GetNumberOfCells())));
            frameLayout->addWidget(createPropertyLabel("# of Points", QString::number(mesh->GetNumberOfPoints())));
        } break;
        default:
            break;
    }
}
void igQtModelInformationWidget::updateInformationFrame() {
    // 禁用界面更新
    this->setUpdatesEnabled(false);
    frameLayout->blockSignals(true);

    // 清空现有控件
    while (QLayoutItem* item = frameLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->hide();        // 隐藏控件
            widget->deleteLater(); // 标记控件待删除
        }
        delete item; // 删除布局项
    }

    // 获取当前场景和模型
    auto sceneManeger = iGame::SceneManager::Instance();
    auto scene = sceneManeger->GetCurrentScene();
    if (!scene) return;

    auto currentModel = scene->GetCurrentModel();
    if (!currentModel) {
        this->hide();
        this->setUpdatesEnabled(true);
        return; // 直接返回，避免继续执行
    }
    this->show();

    auto obj = currentModel->GetDataObject();

    // 处理文件属性
    iGame::Property::Pointer p;
    std::string filePath("");
    if ((p = obj->GetProperties()->GetProperty("FilePath")) != nullptr) { filePath = p->Get<std::string>(); }

    size_t lastSlashPos = filePath.find_last_of("/\\");
    QString directory, fileName;
    if (lastSlashPos == std::string::npos) {
        directory = "(n/a)";
        if (filePath.length() == 0) {
            fileName = QString::fromStdString(obj->GetName());
            if (fileName.length() == 0) { fileName = "(n/a)"; }
        } else {
            fileName = QString::fromStdString(filePath);
        }
    } else {
        directory = QString::fromStdString(filePath.substr(0, lastSlashPos));
        fileName = QString::fromStdString(filePath.substr(lastSlashPos + 1));
    }

    frameLayout->addWidget(createLabel("File Properties"));
    frameLayout->addWidget(createSeparator());
    frameLayout->addWidget(createPropertyLabel("Name", fileName));
    frameLayout->addWidget(createPropertyLabel("Path", directory));

    // 数据统计
    frameLayout->addWidget(createLabel("Data Statistics"));
    frameLayout->addWidget(createSeparator());
    if (obj->HasSubDataObject()) {
        frameLayout->addWidget(createPropertyLabel("Type", "Multiblock Mesh"));
        frameLayout->addWidget(createPropertyLabel("# of blocks", QString::number(obj->GetNumberOfSubDataObjects())));
    } else {
        CreateDataObjectLayoutInfo(obj);
    }

    IGsize memorySize = obj->GetRealMemorySize();
    QString dw[5] = {"B", "KB", "MB", "GB", "TB"};
    igIndex index = 0;
    while (memorySize > 1024 && index < 4) {
        memorySize /= 1024;
        index++;
    }
    frameLayout->addWidget(createPropertyLabel("Memory", QString::number(memorySize) + dw[index]));

    // 处理边界框
    std::string str;
    iGame::BoundingBox bound = obj->GetBoundingBox();
    for (int i = 0; i < 3; i++) {
        float min = bound.min[i];
        float max = bound.max[i];

        std::stringstream stream;
        stream << std::fixed << std::setprecision(2);
        if (i == 0) {
            stream << "Bounds " << std::setw(8) << min << " to " << std::setw(8) << max << " delta: " << std::setw(8)
                   << (max - min);
        } else {
            stream << "      " << std::setw(8) << min << " to " << std::setw(8) << max << " delta: " << std::setw(8)
                   << (max - min);
        }
        str = stream.str();
        QLabel* label = new QLabel(QString::fromStdString(str));
        frameLayout->addWidget(label);
    }


    // 恢复界面更新
    this->setUpdatesEnabled(true);
    this->updateGeometry();
    // 修改布局
    frameLayout->blockSignals(true);
}

QLabel* igQtModelInformationWidget::createLabel(const QString& text) {
    QLabel* label = new QLabel(text);
    label->setStyleSheet("font-weight: bold;");
    label->setWordWrap(false);                                            // 禁用换行
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);   // 允许水平压缩
    return label;
}

QLabel* igQtModelInformationWidget::createPropertyLabel(const QString& name, const QString& value) {
    QLabel* label = new QLabel(QString("%1: %2").arg(name, value));
    label->setWordWrap(false);                                            // 禁用换行
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);   // 允许水平压缩
    return label;
}
QFrame* igQtModelInformationWidget::createSeparator() {
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}