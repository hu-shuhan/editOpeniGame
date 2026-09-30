//
// 高程 (Elevation) 实时参数面板：见头文件说明。
// 参数校验在前、Execute 在后；成功后仅发信号，渲染刷新由主窗口完成。
//

#include <IQWidgets/igQtElevationFilterPanel.h>

#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

// 数值输入框与 X/Y/Z 轴按钮的统一宽度：同一常量控制，行内同宽对齐、整体紧凑
constexpr int kFieldWidth = 140;

// 紧凑数值输入框：显示时去掉小数尾零（1 而非 1.000000），输入仍允许 6 位小数，
// 悬浮提示完整数值（窄框截断长小数时避免看不到），固定窄宽度让面板整体收窄
class CompactDoubleSpinBox : public QDoubleSpinBox {
public:
    explicit CompactDoubleSpinBox(QWidget* parent = nullptr) : QDoubleSpinBox(parent) {
        setDecimals(6);        // 输入精度：允许用户输入 6 位小数
        setRange(-1e9, 1e9);   // 宽值域，接近 ParaView 的自由输入体验
        setSingleStep(0.1);
        setFixedWidth(kFieldWidth);  // 与轴按钮同宽；更长数值框内滚动 + 悬浮提示补全
        // 任何途径改值（键盘 / 上下箭头 / 程序 setValue）都同步悬浮提示为完整精度值
        connect(this, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
                [this](double v) { setToolTip(QString::number(v, 'g', 10)); });
    }

protected:
    // 显示紧凑化：0.500000 -> "0.5"、1.000000 -> "1"（先走基类格式化，再裁掉小数尾零）
    QString textFromValue(double value) const override {
        QString text = QDoubleSpinBox::textFromValue(value);
        if (text.contains(QLatin1Char('.'))) {
            while (text.endsWith(QLatin1Char('0'))) { text.chop(1); }
            if (text.endsWith(QLatin1Char('.'))) { text.chop(1); }
        }
        return text;
    }
};

}  // namespace

igQtElevationFilterPanel::igQtElevationFilterPanel(QWidget* parent) : QDockWidget(parent) {
    setObjectName(QStringLiteral("dockWidget_ElevationPanel"));
    setWindowTitle(QStringLiteral("高程 (Elevation)"));
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetClosable);
    buildUi();
}

void igQtElevationFilterPanel::buildUi() {
    // 数值输入框统一构造：紧凑显示 + 6 位小数输入 + 悬浮显示完整值（见 CompactDoubleSpinBox）
    auto makeSpin = [this]() { return new CompactDoubleSpinBox(this); };
    for (int i = 0; i < 3; ++i) {
        m_LowPointSpin[i] = makeSpin();
        m_HighPointSpin[i] = makeSpin();
    }
    m_RangeLowSpin = makeSpin();  // 标量范围默认 [0, 1]（与 ParaView 默认一致）
    m_RangeLowSpin->setValue(0.0);
    m_RangeHighSpin = makeSpin();
    m_RangeHighSpin->setValue(1.0);

    // X/Y/Z 轴按钮：互斥选中，默认选中 X（与入口对话框默认轴一致）
    m_AxisGroup = new QButtonGroup(this);
    m_AxisGroup->setExclusive(true);
    auto makeAxisButton = [this](const QString& text) {
        auto* btn = new QPushButton(text, this);
        btn->setCheckable(true);
        btn->setFixedWidth(kFieldWidth);  // 与数值输入框同宽，行内对齐
        m_AxisGroup->addButton(btn);
        return btn;
    };
    auto* axisX = makeAxisButton(QStringLiteral("X"));
    auto* axisY = makeAxisButton(QStringLiteral("Y"));
    auto* axisZ = makeAxisButton(QStringLiteral("Z"));
    axisX->setChecked(true);
    connect(axisX, &QPushButton::clicked, this, &igQtElevationFilterPanel::onAxisX);
    connect(axisY, &QPushButton::clicked, this, &igQtElevationFilterPanel::onAxisY);
    connect(axisZ, &QPushButton::clicked, this, &igQtElevationFilterPanel::onAxisZ);

    // 布局自上而下：轴按钮行 -> 低点 -> 高点 -> 标量范围 -> 应用（对齐 ParaView Properties 顺序）
    auto* axisRow = new QHBoxLayout();
    axisRow->addWidget(new QLabel(QStringLiteral("投影轴"), this));
    axisRow->addWidget(axisX);
    axisRow->addWidget(axisY);
    axisRow->addWidget(axisZ);

    auto makePointRow = [this](const QString& label, QDoubleSpinBox* spins[3]) {
        auto* row = new QHBoxLayout();
        row->addWidget(new QLabel(label, this));
        for (int i = 0; i < 3; ++i) row->addWidget(spins[i]);
        return row;
    };
    auto* rangeRow = new QHBoxLayout();
    rangeRow->addWidget(new QLabel(QStringLiteral("标量范围"), this));
    rangeRow->addWidget(m_RangeLowSpin);
    rangeRow->addWidget(m_RangeHighSpin);

    m_ApplyButton = new QPushButton(QStringLiteral("应用"), this);
    connect(m_ApplyButton, &QPushButton::clicked, this, &igQtElevationFilterPanel::onApply);

    auto* layout = new QVBoxLayout();
    layout->addLayout(axisRow);
    layout->addLayout(makePointRow(QStringLiteral("低点"), m_LowPointSpin));
    layout->addLayout(makePointRow(QStringLiteral("高点"), m_HighPointSpin));
    layout->addLayout(rangeRow);
    layout->addWidget(m_ApplyButton);
    layout->addStretch();

    auto* container = new QWidget(this);
    container->setLayout(layout);
    setWidget(container);
}

void igQtElevationFilterPanel::BindSession(iGame::DataObject::Pointer input, iGame::ElevationFilter::Pointer filter) {
    // 防御：空指针直接拒绝绑定
    if (!input || !filter) return;
    m_Input = input;
    m_Filter = filter;
    syncFromFilter();
    show();
    raise();
}

void igQtElevationFilterPanel::UnbindSession() {
    m_Input = nullptr;
    m_Filter = nullptr;
    hide();
}

// 模型树删除回调：被删对象是本面板绑定的输出节点时，自动关闭面板并解除会话
void igQtElevationFilterPanel::onModelDeleted(const std::string& modelName) {
    if (!m_Filter || !m_Filter->GetOutput()) { return; }
    if (modelName == m_Filter->GetOutput()->GetName()) {
        UnbindSession();
    }
}

void igQtElevationFilterPanel::syncFromFilter() {
    if (!m_Filter) return;
    const auto& low = m_Filter->GetLowPoint();
    const auto& high = m_Filter->GetHighPoint();
    for (int i = 0; i < 3; ++i) {
        m_LowPointSpin[i]->setValue(low[i]);
        m_HighPointSpin[i]->setValue(high[i]);
    }
    m_RangeLowSpin->setValue(m_Filter->GetScalarRangeLow());
    m_RangeHighSpin->setValue(m_Filter->GetScalarRangeHigh());
}

void igQtElevationFilterPanel::fillRangeByAxis(int axis) {
    if (!checkSession()) return;
    // 防御：包围盒无效（无点数据）时不填充
    const auto& bb = m_Input->GetBoundingBox();
    if (bb.isNull()) {
        emit applyFailed(QStringLiteral("输入模型包围盒无效，无法按轴填充低/高点。"));
        return;
    }
    const auto c = bb.center();
    // 被选轴取包围盒 min/max，其余两轴取中心（与 ParaView 轴按钮语义一致）
    for (int i = 0; i < 3; ++i) {
        m_LowPointSpin[i]->setValue(i == axis ? bb.min[i] : c[i]);
        m_HighPointSpin[i]->setValue(i == axis ? bb.max[i] : c[i]);
    }
}

bool igQtElevationFilterPanel::checkSession() const {
    return m_Input != nullptr && m_Filter != nullptr;
}

void igQtElevationFilterPanel::onApply() {
    if (!checkSession()) {
        emit applyFailed(QStringLiteral("会话未绑定，请先从菜单执行一次高程滤波。"));
        return;
    }
    // 读取面板参数
    const double lx = m_LowPointSpin[0]->value(), ly = m_LowPointSpin[1]->value(), lz = m_LowPointSpin[2]->value();
    const double hx = m_HighPointSpin[0]->value(), hy = m_HighPointSpin[1]->value(), hz = m_HighPointSpin[2]->value();
    const double rLow = m_RangeLowSpin->value(), rHigh = m_RangeHighSpin->value();

    // 前置校验：低点与高点重合 -> 投影方向为零向量，Execute 会被拒绝
    const double vx = hx - lx, vy = hy - ly, vz = hz - lz;
    if (vx * vx + vy * vy + vz * vz == 0.0) {
        emit applyFailed(QStringLiteral("低点不能与高点重合。"));
        return;
    }
    // 前置校验：标量范围必须下限 < 上限
    if (rLow >= rHigh) {
        emit applyFailed(QStringLiteral("标量范围下限必须小于上限。"));
        return;
    }

    // 回写滤波器并重新执行（滤波器内部复用输出对象，取色范围只扩不缩）
    m_Filter->SetInput(m_Input);
    m_Filter->SetLowPoint(lx, ly, lz);
    m_Filter->SetHighPoint(hx, hy, hz);
    m_Filter->SetScalarRange(rLow, rHigh);
    if (!m_Filter->Execute()) {
        emit applyFailed(QStringLiteral("高程计算失败，请检查参数。"));
        return;
    }
    emit elevationApplied(m_Filter->GetOutput());
}

void igQtElevationFilterPanel::onAxisX() { fillRangeByAxis(0); onApply(); }
void igQtElevationFilterPanel::onAxisY() { fillRangeByAxis(1); onApply(); }
void igQtElevationFilterPanel::onAxisZ() { fillRangeByAxis(2); onApply(); }
#include <QCheckBox>
    // "显示轴"开关：勾选后在场景中显示低点-高点连线，可用中键拖拽调整（见 onShowAxisToggled）
    m_ShowAxisCheck = new QCheckBox(QStringLiteral("显示轴"), this);
    connect(m_ShowAxisCheck, &QCheckBox::toggled, this, &igQtElevationFilterPanel::onShowAxisToggled);

    layout->addWidget(m_ShowAxisCheck);
    registerAxisStyle(); // 注册特殊交互器并准备轴绘制
    m_ShowAxisCheck->setChecked(false); // 新会话默认不显示轴
    m_AxisVisible = false;
    unregisterAxisStyle();
    m_AxisVisible = false;




    refreshAxisFromSpinboxes(); // 将轴端点同步为本次应用的参数

// 从 x/y/z 输入框构造模型坐标端点
iGame::Point igQtElevationFilterPanel::readAxisPoint(QDoubleSpinBox* s[3]) {
    return iGame::Point{static_cast<float>(s[0]->value()),
                        static_cast<float>(s[1]->value()),
                        static_cast<float>(s[2]->value())};
}

// 注册特殊交互器：把轴样式叠加到当前场景交互器上（不替换基础旋转/平移/缩放）
bool igQtElevationFilterPanel::registerAxisStyle() {
    if (m_AxisStyle) return true;
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (!scene) return false;
    auto interactor = scene->GetInteractor();
    if (!interactor) return false;

    m_AxisStyle = iGame::LowHighAxisStyle::New();
    m_AxisStyle->Initialize(interactor);
    // 拖拽更新回调：仅回填输入框并请求重绘，不重算着色（点"应用"才 Execute）
    m_AxisStyle->SetUpdateCallBack(
            [this](const iGame::Point& low, const iGame::Point& high) {
                onAxisDragChanged(low, high);
            });
    interactor->_SetSpecialInteractor("ElevationAxis", m_AxisStyle);
    refreshAxisFromSpinboxes();
    m_AxisStyle->SetAxisVisible(m_AxisVisible);
    return true;
}

// 移除特殊交互器并隐藏轴
void igQtElevationFilterPanel::unregisterAxisStyle() {
    if (!m_AxisStyle) return;
    m_AxisStyle->SetAxisVisible(false);
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene && scene->GetInteractor()) {
        scene->GetInteractor()->RemoveSepcialInteractor("ElevationAxis");
    }
    m_AxisStyle = nullptr;
}

// 用输入框当前值回填轴端点并重绘（不重算着色）
void igQtElevationFilterPanel::refreshAxisFromSpinboxes() {
    if (!m_AxisStyle) return;
    const iGame::Point low = readAxisPoint(m_LowPointSpin);
    const iGame::Point high = readAxisPoint(m_HighPointSpin);
    m_AxisStyle->SetAxisPoints(low, high);
}

// "显示轴"勾选：显隐轴并请求重绘
void igQtElevationFilterPanel::onShowAxisToggled(bool checked) {
    if (!m_AxisStyle) return;
    m_AxisVisible = checked;
    if (checked) refreshAxisFromSpinboxes(); // 开启时从输入框同步端点
    m_AxisStyle->SetAxisVisible(checked);
    emit axisDragUpdated(); // 只刷新场景渲染
}

// 拖拽回调：实时回填输入框（拦截信号避免回环），不执行滤波器
void igQtElevationFilterPanel::onAxisDragChanged(const iGame::Point& low,
                                                 const iGame::Point& high) {
    for (int i = 0; i < 3; ++i) {
        m_LowPointSpin[i]->blockSignals(true);
        m_LowPointSpin[i]->setValue(low[i]);
        m_LowPointSpin[i]->blockSignals(false);
        m_HighPointSpin[i]->blockSignals(true);
        m_HighPointSpin[i]->setValue(high[i]);
        m_HighPointSpin[i]->blockSignals(false);
    }
    emit axisDragUpdated(); // 仅刷新渲染，着色不变
}
