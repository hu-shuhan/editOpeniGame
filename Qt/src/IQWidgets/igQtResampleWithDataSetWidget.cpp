#include "IQWidgets/igQtResampleWithDataSetWidget.h"

#include "IQComponents/igQtModelDialogWidget.h"
#include "ResampleWithDataset/iGameResampleWithDataSet.h"
#include "iGameModel.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"
#include "iGameType.h"

#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QVariant>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

igQtResampleWithDataSet::igQtResampleWithDataSet(igQtModelDialogWidget* modelTreeWidget, QWidget* parent)
    : QWidget(parent), ui(new Ui::ResampleWithDataSetWidget) {
    ui->setupUi(this);

    // 主窗口会传入真正的模型树控件；为空时再从顶层窗口对象树中查找
    m_ModelTreeWidget = (modelTreeWidget != nullptr) ? modelTreeWidget : ResolveModelTreeWidget();

    // 容差允许小数：留空或 0 表示自动（被采样网格包围盒对角线 × 1e-6）
    const QRegularExpression rxFloat("-?\\d*\\.?\\d+");
    ui->lineEdit_Tolerance->setValidator(new QRegularExpressionValidator(rxFloat, this));
    ui->lineEdit_Tolerance->setPlaceholderText(QStringLiteral("自动"));
    // 吸附半径（SnappingRadius）不在面板上暴露：filter 保留 SetSnappingRadius() 接口，
    // 默认关闭（m_SnappingRadius = -1），需要时由调用方在代码里开启

    connect(ui->pushButton_Refresh, &QPushButton::clicked, this, &igQtResampleWithDataSet::RefreshModelList);
    connect(ui->pushButton_Execute, &QPushButton::clicked, this, &igQtResampleWithDataSet::Execute);

    // 选择变化时重新绑定输入观察者（用于「输入模型被删除 → 清空对应下拉框」）
    connect(ui->comboBox_Source, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { AttachInputObserver(0, ResolveDataObject(0)); });
    connect(ui->comboBox_Probe, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { AttachInputObserver(1, ResolveDataObject(1)); });

    RefreshModelList();
}

igQtResampleWithDataSet::~igQtResampleWithDataSet() {
    DetachInputObserver(0);
    DetachInputObserver(1);
}

void igQtResampleWithDataSet::SetModelTreeWidget(igQtModelDialogWidget* modelTreeWidget) {
    if (modelTreeWidget != nullptr) { m_ModelTreeWidget = modelTreeWidget; }
}

void igQtResampleWithDataSet::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // 每次显示都重新枚举一次模型，避免模型树变化后下拉框过期
    RefreshModelList();
}

igQtModelDialogWidget* igQtResampleWithDataSet::ResolveModelTreeWidget() {
    if (m_ModelTreeWidget != nullptr) { return m_ModelTreeWidget; }

    // 1) 优先在自己的顶层窗口里查找
    QWidget* top = this->window();
    if (top != nullptr) {
        if (auto* widget = top->findChild<igQtModelDialogWidget*>()) { return widget; }
    }
    // 2) 回退：遍历应用程序的所有顶层窗口（浮动 Dock 等情况）
    const QWidgetList topLevels = QApplication::topLevelWidgets();
    for (QWidget* level : topLevels) {
        if (level == nullptr) { continue; }
        if (auto* widget = level->findChild<igQtModelDialogWidget*>()) { return widget; }
    }
    return nullptr;
}

/* ------------------------------------------------------------------ */
/* 模型列表与输入选择                                                  */
/* ------------------------------------------------------------------ */
void igQtResampleWithDataSet::RefreshModelList() {
    // 记住当前选择（模型 id，0 = 未选择），刷新后尽量保持不变
    const IGuint oldSource = ui->comboBox_Source->currentData().toUInt();
    const IGuint oldProbe = ui->comboBox_Probe->currentData().toUInt();

    if (m_ModelTreeWidget == nullptr) { m_ModelTreeWidget = ResolveModelTreeWidget(); }

    QSignalBlocker blockSource(ui->comboBox_Source);
    QSignalBlocker blockProbe(ui->comboBox_Probe);
    ui->comboBox_Source->clear();
    ui->comboBox_Probe->clear();

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

    // 只列「模型树里存在的模型」：场景模型池里还有中心坐标轴这类内部模型，
    // 它们不进模型树、名字为空，不能作为采样输入出现在下拉框里。
    if (scene != nullptr) {
        for (auto it = scene->GetModelList()->Begin(); it != scene->GetModelList()->End(); ++it) {
            const auto& entry = *it;
            auto model = entry.second;
            if (model == nullptr) { continue; }
            // 模型 id 统一从模型池取（模型树项不一定记录 id）
            if (!m_ModelTreeWidget || !m_ModelTreeWidget->getItemFromObject(model->GetDataObject())) { continue; }

            auto dataObject = model->GetDataObject();
            if (dataObject == nullptr) { continue; }

            const IGuint id = entry.first;
            QString name = QString::fromStdString(dataObject->GetName()).trimmed();
            if (name.isEmpty()) { name = QStringLiteral("未命名模型"); }
            const QString text = QStringLiteral("%1 · %2").arg(id-1).arg(name);
            const QVariant itemData = QVariant::fromValue<unsigned int>(id);
            ui->comboBox_Source->addItem(text, itemData);
            ui->comboBox_Probe->addItem(text, itemData);
        }
    }

    // 默认值：采样点网格 = 当前模型（若仍存在），被采样网格 = 列表里另一个模型
    IGuint probeWanted = oldProbe;
    if (probeWanted == 0 && scene != nullptr) { probeWanted = scene->GetCurrentModelID(); }

    IGuint sourceWanted = oldSource;
    if (sourceWanted == 0) {
        for (int i = 0; i < ui->comboBox_Source->count(); ++i) {
            const IGuint id = ui->comboBox_Source->itemData(i).toUInt();
            if (id != probeWanted) {
                sourceWanted = id;
                break;
            }
        }
    }

    const auto selectById = [](QComboBox* combo, IGuint wanted) {
        if (wanted == 0) {
            combo->setCurrentIndex(-1);
            return;
        }
        // 模型已被删除时 findData 返回 -1，即自动回到「未选择」
        combo->setCurrentIndex(combo->findData(QVariant::fromValue<unsigned int>(wanted)));
    };
    selectById(ui->comboBox_Source, sourceWanted);
    selectById(ui->comboBox_Probe, probeWanted);

    // 信号被 blocker 抑制了，这里手动同步一次绑定
    AttachInputObserver(0, ResolveDataObject(0));
    AttachInputObserver(1, ResolveDataObject(1));
}

void igQtResampleWithDataSet::BindCurrentModel() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    const IGuint currentId = (scene != nullptr) ? scene->GetCurrentModelID() : 0;

    RefreshModelList();
    if (currentId == 0) { return; }

    // 「采样点网格」默认取当前模型
    const int probeIndex = ui->comboBox_Probe->findData(QVariant::fromValue<unsigned int>(currentId));
    if (probeIndex < 0) { return; }
    {
        QSignalBlocker blocker(ui->comboBox_Probe);
        ui->comboBox_Probe->setCurrentIndex(probeIndex);
    }
    AttachInputObserver(1, ResolveDataObject(1));

    // 两个输入不能是同一个模型：被采样网格若撞上了，自动换一个
    if (ui->comboBox_Source->currentData().toUInt() != currentId) { return; }
    for (int i = 0; i < ui->comboBox_Source->count(); ++i) {
        if (ui->comboBox_Source->itemData(i).toUInt() == currentId) { continue; }
        {
            QSignalBlocker blocker(ui->comboBox_Source);
            ui->comboBox_Source->setCurrentIndex(i);
        }
        AttachInputObserver(0, ResolveDataObject(0));
        break;
    }
}

iGame::Model* igQtResampleWithDataSet::ResolveModel(int which) const {
    QComboBox* combo = (which == 0) ? ui->comboBox_Source : ui->comboBox_Probe;
    const QVariant data = combo->currentData();
    if (!data.isValid()) { return nullptr; }
    const IGuint id = data.toUInt();
    if (id == 0) { return nullptr; }

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene == nullptr) { return nullptr; }
    // 场景的模型池持有引用，这里返回的裸指针在本函数返回后仍然有效
    return scene->GetModelById(static_cast<int>(id)).GetPointer();
}

iGame::DataObject::Pointer igQtResampleWithDataSet::ResolveDataObject(int which) const {
    auto model = ResolveModel(which);
    if (model == nullptr) { return nullptr; }
    return model->GetDataObject();
}

/* ------------------------------------------------------------------ */
/* 输入观察者：模型被删除时清空对应下拉框                              */
/* ------------------------------------------------------------------ */
void igQtResampleWithDataSet::DetachInputObserver(int which) {
    auto& object = (which == 0) ? m_SourceDataObject : m_ProbeDataObject;
    auto& tag = (which == 0) ? m_SourceObserverTag : m_ProbeObserverTag;

    // tag 只对注册它的那个对象有效，必须先摘掉再换绑
    if (object && tag) { object->RemoveObserver(tag); }
    tag = 0;
    object = nullptr;
}

void igQtResampleWithDataSet::AttachInputObserver(int which, iGame::DataObject::Pointer object) {
    DetachInputObserver(which);
    if (object == nullptr) { return; }

    auto& stored = (which == 0) ? m_SourceDataObject : m_ProbeDataObject;
    auto& tag = (which == 0) ? m_SourceObserverTag : m_ProbeObserverTag;
    stored = object;

    // 场景/模型树移除该模型时会 InvokeEvent（此刻对象仍然存活）：
    // 只清空对应的下拉框，面板保持打开，用户可以重新选择输入
    tag = object->AddObserver(iGame::Command::DeleteEvent, [this, which]() -> void {
        ClearInputSelection(which);
    });
}

void igQtResampleWithDataSet::ClearInputSelection(int which) {
    // 观察者随对象一起销毁，tag 不能再用了
    if (which == 0) {
        m_SourceObserverTag = 0;
        m_SourceDataObject = nullptr;
    } else {
        m_ProbeObserverTag = 0;
        m_ProbeDataObject = nullptr;
    }

    QComboBox* combo = (which == 0) ? ui->comboBox_Source : ui->comboBox_Probe;
    if (combo->count() > 0) {
        // blocker 防止 currentIndexChanged 再次触发绑定
        QSignalBlocker blocker(combo);
        combo->setCurrentIndex(-1);
    }
    ui->label_Message->setText(which == 0 ? QStringLiteral("「被采样网格」已被删除，请重新选择。")
                                          : QStringLiteral("「采样点网格」已被删除，请重新选择。"));
}

/* ------------------------------------------------------------------ */
/* 执行                                                                */
/* ------------------------------------------------------------------ */
std::string igQtResampleWithDataSet::MakeUniqueResultName(const std::string& base) const {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene == nullptr) { return base; }

    std::set<std::string> existing;
    for (auto it = scene->GetModelList()->Begin(); it != scene->GetModelList()->End(); ++it) {
            const auto& entry = *it;
        if (entry.second == nullptr) { continue; }
        auto dataObject = entry.second->GetDataObject();
        if (dataObject == nullptr) { continue; }
        existing.insert(dataObject->GetName());
    }

    if (existing.find(base) == existing.end()) { return base; }
    for (int i = 1; i < 100000; ++i) {
        const std::string candidate = base + "_" + std::to_string(i);
        if (existing.find(candidate) == existing.end()) { return candidate; }
    }
    return base;
}

void igQtResampleWithDataSet::Execute() {
    auto source = ResolveDataObject(0);
    auto probe = ResolveDataObject(1);

    if (source == nullptr || probe == nullptr) {
        QMessageBox::warning(this, QStringLiteral("重采样至数据集"),
                             QStringLiteral("请先选择「被采样网格」与「采样点网格」。"));
        return;
    }
    if (source.GetPointer() == probe.GetPointer()) {
        QMessageBox::warning(this, QStringLiteral("重采样至数据集"),
                             QStringLiteral("「被采样网格」与「采样点网格」不能是同一个模型。"));
        return;
    }

    iGame::ResampleWithDataSet::Pointer filter = iGame::ResampleWithDataSet::New();
    filter->SetSourceData(source);
    filter->SetProbeData(probe);

    bool ok = false;
    const double tolerance = ui->lineEdit_Tolerance->text().toDouble(&ok);
    filter->SetTolerance((ok && tolerance > 0.0) ? tolerance : 0.0);
    // 吸附半径保持 filter 默认值（关闭）：面板不暴露该参数，接口仍保留在 filter 上

    //此处是加入显示执行后信息的功能
    if (!filter->Execute()) {
        const QString message = QString::fromStdString(filter->GetMessage());
        // ui->label_Message->setText(message);
        QMessageBox::warning(this, QStringLiteral("重采样至数据集"), message);
        return;
    }

    auto output = filter->GetResampledData();
    if (output == nullptr) {
        // ui->label_Message->setText(QStringLiteral("没有生成结果。"));
        QMessageBox::warning(this, QStringLiteral("重采样至数据集"), "没有生成结果");
        return;
    }

    // 结果 = 采样点网格的深拷贝，名字用「<采样点网格名>_resample」（重名加序号）
    output->SetName(MakeUniqueResultName(probe->GetName() + "_resample"));

    if (m_ModelTreeWidget == nullptr) { m_ModelTreeWidget = ResolveModelTreeWidget(); }
    if (m_ModelTreeWidget != nullptr) {
        m_ModelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);
    }

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene != nullptr) { scene->Update(); }

    // ui->label_Message->setText(QString::fromStdString(filter->GetMessage()));
}
