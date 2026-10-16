#include "IQWidgets/igQtLinearExtrusionWidget.h"
#include "iGameFilterIncludes.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
// 线性拉伸 filter 位于 Filters/Modeling，尚未收录进 iGameFilterIncludes.h，需要按族目录单独包含
#include <Modeling/iGameLinearExtrusionFilter.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QPointer>
#include <set>

namespace {
// 面板「拉伸模式」下拉的条目顺序（与 LinearExtrusionWidget.ui 中的条目顺序一一对应）
constexpr int kModeVector = 0;
constexpr int kModeNormal = 1;
constexpr int kModePoint = 2;

// 面板下拉索引 → filter 的 ExtrusionType：filter 的枚举值从 1 开始，和索引不是同一个数值，
// 所以必须显式映射，不能直接把索引当枚举用
iGame::LinearExtrusionFilter::ExtrusionType ExtrusionTypeFromIndex(int index) {
    switch (index) {
        case kModeVector: return iGame::LinearExtrusionFilter::ExtrusionType::Vector;
        case kModePoint: return iGame::LinearExtrusionFilter::ExtrusionType::Point;
        case kModeNormal:
        default: return iGame::LinearExtrusionFilter::ExtrusionType::Normal;
    }
}
} // namespace

igQtLinearExtrusionWidget::igQtLinearExtrusionWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::LinearExtrusionWidget) {
    ui->setupUi(this);
    // 默认沿点法向（与 filter 的默认值一致）
    ui->comboBox_ExtrusionType->setCurrentIndex(kModeNormal);
    // 模式决定几何语义，滚轮误触会静默换模式（面板上只有成组变灰这一处提示），
    // 因此这里拦掉下拉框的滚轮事件，模式只能通过点开下拉框切换
    ui->comboBox_ExtrusionType->installEventFilter(this);
    connect(ui->comboBox_ExtrusionType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        UpdateParameterState();
    });
    connect(ui->btnApply, &QPushButton::clicked, this, &igQtLinearExtrusionWidget::Apply);
    UpdateParameterState();
}

// 拉伸模式下拉的滚轮拦截：吞掉滚轮，避免误改模式后参数被静默禁用
bool igQtLinearExtrusionWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == ui->comboBox_ExtrusionType && event->type() == QEvent::Wheel) {
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void igQtLinearExtrusionWidget::SetOriginDataObject(iGame::DataObject::Pointer data) {
    m_OriginDataObject = data;
    m_Generated = false;
    m_ResultDataObject = nullptr;
}

// 模式切换时按语义启用/禁用参数：禁用而不是清空，切回该模式仍保留用户上次填的数值。
// 整组禁用（连标题一起变灰）比逐个禁用输入框更能看出「这一组当前不参与拉伸」
void igQtLinearExtrusionWidget::UpdateParameterState() {
    const int index = ui->comboBox_ExtrusionType->currentIndex();
    ui->groupBox_Vector->setEnabled(index == kModeVector);
    ui->groupBox_Point->setEnabled(index == kModePoint);
    // 自动回退的提示只在 Normal 模式有意义：Vector/Point 模式不存在回退
    ui->label_NormalHint->setVisible(index == kModeNormal);
}

void igQtLinearExtrusionWidget::Apply() {
    if (m_OriginDataObject == nullptr) {
        Q_EMIT ApplyFailed(QStringLiteral("请先选择一个模型。"));
        return;
    }

    const int modeIndex = ui->comboBox_ExtrusionType->currentIndex();

    // 缩放系数三种模式都要参与（位移 = 缩放系数 × 偏移）：解析失败必须报出具体字段，
    // 静默按 0 处理会让用户拿到一个「没拉伸」的结果却查不出原因
    bool okScaleFactor = false;
    const double scaleFactor = ui->lineEdit_ScaleFactor->text().toDouble(&okScaleFactor);
    if (!okScaleFactor) {
        Q_EMIT ApplyFailed(QStringLiteral("缩放系数（距离系数）不是有效数字"));
        return;
    }

    // 方向向量只在 Vector 模式生效：其余模式下输入框是禁用的，里面残留的旧值既不参与拉伸也不再校验
    double vectorX = 0.0;
    double vectorY = 0.0;
    double vectorZ = 1.0;
    if (modeIndex == kModeVector) {
        bool okX = false;
        bool okY = false;
        bool okZ = false;
        vectorX = ui->lineEdit_VectorX->text().toDouble(&okX);
        vectorY = ui->lineEdit_VectorY->text().toDouble(&okY);
        vectorZ = ui->lineEdit_VectorZ->text().toDouble(&okZ);
        if (!okX) {
            Q_EMIT ApplyFailed(QStringLiteral("方向向量 X 不是有效数字"));
            return;
        }
        if (!okY) {
            Q_EMIT ApplyFailed(QStringLiteral("方向向量 Y 不是有效数字"));
            return;
        }
        if (!okZ) {
            Q_EMIT ApplyFailed(QStringLiteral("方向向量 Z 不是有效数字"));
            return;
        }
    }

    // 拉伸基点同理，只在 Point 模式生效
    double pointX = 0.0;
    double pointY = 0.0;
    double pointZ = 0.0;
    if (modeIndex == kModePoint) {
        bool okX = false;
        bool okY = false;
        bool okZ = false;
        pointX = ui->lineEdit_PointX->text().toDouble(&okX);
        pointY = ui->lineEdit_PointY->text().toDouble(&okY);
        pointZ = ui->lineEdit_PointZ->text().toDouble(&okZ);
        if (!okX) {
            Q_EMIT ApplyFailed(QStringLiteral("拉伸基点 X 不是有效数字"));
            return;
        }
        if (!okY) {
            Q_EMIT ApplyFailed(QStringLiteral("拉伸基点 Y 不是有效数字"));
            return;
        }
        if (!okZ) {
            Q_EMIT ApplyFailed(QStringLiteral("拉伸基点 Z 不是有效数字"));
            return;
        }
    }

    auto filter = iGame::LinearExtrusionFilter::New();
    filter->SetInput(m_OriginDataObject);
    filter->SetExtrusionType(ExtrusionTypeFromIndex(modeIndex));
    filter->SetScaleFactor(scaleFactor);
    // 只写当前模式用到的参数，避免把禁用输入框里的旧值写进 filter
    if (modeIndex == kModeVector) filter->SetVector(vectorX, vectorY, vectorZ);
    if (modeIndex == kModePoint) filter->SetExtrusionPoint(pointX, pointY, pointZ);
    filter->SetCapping(ui->checkBox_Capping->isChecked());
    if (!filter->Execute()) {
        // 失败原因（无输入/单元类型不支持/Normal 模式找不到点法向等）由 filter 写在 m_Message 里
        Q_EMIT ApplyFailed(QString::fromStdString(filter->GetMessage()));
        return;
    }
    auto result = filter->GetOutput();
    if (result == nullptr) {
        Q_EMIT ApplyFailed(QStringLiteral("线性拉伸执行失败"));
        return;
    }

    if (!m_Generated) {
        // 首次执行：按全局序列命名（输入名_LinearExtrusion_序号）并新增模型树节点
        result->SetName(UniqueResultName(m_OriginDataObject->GetName()));
        m_ResultDataObject = result;
        // 场景/模型树删除结果时重置：同一输入再次执行可重新生成节点
        // The scene can keep the result after this parameter panel is destroyed.
        const QPointer<igQtLinearExtrusionWidget> panel(this);
        m_ResultDataObject->AddObserver(iGame::Command::DeleteEvent, [panel]() -> void {
            if (panel) panel->m_Generated = false;
        });
        m_Generated = true;
        Q_EMIT DrawLinearExtrusionModel(m_ResultDataObject);
    } else {
        // 再次执行：修改上次 filter 结果（不生成新节点），更新同一结果对象内容
        RebuildResultObject(result);
        Q_EMIT UpdateLinearExtrusionModel(m_ResultDataObject);
    }
}

// 结果节点命名：输入名_LinearExtrusion_序号，序号 = 该 filter 类型在场景中的全局序列（与输入名无关），
// 复用最小空缺（删除后序号可复用）
std::string igQtLinearExtrusionWidget::UniqueResultName(const std::string& inputName) {
    std::set<int> used;
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene != nullptr) {
        auto modelList = scene->GetModelList();
        if (modelList != nullptr) {
            const std::string marker = "_LinearExtrusion_";
            for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
                if (it->second == nullptr || it->second->GetDataObject() == nullptr) continue;
                const std::string& name = it->second->GetDataObject()->GetName();
                const size_t pos = name.rfind(marker);
                if (pos == std::string::npos) continue;
                const std::string suffix = name.substr(pos + marker.size());
                if (suffix.empty() || suffix.find_first_not_of("0123456789") != std::string::npos) continue;
                used.insert(std::stoi(suffix));
            }
        }
    }
    int n = 1;
    while (used.count(n) != 0) ++n;
    return inputName + "_LinearExtrusion_" + std::to_string(n);
}

// 把新结果的内容（几何共享 + 属性集 + 名字）搬运到已挂载的结果对象上。
// 线性拉伸的输出固定是 UnstructuredMesh（见 filter 语义），Surface 分支保留为兜底
void igQtLinearExtrusionWidget::RebuildResultObject(iGame::DataObject::Pointer fresh) {
    auto src = iGame::DynamicCast<iGame::UnstructuredMesh>(fresh);
    auto dst = iGame::DynamicCast<iGame::UnstructuredMesh>(m_ResultDataObject);
    if (src != nullptr && dst != nullptr) {
        dst->SetPoints(src->GetPoints());
        dst->SetCells(src->GetCells(), iGame::UnsignedIntArray::Pointer(src->GetCellTypes()));
        dst->SetAttributeSet(src->GetAttributeSet());
        return;
    }
    auto ssrc = iGame::DynamicCast<iGame::SurfaceMesh>(fresh);
    auto sdst = iGame::DynamicCast<iGame::SurfaceMesh>(m_ResultDataObject);
    if (ssrc != nullptr && sdst != nullptr) {
        sdst->SetPoints(ssrc->GetPoints());
        sdst->SetFaces(ssrc->GetFaces());
        sdst->SetAttributeSet(ssrc->GetAttributeSet());
        return;
    }
    m_ResultDataObject = fresh;
}
