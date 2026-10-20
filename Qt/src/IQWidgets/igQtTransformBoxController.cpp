#include <IQWidgets/igQtTransformBoxController.h>

#include <QLineEdit>

#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <iGameBoundingBox.h>
#include <iGameInteractor.h>
#include <iGameSelectionParameter.h>
#include <Transformation/iGameTransformFilter.h>

#include <algorithm>
#include <cmath>

using namespace iGame;

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kMinScale = 1e-3;   // 缩放绝对值下限，避免变换矩阵奇异

/**
 * igm::mat4 按列优先存储（m[col][row]），这里统一转成数学约定 r[row][col]，
 */
void ToMath3x3(const igm::mat4& m, double r[3][3]) {
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            r[row][col] = static_cast<double>(m[col][row]);
        }
    }
}

igm::mat4 FromMath3x3(const double r[3][3]) {
    igm::mat4 out(1.0f);
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            out[col][row] = static_cast<float>(r[row][col]);
        }
    }
    return out;
}

/** R = Rz(rz) * Ry(ry) * Rx(rx)，与 TransformFilter::BuildMatrix 的旋转顺序一致 */
void ComposeEulerZYX(double rxDeg, double ryDeg, double rzDeg, double r[3][3]) {
    const double rx = rxDeg * kPi / 180.0;
    const double ry = ryDeg * kPi / 180.0;
    const double rz = rzDeg * kPi / 180.0;
    const double cx = std::cos(rx), sx = std::sin(rx);
    const double cy = std::cos(ry), sy = std::sin(ry);
    const double cz = std::cos(rz), sz = std::sin(rz);

    r[0][0] = cz * cy;
    r[0][1] = cz * sy * sx - sz * cx;
    r[0][2] = cz * sy * cx + sz * sx;
    r[1][0] = sz * cy;
    r[1][1] = sz * sy * sx + cz * cx;
    r[1][2] = sz * sy * cx - cz * sx;
    r[2][0] = -sy;
    r[2][1] = cy * sx;
    r[2][2] = cy * cx;
}

/** ComposeEulerZYX 的逆运算；万向锁时取 rz = 0（重建出的矩阵与原本一致） */
void ExtractEulerZYX(const double r[3][3], double& rxDeg, double& ryDeg, double& rzDeg) {
    const double s = std::max(-1.0, std::min(1.0, -r[2][0]));
    const double beta = std::asin(s);

    double alpha = 0.0;
    double gamma = 0.0;
    if (std::abs(r[2][0]) < 1.0 - 1e-9) {
        alpha = std::atan2(r[2][1], r[2][2]);
        gamma = std::atan2(r[1][0], r[0][0]);
    } else {
        alpha = std::atan2(-r[1][2], r[1][1]);
    }

    rxDeg = alpha * 180.0 / kPi;
    ryDeg = beta * 180.0 / kPi;
    rzDeg = gamma * 180.0 / kPi;
}

} // namespace

igQtTransformBoxController::igQtTransformBoxController(iGame::DataObject::Pointer input,
                                                       igQtModelDrawWidget* renderWidget,
                                                       igQtFilterDialogDockWidget* dialog,
                                                       const std::array<int, kParamCount>& parameterIds,
                                                       QObject* parent)
    : QObject(parent), m_Input(input), m_RenderWidget(renderWidget), m_Dialog(dialog) {
    if (m_RenderWidget != nullptr) { m_Scene = m_RenderWidget->GetScene(); }

    if (m_Dialog != nullptr) {
        for (int i = 0; i < kParamCount; ++i) {
            m_Edits[i] = dynamic_cast<QLineEdit*>(m_Dialog->getWidget(parameterIds[i]));
        }
        // 输入框改动 -> 参数 -> 盒子
        for (int i = 0; i < kParamCount; ++i) {
            if (m_Edits[i] == nullptr) { continue; }
            connect(m_Edits[i], &QLineEdit::textChanged, this, [this](const QString&) {
                if (m_PreventSync) { return; }
                if (!ReadParametersFromDialog()) { return; }
                SyncParametersToBox();
            });
        }
        // 面板关闭（对话框自毁）-> 清理盒子 -> 通知主窗口
        connect(m_Dialog, &QObject::destroyed, this, [this]() {
            m_Dialog = nullptr;
            for (int i = 0; i < kParamCount; ++i) { m_Edits[i] = nullptr; }
            DestroyBox();
            emit Closed();
        });
    }

    ComputeInitialBounds();
    // 初值取自面板默认值（平移 0 / 旋转 0 / 缩放 1），解析失败时保持单位变换
    ReadParametersFromDialog();
    BuildBox();
}

igQtTransformBoxController::~igQtTransformBoxController() {
    DestroyBox();
}

void igQtTransformBoxController::ComputeInitialBounds() {
    m_Center0[0] = m_Center0[1] = m_Center0[2] = 0.0;
    m_Length0[0] = m_Length0[1] = m_Length0[2] = 1.0;
    if (m_Input == nullptr) { return; }

    const BoundingBox& bbox = m_Input->GetBoundingBox();
    const double minP[3] = {bbox.min[0], bbox.min[1], bbox.min[2]};
    const double maxP[3] = {bbox.max[0], bbox.max[1], bbox.max[2]};

    double maxLength = 0.0;
    for (int i = 0; i < 3; ++i) {
        m_Center0[i] = 0.5 * (minP[i] + maxP[i]);
        const double length = maxP[i] - minP[i];
        m_Length0[i] = (length > 0.0) ? length : 0.0;
        maxLength = std::max(maxLength, m_Length0[i]);
    }

    // 退化保护：完全扁平的模型（某个方向长度为 0）给一点厚度，保证盒子仍可拖拽
    const double fallback = (maxLength > 1e-9) ? maxLength * 0.05 : 1.0;
    for (int i = 0; i < 3; ++i) {
        if (m_Length0[i] < 1e-6) { m_Length0[i] = fallback; }
    }
}

bool igQtTransformBoxController::BuildBox() {
    if (m_Scene == nullptr || m_RenderWidget == nullptr) { return false; }
    auto interactor = m_Scene->GetInteractor();
    if (interactor == nullptr) { return false; }

    // 盒子门控是全局单例、与"框选"共用：先清掉框选盒子，避免两个盒子同时响应鼠标
    SelectionParameter::Instance().SetHaveBox(false);
    interactor->RemoveSepcialInteractor("SelectBox");

    m_BoxStyle = BoxStyle::New();
    m_BoxStyle->Initialize(interactor);

    m_BoxStyle->SetUpdateWidgetFunc([this]() {
        SelectionParameter::Instance().SetHaveBox(true);
        RedrawBox();
    });

    const double half[3] = {0.5 * m_Length0[0], 0.5 * m_Length0[1], 0.5 * m_Length0[2]};
    Point pMin(m_Center0[0] - half[0], m_Center0[1] - half[1], m_Center0[2] - half[2]);
    Point pMax(m_Center0[0] + half[0], m_Center0[1] + half[1], m_Center0[2] + half[2]);
    m_BoxStyle->InitBox(pMin, pMax);
    m_Box = m_BoxStyle->GetBox();

    interactor->_SetSpecialInteractor(kInteractorName(), m_BoxStyle);
    m_BoxStyle->_SetPointMoveCallBack(kCallbackName(), [this]() { SyncBoxToParameters(); });
    SelectionParameter::Instance().SetHaveBox(true);
    m_RenderWidget->update();
    return m_Box != nullptr;
}

void igQtTransformBoxController::DestroyBox() {
    if (m_BoxStyle != nullptr) { m_BoxStyle->RemovePointMoveCallBack(kCallbackName()); }
    if (m_Scene != nullptr) {
        auto interactor = m_Scene->GetInteractor();
        if (interactor != nullptr) { interactor->RemoveSepcialInteractor(kInteractorName()); }
    }
    if (m_BoxStyle != nullptr && m_Scene != nullptr) {
        m_BoxStyle->RemoveUpdateWidgetFunc();
        m_BoxStyle->ClearDraw();
        m_BoxStyle->DeleteBox();
    }
    m_Box = nullptr;
    m_BoxStyle = nullptr;
    SelectionParameter::Instance().SetHaveBox(false);
    if (m_RenderWidget != nullptr) { m_RenderWidget->update(); }
}

void igQtTransformBoxController::RedrawBox() {
    if (m_BoxStyle == nullptr) { return; }
    m_BoxStyle->ClearDraw();
    m_BoxStyle->ToDraw();
    if (m_RenderWidget != nullptr) { m_RenderWidget->update(); }
}

void igQtTransformBoxController::SyncBoxToParameters() {
    if (m_Box == nullptr) { return; }

    const Point& center = m_Box->GetMidPoint();
    const Point& length = m_Box->GetLength();

    double rotation[3][3];
    ToMath3x3(m_Box->GetRotationMatrix(), rotation);

    double rxDeg = 0.0;
    double ryDeg = 0.0;
    double rzDeg = 0.0;
    ExtractEulerZYX(rotation, rxDeg, ryDeg, rzDeg);

    // 轴心 = 初始包围盒中心
    m_Parameters[0] = static_cast<double>(center[0]) - m_Center0[0];
    m_Parameters[1] = static_cast<double>(center[1]) - m_Center0[1];
    m_Parameters[2] = static_cast<double>(center[2]) - m_Center0[2];
    m_Parameters[3] = rxDeg;
    m_Parameters[4] = ryDeg;
    m_Parameters[5] = rzDeg;

    for (int i = 0; i < 3; ++i) {
        double magnitude = (m_Length0[i] > 1e-12) ? (static_cast<double>(length[i]) / m_Length0[i]) : 1.0;
        if (magnitude < kMinScale) { magnitude = kMinScale; }
        const double sign = (m_Parameters[6 + i] < 0.0) ? -1.0 : 1.0;
        m_Parameters[6 + i] = sign * magnitude;
    }

    WriteParametersToDialog();
}

void igQtTransformBoxController::SyncParametersToBox() {
    if (m_Box == nullptr) { return; }

    double rotation[3][3];
    ComposeEulerZYX(m_Parameters[3], m_Parameters[4], m_Parameters[5], rotation);
    m_Box->SetRotation(FromMath3x3(rotation));
    // 盒子长度必须为正：DynamicBox 会把 <0.01 的长度夹到 0.01。
    // 镜像（负缩放）只体现在参数符号上，盒子取绝对值显示"镜像后包围盒"的大小。
    m_Box->SetLength(std::abs(m_Parameters[6]) * m_Length0[0],
                     std::abs(m_Parameters[7]) * m_Length0[1],
                     std::abs(m_Parameters[8]) * m_Length0[2]);
    m_Box->MovePosition(m_Center0[0] + m_Parameters[0],
                        m_Center0[1] + m_Parameters[1],
                        m_Center0[2] + m_Parameters[2]);
    RedrawBox();
}

bool igQtTransformBoxController::ReadParametersFromDialog() {
    double values[kParamCount]{};
    for (int i = 0; i < kParamCount; ++i) {
        if (m_Edits[i] == nullptr) { return false; }
        bool ok = false;
        values[i] = m_Edits[i]->text().toDouble(&ok);
        if (!ok) { return false; }
    }
    for (int i = 0; i < 3; ++i) {
        double scale = values[6 + i];
        if (std::abs(scale) < kMinScale) { scale = (scale < 0.0) ? -kMinScale : kMinScale; }
        values[6 + i] = scale;
    }
    for (int i = 0; i < kParamCount; ++i) { m_Parameters[i] = values[i]; }
    return true;
}

void igQtTransformBoxController::WriteParametersToDialog() {
    m_PreventSync = true;
    for (int i = 0; i < kParamCount; ++i) {
        if (m_Edits[i] == nullptr) { continue; }
        m_Edits[i]->setText(QString::number(m_Parameters[i], 'g', 9));
    }
    m_PreventSync = false;
}

void igQtTransformBoxController::Reset() {
    const double identity[kParamCount] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
    for (int i = 0; i < kParamCount; ++i) { m_Parameters[i] = identity[i]; }
    WriteParametersToDialog();
    SyncParametersToBox();

    // 盒子回到初始位置后可能离当前模型很远，把视角对到盒子上，保证看得见、点得到
    if (m_Scene != nullptr && m_Box != nullptr) {
        const auto extreme = m_Box->GetExtremePoint();
        const double minP[3] = {extreme.first[0], extreme.first[1], extreme.first[2]};
        const double maxP[3] = {extreme.second[0], extreme.second[1], extreme.second[2]};
        m_Scene->ResetCameraView(BoundingBox(minP, maxP));
        if (m_RenderWidget != nullptr) { m_RenderWidget->update(); }
    }
}

void igQtTransformBoxController::FocusPanel() {
    if (m_Dialog == nullptr) { return; }
    m_Dialog->show();
    m_Dialog->raise();
    m_Dialog->activateWindow();
}

DataObject::Pointer igQtTransformBoxController::RunFilter() {
    if (m_Input == nullptr) { return nullptr; }

    auto filter = TransformFilter::New();
    filter->SetInput(m_Input);
    // 轴心取初始包围盒中心：旋转/缩放都绕模型自身中心进行
    filter->SetPivot(m_Center0[0], m_Center0[1], m_Center0[2]);
    filter->SetTranslation(m_Parameters[0], m_Parameters[1], m_Parameters[2]);
    filter->SetRotation(m_Parameters[3], m_Parameters[4], m_Parameters[5]);
    filter->SetScale(m_Parameters[6], m_Parameters[7], m_Parameters[8]);
    if (!filter->Execute()) { return nullptr; }

    auto output = filter->GetOutput();
    if (output != nullptr) { output->SetName(m_Input->GetName() + "_Transform"); }
    return output;
}

void igQtTransformBoxController::Apply() {
    auto output = RunFilter();
    if (output == nullptr) {
        emit Failed();
        return;
    }
    emit Applied(output);
}
