#include "IQCore/igQtMainWindow.h"
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQComponents/Dialog/igQtChromeFramelessDialog.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQWidgets/igQtTextureMapToPlaneWidget.h>
#include <GenerateSurfaceTangents/iGameGenerateSurfaceTangentsFilter.h>
#include <GroupDatasets/iGameGroupDatasetsFilter.h>
#include <PassArrays/iGamePassArraysFilter.h>
#include <AttributeManipulation/iGamePerlinNoiseFilter.h>
#include <TextureMapToSphere/iGameTextureMapToSphereFilter.h>
#include <iGameSceneManager.h>
#include <iGamePointSet.h>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <set>

using namespace iGame;

static std::string UniqueGroupDatasetsName() {
    std::set<std::string> names;
    auto scene = SceneManager::Instance()->GetCurrentScene();
    if (scene && scene->GetModelList()) {
        auto models = scene->GetModelList();
        for (auto it = models->Begin(); it != models->End(); ++it)
            if (it->second && it->second->GetDataObject()) names.insert(it->second->GetDataObject()->GetName());
    }
    int n = 1;
    while (names.count("GroupDatasets_" + std::to_string(n))) ++n;
    return "GroupDatasets_" + std::to_string(n);
}

bool igQtMainWindow::connectAdditionalFilterAction(QAction* action, const QString& filterId) {
    auto currentFilterInput = [this](const QString& title) -> DataObject::Pointer {
        auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto obj = model ? model->GetDataObject() : nullptr;
        if (!obj) {
            showDarkFramelessMessage(title, QStringLiteral("请先加载并选择一个模型。"));
            return nullptr;
        }
        return obj;
    };

    auto refreshFilterResult = [this](DataObject::Pointer input, DataObject::Pointer output,
                                      const QString& title, bool inPlace = false) {
        if (!output) {
            showDarkFramelessMessage(title, QStringLiteral("Filter 未产生有效输出。"));
            return;
        }

        if (inPlace || output.GetPointer() == input.GetPointer()) {
            modelTreeWidget->updateAllAttriubute(output);
            if (auto drawObj = DynamicCast<DrawObject>(output)) { drawObj->ForceReConvertToDrawableData(); }
        } else {
            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
        }
        rendererWidget->update();
        showDarkFramelessMessage(title, QStringLiteral("Filter 执行完成。"), true);
    };

        if (filterId == QStringLiteral("generate_surface_tangents")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        auto data = model->GetDataObject();
        if (data == nullptr || data->GetPoints() == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前模型没有点数据，无法计算表面切向量。"));
            return;
        }

        // 先把模型里的纹理坐标点属性（IG_TCOORD，至少 2 个分量）列出来给用户选
        auto attrs = data->GetAttributeSet();
        std::vector<QString> texLabels;
        std::vector<std::string> texNames;
        if (attrs) {
            for (IGsize i = 0; i < static_cast<IGsize>(attrs->GetNumberOfAttributes()); ++i) {
                auto& attr = attrs->GetAttribute(i);
                if (attr.isDeleted || !attr.pointer) continue;
                if (attr.attachmentType != IG_POINT || attr.type != IG_TCOORD) continue;
                if (attr.pointer->GetDimension() < 2) continue;
                texNames.push_back(attr.pointer->GetName());
                texLabels.push_back(QStringLiteral("%1（%2 个分量）")
                                            .arg(QString::fromStdString(attr.pointer->GetName()))
                                            .arg(attr.pointer->GetDimension()));
            }
        }
        if (texLabels.empty()) {
            showDarkFramelessMessage(
                    QStringLiteral("Warning"),
                    QStringLiteral("当前模型没有纹理坐标（IG_TCOORD 点属性），<br>"
                                   "切向量必须由纹理坐标算出来。<br>"
                                   "请先做一次「球面纹理坐标 (Texture Map To Sphere)」之类的 filter。"));
            return;
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("表面切向量 (Generate Surface Tangents)"));
        int texId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("纹理坐标"), texLabels);
        int pointId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                           QStringLiteral("输出点切向量 Tangents"), "true");
        int cellId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                          QStringLiteral("输出单元切向量（不归一化）"), "false");
        int bitangentId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                               QStringLiteral("输出副切向量 Bitangents"), "false");
        dialog->setApplyFunctor([=, this]() {
            bool okTex = false, okPoint = false, okCell = false, okBitangent = false;
            const int texIndex = dialog->getComboIndex(texId, okTex);
            const bool computePoint = dialog->getChecked(pointId, okPoint);
            const bool computeCell = dialog->getChecked(cellId, okCell);
            const bool computeBitangent = dialog->getChecked(bitangentId, okBitangent);
            if (!okPoint || !okCell || !okBitangent) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("参数读取失败"));
                return;
            }
            if (!computePoint && !computeCell && !computeBitangent) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("至少勾选一种切向量输出"));
                return;
            }

            auto filter = iGame::GenerateSurfaceTangentsFilter::New();
            if (okTex && texIndex >= 0 && texIndex < static_cast<int>(texNames.size())) {
                filter->SetTextureCoordinatesArrayName(texNames[texIndex]);
            }
            filter->SetComputePointTangents(computePoint);
            filter->SetComputeCellTangents(computeCell);
            filter->SetComputeBitangents(computeBitangent);
            filter->SetInput(0, data);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("表面切向量执行失败：<br>"
                                                        "模型需要有纹理坐标，以及三角形 / 四边形 / 多边形单元"
                                                        "（体网格请先「转换为表面网格」）。<br>"
                                                        "细节见日志。"));
                return;
            }
            auto outObj = filter->GetOutput(); // 独立输出，原模型不变
            if (auto drawObject = iGame::DynamicCast<iGame::DrawObject>(outObj)) {
                drawObject->ForceReConvertToDrawableData();
            }
            modelTreeWidget->addDataObjectToModelTree(outObj, Algorithm);
            // 把新节点选成当前模型
            if (auto item = modelTreeWidget->getItemFromObject(outObj)) { modelTreeWidget->setCurrentItem(item); }
            rendererWidget->update();
            showDarkFramelessMessage(
                    QStringLiteral("表面切向量"),
                    QStringLiteral("已生成点属性 Tangents（单位向量，方向沿纹理坐标 u 增大的方向）。<br>"), true);
            dialog->close();
        });
        dialog->show();
    });
            return true;
        }
        if (filterId == QStringLiteral("group_datasets")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                // 1) 读取模型树中当前选中的数据对象（支持多选，也支持多块装配体的子块）
                auto inputs = modelTreeWidget->GetSelectedDataObjects();
                if (inputs.empty()) {
                    showDarkFramelessMessage(
                            QStringLiteral("合并数据集"),
                            QStringLiteral("请先在左侧模型树中选中对象。\n\n"
                                           "• 多选：按住 Ctrl 或 Shift 点击\n"
                                           "• 支持选中多块装配体的子块"));
                    return;
                }

                // 2) 唯一命名。必须在注册进场景【之前】算，因为它扫描的是场景的模型列表。
                const std::string groupName = UniqueGroupDatasetsName();

                // 3) 执行组合（只做容器组合，不合并几何）
                auto filter = iGame::GroupDatasetsFilter::New();
                filter->SetInputs(inputs);
                filter->SetOutputName(groupName);
                if (!filter->Execute()) {
                    showDarkFramelessMessage(QStringLiteral("合并数据集失败"),
                                             QString::fromStdString(filter->GetMessage()));
                    return;
                }

                auto group = filter->GetOutput(0);
                if (!group) {
                    showDarkFramelessMessage(QStringLiteral("合并数据集失败"),
                                             QStringLiteral("未生成结果节点。"));
                    return;
                }

                // 4) 挂到场景 + 模型树。
                //    注意：addDataObjectToModelTree 内部已经调用 scene->AddModel()，
                //    这里不要再单独 AddModel，否则会重复注册出冗余节点。
                modelTreeWidget->addDataObjectToModelTree(group, ItemSource::Algorithm);

                // 5) 【不隐藏原始输入】。
                //
                //    组的每个子块都是原模型的【深拷贝克隆】（几何 / 属性 / 显隐样式各持一份），
                //    隐藏原模型不会影响组的内容，隐藏组内的块也不会影响原模型 —— 新旧完全独立。
                //    因此原模型保持原状即可；这里仅刷新全树眼睛图标，让每处图标都反映
                //    各自对象的真实显隐状态。
                modelTreeWidget->RefreshModelTreeVisibilityIcons();

                rendererWidget->update();

                // 6) 汇报结果（子块按序号列出，重名也能区分）
                QString blockList;
                const auto& blockNames = filter->GetBlockNames();
                for (size_t i = 0; i < blockNames.size(); ++i) {
                    blockList += QStringLiteral("%1. %2\n")
                                         .arg(static_cast<int>(i) + 1)
                                         .arg(QString::fromStdString(blockNames[i]));
                }
                const QString report = QStringLiteral("已合并 %1 个对象 → %2\n%3")
                                               .arg(filter->GetGroupedCount())
                                               .arg(QString::fromStdString(groupName))
                                               .arg(blockList);
                showDarkFramelessMessage(QStringLiteral("合并数据集"), report, true);
            });
            return true;
        }
        if (filterId == QStringLiteral("pass_arrays")) {
            connect(action, &QAction::triggered, this, [=, this]() {
        const QString title = QStringLiteral("传递过滤数据数组");
        auto input = currentFilterInput(title);
        if (!input) return;

        auto attrSet = input->GetAttributeSet();
        if (attrSet == nullptr) {
            showDarkFramelessMessage(title, QStringLiteral("当前模型没有属性集。"));
            return;
        }

        // 收集属性名称：点数据与单元数据分开收集，后续也分开记录勾选结果
        QStringList pointNames;
        QStringList cellNames;
        auto collectNames = [](auto attrs, QStringList& out) {
            if (!attrs) return;
            for (IGsize i = 0; i < attrs->GetNumberOfElements(); ++i) {
                auto& attr = attrs->GetElement(i);
                if (attr.IsNone() || !attr.pointer) continue;
                out.append(QString::fromStdString(attr.pointer->GetName()));
            }
        };
        collectNames(attrSet->GetAllPointAttributes(), pointNames);
        collectNames(attrSet->GetAllCellAttributes(), cellNames);
        if (pointNames.isEmpty() && cellNames.isEmpty()) {
            showDarkFramelessMessage(title, QStringLiteral("当前模型没有任何属性可传递。"));
            return;
        }

        // ---------- 属性多选框对话框 ----------
        igQtChromeFramelessDialog* dlg = new igQtChromeFramelessDialog(this);
        dlg->setDialogTitle(QStringLiteral("选择要传递的属性"));
        dlg->setMaximizeEnabled(false);
        dlg->setAttribute(Qt::WA_DeleteOnClose);

        QWidget* content = new QWidget(dlg->contentHost());
        content->setStyleSheet(
                "QWidget { background-color: #252526; color: #D4D4D4; }"
                "QCheckBox { color: #D4D4D4; spacing: 6px; }"
                "QCheckBox::indicator { width: 16px; height: 16px; }"
                "QCheckBox::indicator:unchecked { border: 1px solid #6A6A6A; background-color: #3A3A3A; }"
                "QCheckBox::indicator:checked { border: 1px solid #0E639C; background-color: #0E639C; }"
                "QCheckBox::indicator:unchecked:hover { border: 1px solid #9A9A9A; }"
                "QCheckBox::indicator:checked:hover { background-color: #1177BB; }"
                "QPushButton { background-color: #3A3A3A; color: #D4D4D4; border: 1px solid #4A4A4A; "
                "              padding: 6px 12px; border-radius: 4px; }"
                "QPushButton:hover { background-color: #4A4A4A; border-color: #5A5A5A; }"
                "QPushButton:pressed { background-color: #2A2A2A; }"
                "QScrollArea { background-color: #1E1E1E; border: none; }"
                "QScrollBar:vertical { background: #1E1E1E; width: 10px; margin: 0; }"
                "QScrollBar::handle:vertical { background: #4A4A4A; border-radius: 5px; min-height: 20px; }"
                "QScrollBar::handle:vertical:hover { background: #5A5A5A; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");
        QVBoxLayout* mainLayout = new QVBoxLayout(content);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);

        QLabel* tip = new QLabel(
                QStringLiteral("勾选需要传递到输出的属性，未勾选的属性会被丢弃。"),
                content);
        tip->setWordWrap(true);
        mainLayout->addWidget(tip);

        QScrollArea* scrollArea = new QScrollArea(content);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        QWidget* listWidget = new QWidget(scrollArea);
        QVBoxLayout* listLayout = new QVBoxLayout(listWidget);
        listLayout->setSpacing(4);
        listLayout->setContentsMargins(0, 0, 0, 0);

        QList<QCheckBox*> pointBoxes;
        QList<QCheckBox*> cellBoxes;
        auto addSection = [&](const QString& header, const QStringList& names, const QString& emptyTip,
                              QList<QCheckBox*>& boxes) {
            QLabel* headerLabel = new QLabel(header, listWidget);
            QFont headerFont = headerLabel->font();
            headerFont.setBold(true);
            headerLabel->setFont(headerFont);
            listLayout->addWidget(headerLabel);

            if (names.isEmpty()) {
                listLayout->addWidget(new QLabel(emptyTip, listWidget));
                return;
            }

            QHBoxLayout* btnRow = new QHBoxLayout();
            btnRow->setContentsMargins(0, 0, 0, 0);
            QPushButton* selectAllBtn = new QPushButton(QStringLiteral("全选"), listWidget);
            QPushButton* deselectAllBtn = new QPushButton(QStringLiteral("全不选"), listWidget);
            btnRow->addWidget(selectAllBtn);
            btnRow->addWidget(deselectAllBtn);
            btnRow->addStretch();
            listLayout->addLayout(btnRow);

            for (const QString& name: names) {
                QCheckBox* cb = new QCheckBox(name, listWidget);
                cb->setChecked(true); // 默认全部选中
                boxes.append(cb);
                listLayout->addWidget(cb);
            }
            // 按值捕获复选框列表：列表内容此时已经填充完毕
            const QList<QCheckBox*> sectionBoxes = boxes;
            connect(selectAllBtn, &QPushButton::clicked, listWidget,
                    [sectionBoxes]() { for (QCheckBox* cb: sectionBoxes) cb->setChecked(true); });
            connect(deselectAllBtn, &QPushButton::clicked, listWidget,
                    [sectionBoxes]() { for (QCheckBox* cb: sectionBoxes) cb->setChecked(false); });
        };
        addSection(QStringLiteral("点数据 (Point Data)"), pointNames,
                   QStringLiteral("（无点属性）"), pointBoxes);
        addSection(QStringLiteral("单元数据 (Cell Data)"), cellNames,
                   QStringLiteral("（无单元属性）"), cellBoxes);
        listWidget->setLayout(listLayout);
        scrollArea->setWidget(listWidget);
        mainLayout->addWidget(scrollArea);

        QDialogButtonBox* buttonBox =
                new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, content);
        mainLayout->addWidget(buttonBox);

        dlg->setContentWidget(content);
        dlg->resize(420, 560);

        connect(buttonBox, &QDialogButtonBox::accepted, this,
                [this, dlg, input, pointBoxes, cellBoxes, pointNames, cellNames, title,
                 refreshFilterResult]() {
            std::vector<std::string> selectedPointNames;
            std::vector<std::string> selectedCellNames;
            for (int i = 0; i < pointBoxes.size(); ++i) {
                if (pointBoxes[i]->isChecked()) selectedPointNames.push_back(pointNames[i].toStdString());
            }
            for (int i = 0; i < cellBoxes.size(); ++i) {
                if (cellBoxes[i]->isChecked()) selectedCellNames.push_back(cellNames[i].toStdString());
            }
            if (selectedPointNames.empty() && selectedCellNames.empty()) {
                showDarkFramelessMessage(title, QStringLiteral("请至少勾选一个属性。"));
                return;
            }

            auto filter = PassArrays::New();
            filter->SetInput(input);
            // 点数据与单元数据的勾选结果分别记录，同名属性不会互相牵连
            filter->SetPointArrayNames(selectedPointNames);
            filter->SetCellArrayNames(selectedCellNames);
            if (!filter->Execute()) {
                showDarkFramelessMessage(title, QStringLiteral("数组过滤失败：输入数据类型不受支持。"));
                return;
            }
            auto output = filter->GetOutput();
            if (!output) {
                showDarkFramelessMessage(title, QStringLiteral("未得到输出数据。"));
                return;
            }
            output->SetName(input->GetName() + "_filtered");
            refreshFilterResult(input, output, title);
            dlg->accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

        dlg->show();
    });
            return true;
        }
        if (filterId == QStringLiteral("perlin_noise")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("Perlin 噪声"), QStringLiteral("请先选择一个模型。"));
                    return;
                }
                auto object = currentModel->GetDataObject();
                if (object == nullptr || iGame::DynamicCast<iGame::PointSet>(object).IsNull()) {
                    showDarkFramelessMessage(QStringLiteral("Perlin 噪声"),
                                             QStringLiteral("当前对象不支持 Perlin 噪声（需要网格 / 点集）"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("Perlin 噪声"));
                dialog->setFilterDescription(
                        QStringLiteral("在每个输入点上采样 3D Perlin 噪声，输出新模型 + 点标量数组"));
                const int ampId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                       QStringLiteral("振幅 (Amplitude)"), "1");
                const int fxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("频率 X (Frequency X)"), "1");
                const int fyId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("频率 Y (Frequency Y)"), "1");
                const int fzId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("频率 Z (Frequency Z)"), "1");
                const int pxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("相位 X (Phase X)"), "0");
                const int pyId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("相位 Y (Phase Y)"), "0");
                const int pzId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("相位 Z (Phase Z)"), "0");
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    const int ids[7] = {ampId, fxId, fyId, fzId, pxId, pyId, pzId};
                    const char* names[7] = {"振幅 (Amplitude)", "频率 X (Frequency X)",
                                            "频率 Y (Frequency Y)", "频率 Z (Frequency Z)",
                                            "相位 X (Phase X)", "相位 Y (Phase Y)",
                                            "相位 Z (Phase Z)"};
                    double values[7] = {0.0};
                    bool ok = false;
                    for (int i = 0; i < 7; ++i) {
                        values[i] = dialog->getDouble(ids[i], ok);
                        if (!ok) {
                            showDarkFramelessMessage(
                                    QStringLiteral("错误"),
                                    QString::fromUtf8(names[i]) + QStringLiteral(" 不是有效数字。"));
                            return;
                        }
                    }

                    auto filter = PerlinNoiseFilter::New();
                    filter->SetAmplitude(values[0]);
                    filter->SetFrequency(values[1], values[2], values[3]);
                    filter->SetPhase(values[4], values[5], values[6]);
                    filter->SetInput(object);
                    if (filter->Execute()) {
                        modelTreeWidget->addDataObjectToModelTree(filter->GetOutput(), ItemSource::Algorithm);
                        modelTreeWidget->updateCloudPicture();
                        rendererWidget->update();
                        dialog->close();
                    } else {
                        showDarkFramelessMessage(QStringLiteral("错误"),
                                                 QStringLiteral("Perlin 噪声生成失败（需要网格 / 点集）"));
                    }
                });
            });
            return true;
        }
        if (filterId == QStringLiteral("texture_map_to_plane")) {
            connect(action, &QAction::triggered, this, [this]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (!model) {
            showDarkFramelessMessage(QStringLiteral("生成平面纹理坐标"),
                                     QStringLiteral("请先在模型树中选择一个包含点数据的网格模型。"));
            return;
        }
        auto* panel = new igQtTextureMapToPlaneWidget(rendererWidget, modelTreeWidget, model, this);
        if (!panel->isReady()) {
            showDarkFramelessMessage(QStringLiteral("生成平面纹理坐标"),
                                     QStringLiteral("当前模型不属于 PointSet 网格体系，或模型中没有点；操作已取消。"));
            panel->deleteLater();
            return;
        }
        panel->show();
        panel->raise();
        panel->activateWindow();
    });
            return true;
        }
        if (filterId == QStringLiteral("texture_map_to_sphere")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        auto data = model->GetDataObject();
        if (data == nullptr || data->GetPoints() == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前模型没有点数据，无法生成球面纹理坐标。"));
            return;
        }

        // 默认值与算法的自动球心一致：所有输入点的坐标平均值。
        double center[3] = {0.0, 0.0, 0.0};
        auto spherePoints = data->GetPoints();
        const IGsize spherePointCount = spherePoints->GetNumberOfPoints();
        for (IGsize i = 0; i < spherePointCount; ++i) {
            const auto& point = spherePoints->GetPoint(i);
            for (int axis = 0; axis < 3; ++axis) { center[axis] += static_cast<double>(point[axis]); }
        }
        if (spherePointCount > 0) {
            for (double& value : center) { value /= static_cast<double>(spherePointCount); }
        }
        const QString defaultX = QString::number(center[0], 'g', 6);
        const QString defaultY = QString::number(center[1], 'g', 6);
        const QString defaultZ = QString::number(center[2], 'g', 6);

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("球面纹理坐标 (Texture Map To Sphere)"));
        int autoCenterId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                QStringLiteral("自动使用点坐标平均值"), "true");
        int centerXId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("球心 X"),
                                             defaultX);
        int centerYId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("球心 Y"),
                                             defaultY);
        int centerZId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("球心 Z"),
                                             defaultZ);
        int seamId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                          QStringLiteral("防止接缝 (PreventSeam)"), "true");
        dialog->setApplyFunctor([=, this]() {
            bool okAuto = false, okSeam = false;
            const bool autoCenter = dialog->getChecked(autoCenterId, okAuto);
            const bool preventSeam = dialog->getChecked(seamId, okSeam);

            auto filter = iGame::TextureMapToSphereFilter::New();
            filter->SetPreventSeam(preventSeam);
            if (!autoCenter) {
                bool okX = false, okY = false, okZ = false;
                const double cx = dialog->getDouble(centerXId, okX);
                const double cy = dialog->getDouble(centerYId, okY);
                const double cz = dialog->getDouble(centerZId, okZ);
                if (!okX || !okY || !okZ) {
                    showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("球心坐标必须是数字"));
                    return;
                }
                filter->SetCenter(cx, cy, cz);
            }
            filter->SetInput(0, data);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("球面纹理坐标执行失败：不支持的网格类型"));
                return;
            }
            auto outObj = filter->GetOutput(); // 独立输出，原模型不变
            if (auto drawObject = iGame::DynamicCast<iGame::DrawObject>(outObj)) {
                drawObject->ForceReConvertToDrawableData();
            }
            modelTreeWidget->addDataObjectToModelTree(outObj, ItemSource::Algorithm);
            rendererWidget->update();
            showDarkFramelessMessage(QStringLiteral("球面纹理坐标"),
                                     QStringLiteral("已生成点属性 TextureCoordinates（2 分量：s、t）。<br>"
                                                    "可在“查找信息”面板里查看数值。"),
                                     true);
            dialog->close();
        });
        dialog->show();
    });
            return true;
        }
    return false;
}
