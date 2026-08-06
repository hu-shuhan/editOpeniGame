#include "StreamView/iGameStreamlineSimplifier.h"
#include "iGameSelection.h"
#include <IQWidgets/igQtStreamTracerWidget.h>
#include <iGameBoxStyle.h>
#include <iGameSceneManager.h>


using namespace iGame;

bool igQtStreamTracerWidget::isUsableSource(const iGame::Model::Pointer& m,
                                           const iGame::DataObject::Pointer& curResult) {
    if (!m) return false;
    auto obj = m->GetDataObject();
    if (!obj) return false;
    // 排除当前那个流线结果本身
    if (curResult && obj.GetPointer() == curResult.GetPointer()) return false;
    // 排除任何以前生成的流线结果（名字里含 _StreamLine）——
    // 它们通常是仅有 line cell 的 UnstructuredMesh，走 TransferToVolumeMesh 会得到 0 体单元
    if (obj->GetName().find("_StreamLine") != std::string::npos) return false;
    // 必须是含体单元的 VolumeMesh / UnstructuredMesh
    if (auto vol = iGame::DynamicCast<iGame::VolumeMesh>(obj)) { return vol->GetNumberOfVolumes() > 0; }
    if (auto um = iGame::DynamicCast<iGame::UnstructuredMesh>(obj)) {
        // 流线积分要求 3D 体单元：逐个单元判维度，只要有非 3D 单元就不是合法流场输入
        //（TransferToVolumeMesh 用的是同一条判据，这里提前判掉以便给出明确提示）
        auto cells = um->GetCells();
        auto types = um->GetCellTypes();
        if (!cells || cells->GetNumberOfCells() == 0) return false;
        const igIndex cellNum = static_cast<igIndex>(um->GetNumberOfCells());
        for (igIndex i = 0; i < cellNum; ++i) {
            if (iGame::Cell::GetCellDimension(um->GetCellType(i)) != 3) return false;
        }
        return true;
    }
    return false;
}

// 所有种子模式共用的输入校验：不是 3D 体网格就弹同一个提示。
// 之前这个判断依赖 initStreamTracer 之后 GetMesh() 为空，而 pickSourceModel 会在
// 用户选中 2D 模型时"体贴地"回退到场景里另一个 3D 模型，于是既不报错也不是用户要的结果。
bool igQtStreamTracerWidget::warnUnsupportedSource(const iGame::Model::Pointer& model) {
    if (isUsableSource(model, m_ResultObject)) return true;

    QString name;
    if (model && model->GetDataObject()) { name = QString::fromStdString(model->GetDataObject()->GetName()); }
    QMessageBox::warning(this, tr("Stream Tracer"),
                         tr("模型「%1」不包含 3D 体单元（可能是 2D / 面流场），"
                            "现版本的流线追踪只支持 3D 体网格。已取消生成。")
                                 .arg(name.isEmpty() ? tr("(未命名)") : name));
    return false;
}

iGame::Model::Pointer igQtStreamTracerWidget::pickSourceModel() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (!scene) return nullptr;

    auto isUsable = [&](iGame::Model::Pointer m) -> bool { return isUsableSource(m, m_ResultObject); };

    auto cur = scene->GetCurrentModel();
    auto list = scene->GetModelList();

    // 在 model list 中寻找可用的体网格 model，优先 handle 最大的（最近导入）
    iGame::Model::Pointer newest = nullptr;
    IGuint newestHandle = 0;
    int usableCount = 0;
    if (list) {
        for (auto it = list->Begin(); it != list->End(); ++it) {
            if (!isUsable(it->second)) continue;
            ++usableCount;
            if (!newest || it->first > newestHandle) {
                newest = it->second;
                newestHandle = it->first;
            }
        }
    }

    // 用户当前选中的是一个"真实模型但不是 3D 体网格"（典型：导入了 2D / 面流场）时，
    // 直接把它返回给调用方，由调用方弹出统一提示 —— 而不是悄悄换成场景里别的 3D 模型。
    // 只有当 current 是流线结果对象或为空时，下面的回退规则才是用户想要的"体贴"。
    auto isStreamResult = [&](const iGame::Model::Pointer& m) -> bool {
        if (!m || !m->GetDataObject()) return true; // 空的按"可替换"处理
        auto obj = m->GetDataObject();
        if (m_ResultObject && obj.GetPointer() == m_ResultObject.GetPointer()) return true;
        return obj->GetName().find("_StreamLine") != std::string::npos;
    };
    if (cur && !isUsable(cur) && !isStreamResult(cur)) {
        std::cout << "[pickSourceModel] current model is not a 3D volume mesh -> return it so the caller can warn: "
                  << cur->GetDataObject()->GetName() << std::endl;
        return cur;
    }

    // 规则：
    // 1) 只有一个可用体网格 → 用它（即便 current 指向流线结果）
    // 2) 当前 model 本身可用，且等于上次绑定对象 → 用 current（用户在原模型上重生成）
    // 3) 否则用最新的可用体网格（用户刚导入新模型）
    if (usableCount == 1 && newest) {
        std::cout << "[pickSourceModel] only-one-usable -> " << newest->GetDataObject()->GetName() << std::endl;
        return newest;
    }
    if (isUsable(cur) && m_DataObject &&
        cur->GetDataObject().GetPointer() == m_DataObject.GetPointer()) {
        std::cout << "[pickSourceModel] keep-current -> " << cur->GetDataObject()->GetName() << std::endl;
        return cur;
    }
    if (newest) {
        std::cout << "[pickSourceModel] newest-usable -> " << newest->GetDataObject()->GetName() << std::endl;
        return newest;
    }
    return cur;
}

void igQtStreamTracerWidget::ensureStreamBase() {
    if (m_StreamBase) return;
    m_StreamBase = iGame::StreamBase::New();
    m_StreamBase->DrawObject::AddObserver(iGame::Command::DeleteEvent, [&]() -> void {
        haveDraw = false;
        first = true;
        std::cout << "change first" << first << std::endl;
        this->hide();
    });
}

igQtStreamTracerWidget::igQtStreamTracerWidget(QWidget* parent) : QWidget(parent), ui(new Ui::SteamLineTracer) {
    ui->setupUi(this);
    ui->source->hide();

    connect(ui->control_comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeControl()));
    connect(ui->numOfSeedLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(changenumOfSeeds()));
    connect(ui->lengthOfStreamLine, SIGNAL(textChanged(const QString&)), this, SLOT(changelengthOfStreamLine()));
    connect(ui->lineWidth, SIGNAL(textChanged(const QString&)), this, SLOT(changeWidthOfStreamLine()));
    connect(ui->lengthOfStep, SIGNAL(textChanged(const QString&)), this, SLOT(changelengthOfStep()));
    connect(ui->maxSteps, SIGNAL(textChanged(const QString&)), this, SLOT(changemaxSteps()));
    connect(ui->startX, &QLineEdit::editingFinished, this, &igQtStreamTracerWidget::changeStart);
    connect(ui->startY, &QLineEdit::editingFinished, this, &igQtStreamTracerWidget::changeStart);
    connect(ui->startZ, &QLineEdit::editingFinished, this, &igQtStreamTracerWidget::changeStart);
    ui->startX->setText("0");
    ui->startY->setText("0");
    ui->startZ->setText("0");
    connect(ui->endX, &QLineEdit::editingFinished, this, &igQtStreamTracerWidget::changeEnd);
    connect(ui->endY, &QLineEdit::editingFinished, this, &igQtStreamTracerWidget::changeEnd);
    connect(ui->endZ, &QLineEdit::editingFinished, this, &igQtStreamTracerWidget::changeEnd);

    connect(ui->splitX, SIGNAL(textChanged(const QString&)), this, SLOT(changeSplit()));
    connect(ui->splitY, SIGNAL(textChanged(const QString&)), this, SLOT(changeSplit()));
    connect(ui->splitZ, SIGNAL(textChanged(const QString&)), this, SLOT(changeSplit()));


    ui->endX->setText("0");
    ui->endY->setText("0");
    ui->endZ->setText("0");

    ui->splitX->setText("6");
    ui->splitY->setText("6");
    ui->splitZ->setText("6");

    connect(ui->terminalSpeed, SIGNAL(textChanged(const QString&)), this, SLOT(changeterminalSpeed()));


    connect(ui->comboBox, &QComboBox::currentTextChanged, this, [&]() { this->changeVecName(); });

    connect(ui->generate_streamline_btn, &QPushButton::clicked, this, &igQtStreamTracerWidget::generateStreamline);
    connect(ui->refreshBtn, &QPushButton::clicked, this, &igQtStreamTracerWidget::refresh);
    connect(ui->Cluster, &QPushButton::clicked, this, &igQtStreamTracerWidget::Simplifier);

    numOfSeeds = 200;
    ui->numOfSeedLineEdit->setText("200");
    control = 0;
    haveClicked = false;
    //	 proportion = 0.35;
    // ui->proportion_Slider->setValue(35);
    lengthOfStreamLine = 5;
    ui->lengthOfStreamLine->setText("5");
    widthOfStreamLine = 3;
    ui->lineWidth->setText("3");
    maxSteps = 1200;
    ui->maxSteps->setText("1200");
    lengthOfStep = 0.05;
    ui->lengthOfStep->setText("0.05");
    terminalSpeed = 0.005;
    ui->terminalSpeed->setText("0.005");
    haveDraw = false;
    haveClicked = true;
    ui->control_comboBox->setCurrentIndex(1);
    streamlineResult = UnstructuredMesh::New();
}

void igQtStreamTracerWidget::refresh() {
    modelBound = false;
    updateVectorNameList();
}
void igQtStreamTracerWidget::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    auto scene = SceneManager::Instance()->GetCurrentScene();
    if (!scene) return;
    auto interactor = scene->GetInteractor();
    if (!interactor) return;
    interactor->RequestBasicStyle();
}
void igQtStreamTracerWidget::showEvent(QShowEvent* event) {
    if (event->spontaneous()) {
        QWidget* p = this->parentWidget();
        QDockWidget* dock = nullptr;
        while (p) {
            dock = qobject_cast<QDockWidget*>(p);
            if (dock) break;
            p = p->parentWidget();
        }

        if (dock && !dock->isFloating()) { return; }
    }
    QWidget::showEvent(event);
    if (isExisted) {
        auto scene = SceneManager::Instance()->GetCurrentScene();
        Selection->Start = startP;
        Selection->End = endP;
        Selection->SetSelectionCallBackEvent(
                [&](IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope) {
                    if (itemType == IG_CHANGE) {
                        startP = Selection->Start;
                        endP = Selection->End;
                        auto temStart = startP;
                        auto temEnd = endP;
                        ui->startX->setText(QString::number(temStart[0]));
                        ui->startY->setText(QString::number(temStart[1]));
                        ui->startZ->setText(QString::number(temStart[2]));
                        ui->endX->setText(QString::number(temEnd[0]));
                        ui->endY->setText(QString::number(temEnd[1]));
                        ui->endZ->setText(QString::number(temEnd[2]));
                    }
                },
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

        scene->GetInteractor()->SetDataObject(m_DataObject);
        scene->GetInteractor()->SetPainter3D(Painter);

        scene->GetInteractor()->RequestStreamLineStyle(Selection);
    }
    std::cout << first << std::endl;
    if (first) {
        std::cout << "do link" << std::endl;
        ensureStreamBase();
        auto sceneManager = iGame::SceneManager::Instance();
        auto scene = sceneManager->GetCurrentScene();
        if (!scene) return;
        auto currentModel = scene->GetCurrentModel();
        if (!currentModel) return;
        auto obj = currentModel->GetDataObject();
        if (!obj) return;
        iGame::VolumeMesh::Pointer mesh;
        if (iGame::DynamicCast<UnstructuredMesh>(obj))
            mesh = iGame::DynamicCast<UnstructuredMesh>(obj)->TransferToVolumeMesh();
        else if (DynamicCast<VolumeMesh>(obj))
            mesh = DynamicCast<VolumeMesh>(obj);
        if (!mesh) return;
        first = false;
    }
    std::cout << "show" << std::endl;
    updateVectorNameList();
}
void igQtStreamTracerWidget::changeControl() {
    control = ui->control_comboBox->currentIndex();
    //std::cout << "current index=" << control <<std::endl;
}
void igQtStreamTracerWidget::changenumOfSeeds() {
    numOfSeeds = ui->numOfSeedLineEdit->text().toInt();
    //std::cout << "current seeds=" << numOfSeeds << std::endl;
}
void igQtStreamTracerWidget::changeStart() {
    startP = Vector3f(ui->startX->text().toFloat(), ui->startY->text().toFloat(), ui->startZ->text().toFloat());
    if (Selection) {
        Selection->Start = startP;
        auto scene = SceneManager::Instance()->GetCurrentScene();
        Painter = scene->GetCurrentModel()->GetPainter3D();
        scene->GetInteractor()->SetDataObject(m_DataObject);
        scene->GetInteractor()->SetPainter3D(Painter);
        scene->GetInteractor()->RequestStreamLineStyle(Selection);
        scene->Update();
    }
    //std::cout << "current seeds=" << numOfSeeds << std::endl;
}
void igQtStreamTracerWidget::changeSplit() {
    splitX = ui->splitX->text().toInt();
    splitY = ui->splitY->text().toInt();
    splitZ = ui->splitZ->text().toInt();
}
void igQtStreamTracerWidget::changeEnd() {
    endP = Vector3f(ui->endX->text().toFloat(), ui->endY->text().toFloat(), ui->endZ->text().toFloat());
    if (Selection) {
        Selection->End = endP;
        auto scene = SceneManager::Instance()->GetCurrentScene();
        Painter = scene->GetCurrentModel()->GetPainter3D();
        scene->GetInteractor()->SetDataObject(m_DataObject);
        scene->GetInteractor()->SetPainter3D(Painter);
        scene->GetInteractor()->RequestStreamLineStyle(Selection);
        scene->Update();
    }
    //std::cout << "current seeds=" << numOfSeeds << std::endl;
}
void igQtStreamTracerWidget::changelengthOfStreamLine() {
    lengthOfStreamLine = ui->lengthOfStreamLine->text().toFloat();
}
void igQtStreamTracerWidget::changeWidthOfStreamLine() { widthOfStreamLine = ui->lineWidth->text().toFloat(); }
void igQtStreamTracerWidget::changelengthOfStep() { lengthOfStep = ui->lengthOfStep->text().toFloat(); }
void igQtStreamTracerWidget::changemaxSteps() { maxSteps = ui->maxSteps->text().toFloat(); }
void igQtStreamTracerWidget::changeterminalSpeed() { terminalSpeed = ui->terminalSpeed->text().toFloat(); }
void igQtStreamTracerWidget::Pressed() {
    haveClicked = false;
    std::cout << "aaa" << std::endl;
}
void igQtStreamTracerWidget::Released() {
    haveClicked = true;
    generateStreamline();
}
//void igQtStreamTracerWidget::changeOffsetP1() {
//    p1 = ui->lineEdit->text().toInt();
//    p2 = ui->lineEdit_2->text().toInt();
//    offsetP1 = Vector3f(ui->lineEdit->text().toFloat(), ui->lineEdit_2->text().toFloat(), ui->lineEdit_3->text().toFloat());
//}
//void igQtStreamTracerWidget::changeOffsetP2() {
//    offsetP2 = Vector3f(ui->lineEdit_4->text().toFloat(), ui->lineEdit_5->text().toFloat(),
//                        ui->lineEdit_6->text().toFloat());
//}
void igQtStreamTracerWidget::changeProportion() {
    //	proportion = ui->proportion_Slider->value();
    //	proportion /= 100;
    if (haveClicked) generateStreamline();
    //std::cout << "current value=" << proportion << std::endl;
}
void igQtStreamTracerWidget::reduceProportion() {
    //	proportion = ui->proportion_Slider->value();
    if (proportion) {
        proportion = proportion - 1;
        //	ui->proportion_Slider->setValue(proportion);
        //std::cout << "current value=" << proportion << std::endl;
    }
}
void igQtStreamTracerWidget::increaseProportion() {
    //proportion = ui->proportion_Slider->value();
    if (proportion < 99) {
        proportion = proportion + 1;
        //	ui->proportion_Slider->setValue(proportion);
    }
    //std::cout << "current value=" << proportion << std::endl;
}
void igQtStreamTracerWidget::updateVectorNameList() {
    ui->comboBox->clear();
    auto sceneManager = iGame::SceneManager::Instance();
    auto scene = sceneManager->GetCurrentScene();
    if (!scene) return;
    auto currentModel = pickSourceModel();
    if (!currentModel) return;
    auto obj = currentModel->GetDataObject();
    if (!obj) return;
    //startP = obj->GetBoundingBox().min;
    //endP = obj->GetBoundingBox().max;

    startP = Vector3f(-0.3, -4.4, 0.13);
    endP = Vector3f(-0.3, 4.4, 0.13);

    auto temStart = startP;
    auto temEnd = endP;
    ui->startX->setText(QString::number(temStart[0]));
    ui->startY->setText(QString::number(temStart[1]));
    ui->startZ->setText(QString::number(temStart[2]));
    ui->endX->setText(QString::number(temEnd[0]));
    ui->endY->setText(QString::number(temEnd[1]));
    ui->endZ->setText(QString::number(temEnd[2]));
    iGame::AttributeSet* _AttributeSet;
    if (obj->HasSubDataObject()) {
        auto it = obj->SubDataObjectIteratorBegin();
        // it++;
        _AttributeSet = it->second->GetAttributeSet();
    } else {
        _AttributeSet = obj->GetAttributeSet();
    }
    if (!_AttributeSet) return;
    auto allAttributes = _AttributeSet->GetAllAttributes();
    if (!allAttributes) return;

    for (int i = 0; i < allAttributes->GetNumberOfElements(); i++) {
        auto attribute = allAttributes->GetElement(i);
        // if (attribute.type == IG_VECTOR&&attribute.attachmentType == IG_POINT) {
        if (attribute.type == IG_VECTOR) {
            if (attribute.pointer) { ui->comboBox->addItem(QString::fromStdString(attribute.pointer->GetName())); }
        }
    }

    ensureStreamBase();
    iGame::StreamTracer* streamtracer = m_StreamBase->streamFilter;
    // 当前 model 的 DataObject 与上次绑定不同 / 上次绑定失败（mesh 为空），都强制重新绑定
    if (modelBound) {
        if (!streamtracer->GetMesh() || m_DataObject.GetPointer() != currentModel->GetDataObject().GetPointer()) {
            modelBound = false;
        }
    }
    if (!modelBound) {
        std::cout << "[StreamTracer] First model binding\n";

        streamtracer->initStreamTracer(currentModel);
        masterName = currentModel->GetDataObject()->GetName();
        ui->source->setText(QString::fromStdString("Source: " + masterName));
        ui->source->show();
        auto tem = currentModel->GetDataObject();
        m_DataObject = tem;

        modelBound = true;
    } else {
    }
}
void igQtStreamTracerWidget::changeVecName() {
    vectorName = ui->comboBox->currentText().toStdString();
    std::cout << "current value=" << vectorName << std::endl;
}
void igQtStreamTracerWidget::generateStreamline() {

    auto scene = SceneManager::Instance()->GetCurrentScene();
    if (!scene) return;
    ensureStreamBase();
    iGame::StreamTracer* streamtracer = m_StreamBase->streamFilter;
    Model::Pointer model = pickSourceModel();
    if (!model) {
        QMessageBox::warning(this, tr("Stream Tracer"), tr("场景中没有可用于流线追踪的模型。"));
        return;
    }
    // 输入校验放在最前面，且与种子模式无关：任何模式拿到非 3D 体网格都给同一个提示。
    // 放在 initStreamTracer 之前还能省掉一次注定失败的 TransferToVolumeMesh / 邻接构建。
    if (!warnUnsupportedSource(model)) return;
    // 当前 model 的 DataObject 与上次绑定不同 / 上次绑定失败（mesh 为空），都强制重新绑定
    if (modelBound) {
        if (!streamtracer->GetMesh() || m_DataObject.GetPointer() != model->GetDataObject().GetPointer()) {
            modelBound = false;
        }
    }
    bool justRebound = false;
    if (!modelBound) {
        std::cout << "[StreamTracer] First model binding\n";
        streamtracer->initStreamTracer(model);
        masterName = model->GetDataObject()->GetName();
        ui->source->setText(QString::fromStdString("Source: " + masterName));
        ui->source->show();
        auto tem = model->GetDataObject();
        m_DataObject = tem;
        modelBound = true;
        justRebound = true;
        // 切到新源模型 → 不再复用旧流线对象，让新流线作为独立对象添加到 scene，
        // 这样上一个模型的流线显示得以保留。
        m_ResultObject = nullptr;
        haveDraw = false;
        std::cout << "[StreamTracer] new source model -> create fresh result object\n";
    }
    // 切换了源模型 → 用新 mesh 的包围盒重置 startP/endP（避免沿用旧模型范围）
    if (justRebound && streamtracer->GetMesh()) {
        auto bbox = streamtracer->GetMesh()->GetBoundingBox();
        // 沿 X 方向、Y/Z 取中心，并向内 5% 收缩，避免落在 mesh 边界外
        float dx = bbox.max[0] - bbox.min[0];
        float midY = (bbox.min[1] + bbox.max[1]) * 0.5f;
        float midZ = (bbox.min[2] + bbox.max[2]) * 0.5f;
        startP = Vector3f(bbox.min[0] + 0.05f * dx, midY, midZ);
        endP   = Vector3f(bbox.max[0] - 0.05f * dx, midY, midZ);
        std::cout << "[StreamTracer] bbox: min(" << bbox.min[0] << "," << bbox.min[1] << "," << bbox.min[2]
                  << ") max(" << bbox.max[0] << "," << bbox.max[1] << "," << bbox.max[2] << ")\n";
        std::cout << "[StreamTracer] new startP=(" << startP[0] << "," << startP[1] << "," << startP[2]
                  << ") endP=(" << endP[0] << "," << endP[1] << "," << endP[2] << ")\n";
        ui->startX->setText(QString::number(startP[0]));
        ui->startY->setText(QString::number(startP[1]));
        ui->startZ->setText(QString::number(startP[2]));
        ui->endX->setText(QString::number(endP[0]));
        ui->endY->setText(QString::number(endP[1]));
        ui->endZ->setText(QString::number(endP[2]));
        if (Selection) {
            Selection->Start = startP;
            Selection->End = endP;
        }
        std::cout << "[StreamTracer] reset startP/endP for new model bbox" << std::endl;
    }
    // 切换了源模型 → 重新填充矢量名列表，避免沿用旧模型不存在的 vectorName
    if (justRebound) {
        QSignalBlocker block(ui->comboBox);
        ui->comboBox->clear();
        iGame::AttributeSet* attrSet = nullptr;
        auto obj = m_DataObject;
        if (obj->HasSubDataObject()) {
            auto it = obj->SubDataObjectIteratorBegin();
            attrSet = it->second->GetAttributeSet();
        } else {
            attrSet = obj->GetAttributeSet();
        }
        if (attrSet) {
            auto all = attrSet->GetAllAttributes();
            if (all) {
                for (int i = 0; i < all->GetNumberOfElements(); ++i) {
                    auto a = all->GetElement(i);
                    if (a.type == IG_VECTOR && a.pointer) {
                        ui->comboBox->addItem(QString::fromStdString(a.pointer->GetName()));
                    }
                }
            }
        }
        if (ui->comboBox->count() > 0) {
            ui->comboBox->setCurrentIndex(0);
            vectorName = ui->comboBox->currentText().toStdString();
            std::cout << "[StreamTracer] auto-pick vector for new model: " << vectorName << std::endl;
        } else {
            vectorName.clear();
            QMessageBox::warning(this, tr("Stream Tracer"),
                                 tr("新模型中未找到矢量属性，无法生成流线。"));
            return;
        }
    }
    std::vector<std::vector<int>> seedPids = {{1797284, 3468659},
                                              {536542, 2738820},
                                              {536542, 2658742},
                                              {5485895, 536542}};

    std::vector<std::vector<float>> streamlineColor;
    std::vector<std::vector<float>> streamline;
    iGame::AttributeSet* _AttributeSet;
    auto tem = m_DataObject;
    if (tem->HasSubDataObject()) {
        auto it = tem->SubDataObjectIteratorBegin();
        // it++;
        _AttributeSet = it->second->GetAttributeSet();
    } else {
        _AttributeSet = tem->GetAttributeSet();
    }
    auto allAttributes = _AttributeSet ? _AttributeSet->GetAllAttributes() : nullptr;
    if (!allAttributes) {
        QMessageBox::warning(this, tr("Stream Tracer"), tr("当前模型没有属性数据，无法生成流线。"));
        return;
    }

    std::cout << vectorName << std::endl;
    std::vector<Vector3f> seeds;
    // 兜底：warnUnsupportedSource 已在函数开头挡掉非 3D 输入，这里覆盖 initStreamTracer
    // 自身失败（如多面体邻接构建异常）导致 mesh 为空的情况，同样对所有模式生效。
    if (!streamtracer->GetMesh() || streamtracer->GetMesh()->GetNumberOfVolumes() <= 0) {
        QMessageBox::warning(this, tr("Stream Tracer"),
                             tr("初始化流场失败：未能从当前模型得到 3D 体网格。"
                                "现版本的流线追踪只支持 3D 体网格。已取消生成。"));
        return;
    }
    if (control == 0) {
        seeds = streamtracer->seedPCoordGenerate(numOfSeeds, startP, endP);
    } else if (control == 1) {
        auto Smodel = streamtracer->GetModel();
        if (!Smodel) { Smodel = model; }
        Q_EMIT SetUseBox(Smodel);
        emit SetSelectItemShow(false);
        seeds = streamtracer->getModelSelectMax(vectorName, numOfSeeds);
        model->GetSelection()->ClearSelections();
        //seeds = streamtracer->getModelSelect();
    } else if (control == 2) {
        auto Smodel = streamtracer->GetModel();
        if (!Smodel) { Smodel = model; }
        Q_EMIT SetUseBox(Smodel);
        emit SetSelectItemShow(false);
        seeds = streamtracer->getModelSelectMin(vectorName, numOfSeeds);
        model->GetSelection()->ClearSelections();
        //seeds = streamtracer->getModelSelect();
    } else if (control == 3) {
        auto Smodel = streamtracer->GetModel();
        if (!Smodel) { Smodel = model; }
        Q_EMIT SetUseBox(Smodel);
        emit SetSelectItemShow(false);
        //seeds = streamtracer->getModelSelect();
        seeds = streamtracer->getModelSelectMax(vectorName, numOfSeeds);
        model->GetSelection()->ClearSelections();
        auto temSeeds = streamtracer->seedPCoordGenerate(numOfSeeds, startP, endP);
        //auto temSeeds = streamtracer->getModelSelectMax(vectorName, numOfSeeds);
        for (auto seed: temSeeds) { seeds.emplace_back(seed); }
    } else if (control == 4) {
        auto Smodel = streamtracer->GetModel();
        if (!Smodel) { Smodel = model; }
        Q_EMIT SetUseBox(model);
        emit SetSelectItemShow(false);
        //seeds = streamtracer->getModelSelect();
        seeds = streamtracer->getModelSelectMin(vectorName, numOfSeeds);
        model->GetSelection()->ClearSelections();
        auto temSeeds = streamtracer->seedPCoordGenerate(numOfSeeds, startP, endP);
        //auto temSeeds = streamtracer->getModelSelectMax(vectorName, numOfSeeds);
        for (auto seed: temSeeds) { seeds.emplace_back(seed); }
    } else if (control == 5) {
        auto Smodel = streamtracer->GetModel();
        if (!Smodel) { Smodel = model; }
        seeds = streamtracer->computeSubBlockCenters(streamtracer->GetMesh()->GetBoundingBox().min,
                                                     streamtracer->GetMesh()->GetBoundingBox().max, splitX, splitY,
                                                     splitZ);
    } else if (control == 6) {
        // 熵驱动：若当前有选区，则把熵排序限制在选区内（topPercent 变成选区内相对排名），
        // 保证用户框选的区域一定能取到代表性种子；无选区时退回全局行为。
        auto Smodel = streamtracer->GetModel();
        if (!Smodel) { Smodel = model; }
        Q_EMIT SetUseBox(Smodel);
        emit SetSelectItemShow(false);

        auto& selPts = model->GetSelection()->GetSelectedItems(IG_POINT);
        auto& selCells = model->GetSelection()->GetSelectedItems(IG_CELL);
        const bool hasSelection = !selPts.empty() || !selCells.empty();

        seeds = streamtracer->getEntropySeeding(vectorName, 0.025f, 8, hasSelection);

        // 均匀补充种子：有选区时限制在选区包围盒内，避免把全局均匀点混进来稀释选区结果
        Vector3f uniMin = streamtracer->GetMesh()->GetBoundingBox().min;
        Vector3f uniMax = streamtracer->GetMesh()->GetBoundingBox().max;
        if (hasSelection && !seeds.empty()) {
            uniMin = seeds[0];
            uniMax = seeds[0];
            for (const auto& s: seeds) {
                for (int d = 0; d < 3; ++d) {
                    uniMin[d] = std::min(uniMin[d], s[d]);
                    uniMax[d] = std::max(uniMax[d], s[d]);
                }
            }
            std::cout << "[StreamTracer] entropy seeds bbox: (" << uniMin[0] << "," << uniMin[1] << "," << uniMin[2]
                      << ") - (" << uniMax[0] << "," << uniMax[1] << "," << uniMax[2] << ")\n";
        }
        auto uniformSeeds = streamtracer->computeSubBlockCenters(uniMin, uniMax, splitX, splitY, splitZ);
        seeds.insert(seeds.end(), uniformSeeds.begin(), uniformSeeds.end());

        if (hasSelection) { model->GetSelection()->ClearSelections(); }
    }
    // 种子为空时回退：用新 mesh 包围盒生成线段种子，避免静默不出图
    if (seeds.empty()) {
        std::cout << "[StreamTracer] seeds empty (no selection?) -> fallback to line seeds on bbox"
                  << std::endl;
        seeds = streamtracer->seedPCoordGenerate(numOfSeeds, startP, endP);
        if (seeds.empty()) {
            QMessageBox::warning(this, tr("Stream Tracer"),
                                 tr("无法生成种子点。请在新模型上重新框选区域，或切换到\"线段 / 子块中心\"模式。"));
            return;
        }
    }
    std::cout << "[StreamTracer] seeds count = " << seeds.size() << std::endl;
    if (!seeds.empty()) {
        std::cout << "[StreamTracer] seed[0]=(" << seeds[0][0] << "," << seeds[0][1] << "," << seeds[0][2]
                  << "), seed[last]=(" << seeds.back()[0] << "," << seeds.back()[1] << "," << seeds.back()[2]
                  << ")\n";
    }
    streamtracer->SetInput(seeds, vectorName, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
    streamtracer->Execute();
    if (auto out = streamtracer->GetOutput()) {
        std::cout << "[StreamTracer] result points=" << out->GetNumberOfPoints()
                  << ", cells=" << out->GetNumberOfCells() << std::endl;
    } else {
        std::cout << "[StreamTracer] result is null" << std::endl;
    }
    // 每次点击都产出一个独立的流线对象，保留历史结果以便用户对比/回看。
    auto resObj = streamtracer->GetOutput();
    auto newResult = iGame::UnstructuredMesh::New();
    newResult->AddObserver(iGame::Command::DeleteEvent, [this, weakRef = newResult.GetPointer()]() -> void {
        // 无论是不是当前结果，都要清掉简化原始缓存，避免悬空键
        m_OriginalCache.erase(weakRef);
        // 仅当被删的恰好是"最近一次结果"时，清理与之相关的状态。
        // 这样删历史结果不会影响当前正在用的会话。
        if (m_ResultObject.GetPointer() == weakRef) {
            if (m_StreamBase && m_StreamBase->streamFilter) { m_StreamBase->streamFilter->meshId = -1; }
            modelBound = false;
            haveDraw = false;
            isExisted = false;
            m_ResultObject = nullptr;
            if (auto scene = SceneManager::Instance()->GetCurrentScene()) {
                if (auto interactor = scene->GetInteractor()) { interactor->RequestBasicStyle(); }
            }
        }
    });
    if (resObj) {
        newResult->SetPoints(resObj->GetPoints());
        newResult->SetCells(resObj->GetCells(), resObj->GetCellTypes());
        newResult->SetAttributeSet(resObj->GetAttributeSet());
        newResult->SetShellRenderingOption(false);
        newResult->ViewCloudPicture(scene, 0);
        newResult->SetLineWidth(widthOfStreamLine);
    } else {
        newResult->SetPoints(iGame::Points::New());
        newResult->SetCells(iGame::CellArray::New(), iGame::UnsignedIntArray::New());
        newResult->SetAttributeSet(iGame::AttributeSet::New());
        newResult->SetShellRenderingOption(false);
        newResult->SetAttributeIndex(-1);
        newResult->SetLineWidth(widthOfStreamLine);
    }

    // 命名带序号 + control 模式，便于在模型树里区分
    ++m_StreamlineCounter;
    std::string ctrlTag;
    switch (control) {
        case 0: ctrlTag = "Line"; break;
        case 1: ctrlTag = "SelMax"; break;
        case 2: ctrlTag = "SelMin"; break;
        case 3: ctrlTag = "SelMax+Line"; break;
        case 4: ctrlTag = "SelMin+Line"; break;
        case 5: ctrlTag = "Uniform"; break;
        case 6: ctrlTag = "Entropy"; break;
        default: ctrlTag = "Mode" + std::to_string(control); break;
    }
    newResult->DataObject::SetName(masterName + "_StreamLine_" + std::to_string(m_StreamlineCounter) + "_" + ctrlTag);
    Q_EMIT AddStreamObject(newResult);

    bool wasFirstDraw = !haveDraw;
    haveDraw = true;
    m_ResultObject = newResult;
    if (wasFirstDraw) scene->GetCurrentModel()->SetViewPointsSwitch(true);
    // scene->SetCurrentModel(1);

    if (isExisted == false) {
        isExisted = true;
        Selection = StreamLineSelection::New();
        Painter = scene->GetCurrentModel()->GetPainter3D();
        Selection->Start = startP;
        Selection->End = endP;
        Selection->SetSelectionCallBackEvent(
                [&](IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope) {
                    if (itemType == IG_CHANGE) {
                        startP = Selection->Start;
                        endP = Selection->End;
                        auto temStart = startP;
                        auto temEnd = endP;
                        ui->startX->setText(QString::number(temStart[0]));
                        ui->startY->setText(QString::number(temStart[1]));
                        ui->startZ->setText(QString::number(temStart[2]));
                        ui->endX->setText(QString::number(temEnd[0]));
                        ui->endY->setText(QString::number(temEnd[1]));
                        ui->endZ->setText(QString::number(temEnd[2]));
                    }
                },
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

        scene->GetInteractor()->SetDataObject(m_DataObject);
        scene->GetInteractor()->SetPainter3D(Painter);
        scene->GetInteractor()->RequestStreamLineStyle(Selection);
    }
    m_OriginalStream = iGame::UnstructuredMesh::New();
    m_OriginalStream->DeepCopy(m_ResultObject);
}
void igQtStreamTracerWidget::Simplifier() {
    if (!m_StreamBase || !m_StreamBase->streamFilter) return;

    auto scene = SceneManager::Instance()->GetCurrentScene();
    if (!scene) return;

    // 以"当前在场景里选中的流线对象"为简化目标。
    // 必须是 UnstructuredMesh，并且名字里带 "_StreamLine"（避开把源体网格当成流线简化）。
    auto curModel = scene->GetCurrentModel();
    iGame::UnstructuredMesh::Pointer target = nullptr;
    if (curModel) {
        auto obj = curModel->GetDataObject();
        if (obj) {
            auto asUM = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
            if (asUM && obj->GetName().find("_StreamLine") != std::string::npos) {
                target = asUM;
            }
        }
    }
    if (!target) {
        QMessageBox::warning(this, tr("Stream Tracer"),
                             tr("请先在模型树中选中要简化的流线对象（名称含 _StreamLine）。"));
        return;
    }

    // 取该 target 对应的"原始未简化"版本。
    // 若是第一次简化，从 target 当前的 points/cells/attr 抓一个快照（共享底层数据指针）；
    // 之后每次简化都从这个快照出发，target 自身被简化结果覆盖也不影响。
    // 注意：UnstructuredMesh::DeepCopy 实际是基类空实现，所以不能用它来克隆。
    iGame::UnstructuredMesh::Pointer originalForSimp;
    auto cacheIt = m_OriginalCache.find(target.GetPointer());
    if (cacheIt != m_OriginalCache.end()) {
        originalForSimp = cacheIt->second;
    } else {
        originalForSimp = iGame::UnstructuredMesh::New();
        originalForSimp->SetPoints(target->GetPoints());
        originalForSimp->SetCells(target->GetCells(), target->GetCellTypes());
        originalForSimp->SetAttributeSet(target->GetAttributeSet());
        m_OriginalCache[target.GetPointer()] = originalForSimp;
        std::cout << "[Simplifier] cached original snapshot for target " << target.GetPointer() << std::endl;
    }

    auto simp = iGame::StreamlineSimplifier::New();
    simp->SetInput(originalForSimp);
    simp->SetCurvBins(40);
    simp->SetNumClusters(ui->clusterSpin->value());
    simp->SetTotalTarget(ui->perClusterSpin->value());

    if (!simp->Execute()) return;
    auto out = simp->GetOutput();
    if (!out) return;

    target->SetPoints(out->GetPoints());
    target->SetCells(out->GetCells(), out->GetCellTypes());
    target->SetAttributeSet(out->GetAttributeSet());
    target->SetShellRenderingOption(false);
    target->SetLineWidth(widthOfStreamLine);

    // 用 ClusterLabel 着色
    auto outAttr = out->GetAttributeSet();
    int clusterIdx = outAttr ? outAttr->GetAttributeIndex("ClusterLabel") : -1;
    if (clusterIdx >= 0) {
        target->ViewCloudPicture(scene, clusterIdx);
    } else {
        target->ViewCloudPicture(scene, 0);
    }

    target->ConvertToDrawableData();
    Q_EMIT UpdateStreamObject(target);
}