#include "IQWidgets/igQtModelInformationWidget.h"
#include "iGameSceneManager.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <filesystem>

namespace {
/** 在路径分隔符后插入零宽空格，便于 QLabel 在窄 dock 内按段换行（路径通常不含空格）。 */
QString pathForLabelWrap(const QString& path) {
    if (path.isEmpty() || path == QLatin1String("(n/a)")) return path;
    QString out;
    out.reserve(path.size() + path.size() / 8 + 8);
    for (QChar c : path) {
        out.append(c);
        if (c == QLatin1Char('/') || c == QLatin1Char('\\')) out.append(QChar(0x200B));
    }
    return out;
}
} // namespace

igQtModelInformationWidget::igQtModelInformationWidget(QWidget* parent) : QWidget(parent) {
    // Layout for the main widget
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    // Creating frames and adding them to the main layout
    this->scrollArea = new QScrollArea(this);
    this->scrollArea->setWidgetResizable(true);
    this->scrollArea->setFrameShape(QFrame::NoFrame);
    this->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->informationFrame = new QFrame();
    this->informationFrame->setFrameShape(QFrame::StyledPanel);
    this->frameLayout = new QVBoxLayout(this->informationFrame);
    this->frameLayout->setContentsMargins(8, 8, 8, 8);
    this->frameLayout->setSpacing(6);
    this->scrollArea->setWidget(this->informationFrame);
    mainLayout->addWidget(this->scrollArea);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

void igQtModelInformationWidget::CreateDataObjectLayoutInfo(iGame::DataObject::Pointer obj, QFormLayout* formLayout) {
    if (!formLayout) {
        return;
    }
    switch (obj->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto mesh = iGame::DynamicCast<iGame::SurfaceMesh>(obj);
            createPropertyLabel(formLayout, "Type", "Surface Mesh");
            createPropertyLabel(formLayout, "# of Faces", QString::number(mesh->GetNumberOfFaces()));
            createPropertyLabel(formLayout, "# of Points", QString::number(mesh->GetNumberOfPoints()));
        } break;
        case IG_VOLUME_MESH: {
            auto mesh = iGame::DynamicCast<iGame::VolumeMesh>(obj);
            createPropertyLabel(formLayout, "Type", "Volume Mesh");
            createPropertyLabel(formLayout, "# of Volumes", QString::number(mesh->GetNumberOfVolumes()));
            createPropertyLabel(formLayout, "# of Points", QString::number(mesh->GetNumberOfPoints()));
        } break;
        case IG_STRUCTURED_MESH: {
            auto mesh = iGame::DynamicCast<iGame::StructuredMesh>(obj);
            auto size = mesh->GetDimensionSize();
            createPropertyLabel(formLayout, "Type", "Structured Mesh");
            createPropertyLabel(formLayout, "# of Dimesion X ", QString::number(size[0]));
            createPropertyLabel(formLayout, "# of Dimesion Y ", QString::number(size[1]));
            createPropertyLabel(formLayout, "# of Dimesion Z ", QString::number(size[2]));
        } break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
            createPropertyLabel(formLayout, "Type", "Unstructured Mesh");
            createPropertyLabel(formLayout, "# of Cells", QString::number(mesh->GetNumberOfCells()));
            createPropertyLabel(formLayout, "# of Points", QString::number(mesh->GetNumberOfPoints()));
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
    if (!scene) {
        frameLayout->blockSignals(false);
        this->setUpdatesEnabled(true);
        return;
    }

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
    QWidget* filePropWidget = new QWidget(this->informationFrame);
    QFormLayout* filePropForm = new QFormLayout(filePropWidget);
    filePropForm->setContentsMargins(0, 0, 0, 0);
    filePropForm->setHorizontalSpacing(10);
    filePropForm->setVerticalSpacing(6);
    filePropForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    createPropertyLabel(filePropForm, "Name", pathForLabelWrap(fileName));
    createPropertyLabel(filePropForm, "Path", pathForLabelWrap(directory));
    frameLayout->addWidget(filePropWidget);

    // 数据统计
    frameLayout->addWidget(createLabel("Data Statistics"));
    frameLayout->addWidget(createSeparator());
    QWidget* statWidget = new QWidget(this->informationFrame);
    QFormLayout* statForm = new QFormLayout(statWidget);
    statForm->setContentsMargins(0, 0, 0, 0);
    statForm->setHorizontalSpacing(10);
    statForm->setVerticalSpacing(6);
    statForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    if (obj->HasSubDataObject()) {
        createPropertyLabel(statForm, "Type", "Multiblock Mesh");
        createPropertyLabel(statForm, "# of blocks", QString::number(obj->GetNumberOfSubDataObjects()));
    } else {
        CreateDataObjectLayoutInfo(obj, statForm);
    }

    IGsize memorySize = obj->GetRealMemorySize();
    QString dw[5] = {"B", "KB", "MB", "GB", "TB"};
    igIndex index = 0;
    while (memorySize > 1024 && index < 4) {
        memorySize /= 1024;
        index++;
    }
    createPropertyLabel(statForm, "Memory", QString::number(memorySize) + dw[index]);
    frameLayout->addWidget(statWidget);

    // 处理边界框（并入表单布局，保持与其它项一致对齐）
    iGame::BoundingBox bound = obj->GetBoundingBox();
    const char* axisNames[3] = {"Bounds X", "Bounds Y", "Bounds Z"};
    for (int i = 0; i < 3; i++) {
        const float min = bound.min[i];
        const float max = bound.max[i];
        const float delta = max - min;
        const QString boundsValue = QString("%1 to %2  (delta: %3)")
                                            .arg(min, 0, 'f', 2)
                                            .arg(max, 0, 'f', 2)
                                            .arg(delta, 0, 'f', 2);
        createPropertyLabel(statForm, axisNames[i], boundsValue);
    }


    // 底部弹性区保证内容在较少时贴顶显示
    frameLayout->addStretch();

    // 恢复界面更新
    this->setUpdatesEnabled(true);
    // 修改布局
    frameLayout->blockSignals(false);
}

QLabel* igQtModelInformationWidget::createLabel(const QString& text) {
    QLabel* label = new QLabel(text);
    label->setWordWrap(false);                                            // 禁用换行
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);   // 允许水平压缩
    label->setStyleSheet(R"(
        QLabel {
            font-size: 14px !important;
            color: #FFFFFF !important; /* 纯纯白，匹配iOS的#FFFFFF */
        }
    )");
    return label;
}

void igQtModelInformationWidget::createPropertyLabel(QFormLayout* formLayout, const QString& name, const QString& value) {
    QLabel* nameLabel = new QLabel(name + ":");
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setMinimumWidth(120);
    nameLabel->setStyleSheet("QLabel { font-size: 14px; color: #C8C8C8; }");

    QLabel* valueLabel = new QLabel(value);
    valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    valueLabel->setWordWrap(true);
    valueLabel->setMinimumWidth(0);
    valueLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    valueLabel->setStyleSheet("QLabel { font-size: 14px; color: #FFFFFF; }");
    formLayout->addRow(nameLabel, valueLabel);
}
QFrame* igQtModelInformationWidget::createSeparator() {
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    //添加修改样式
    line->setStyleSheet(R"(
        QWidget {
            background-color: #3A3A3A !important; /* 深灰分割线，适配深色主题 */
            height: 1px !important;
            margin: 4px 0 !important;
        }
    )");
    return line;
}