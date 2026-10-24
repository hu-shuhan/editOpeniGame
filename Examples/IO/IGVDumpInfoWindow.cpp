/**
 * @file  IGVDumpInfoWindow.cpp
 * @brief `igvDump --gui`：ParaView Information 风格的 IGV 元数据面板（Qt Widgets 实现）。
 *
 * 为什么要有它：`igvDump` 的文本模式对一个**结构**问题很好用，但"这个文件里到底有哪些
 * 属性、每个属性的范围是多少、包围盒多大"这类**看数**的需求，文本模式要人肉在几百行
 * dump 里找。ParaView 的 Information 面板正是这个形状，所以这里照它的三段式做：
 *
 *   File Properties（名字 / 路径 / 大小 / 版本 / 生成器）
 *   Data Statistics（类型 / 单元数 / 点数 / 内存 / Bounds）
 *   Data Arrays      （依附 / Name / Type / 分量数 / Ranges）
 *
 * 两个刻意的设计约束（与 IGV 其它工具一致）：
 *
 * 1. **只走元数据快路径**（`IGVMetadataReader::ReadFromFile`），不解压任何 dataset。
 *    因此 5 GiB 的 `.igv` 也是"瞬时打开"；状态栏会把 `H5Dread` 次数打出来，
 *    这个数字应该是 1（只读 `Version`，见 `iGameIGVMetadataReader.h` 的说明）。
 * 2. **不自行重算 range**：面板显示的就是文件里预计算的值（`/IGV/*Data/<name>` 上的
 *    `range` attribute）。这样它同时是一个"写出侧 range 对不对"的检查入口 ——
 *    如果显示与常识不符，问题在文件而不在工具。
 *
 * 没有使用 `Q_OBJECT`：本类不需要信号/槽声明（全部用 lambda 连接），
 * 因此不引入 moc 依赖，`target_sources` 加进来即可编译。
 *
 * 关于本文件里的中文字面量：它们是**界面字符串**（菜单/标签/状态栏），走 Qt 的
 * Unicode 绘制路径，不经过终端代码页，因此不受"控制台必须 ASCII"那条约束
 * （截图成功/失败那一行）是英文。
 */

#include "IGVDumpInfoWindow.h"

#include <IGV/iGameIGVFormat.h>
#include <IGV/iGameIGVMetadataReader.h>
#include <IGV/iGameIGVStats.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

namespace {

using namespace iGame;

// =============================================================================
// 数值 / 单位格式化
// =============================================================================
//
// 统一用 7 位有效数字（ParaView 的量级）：既能看清 `-12.1696`，又不会把
// `float32` 的十进制噪声（0.10000000149）整串糊在面板上。

QString FormatDouble(double v) {
    if (std::isnan(v)) { return QStringLiteral("nan"); }
    if (std::isinf(v)) { return v > 0 ? QStringLiteral("inf") : QStringLiteral("-inf"); }
    if (v == 0.0) { return QStringLiteral("0"); }
    return QString::number(v, 'g', 7);
}

QString FormatBytes(int64_t bytes) {
    if (bytes < 0) { return QStringLiteral("-"); }
    const double b = static_cast<double>(bytes);
    if (bytes < 1024) { return QString::number(bytes) + QStringLiteral(" B"); }
    if (b < 1024.0 * 1024.0) { return QString::number(b / 1024.0, 'f', 3) + QStringLiteral(" KB"); }
    if (b < 1024.0 * 1024.0 * 1024.0) {
        return QString::number(b / (1024.0 * 1024.0), 'f', 5) + QStringLiteral(" MB");
    }
    return QString::number(b / (1024.0 * 1024.0 * 1024.0), 'f', 3) + QStringLiteral(" GB");
}

/// 计数加千分位（与 ParaView 的 `# of Cells 4,800` 一致，长数字更好读）。
QString FormatCount(int64_t value) {
    return QLocale(QLocale::English).toString(static_cast<qlonglong>(value));
}

/// 属性数组的类型标签：`Float64(64bit)`。
///
/// 位宽**一定**显示，因为这是用户对着 vtk legacy 的 `long`/`id type` 踩过的坑：
/// 面板把它连名字带位宽一起摊开，读者不需要去查"double 是几位"。
QString DTypeLabelOf(const igv::IGVArrayInfo& array) {
    if (!array.dTypeToken.empty() && array.dTypeBits > 0) {
        return QString::fromStdString(array.dTypeToken) + QStringLiteral("(") +
               QString::number(array.dTypeBits) + QStringLiteral("bit)");
    }
    // 1.0 的老文件没有 DataType attribute：只能按字节数给保守标签（不带符号信息）。
    if (array.valueSize > 0) {
        return QStringLiteral("?(") + QString::number(array.valueSize * 8) + QStringLiteral("bit)");
    }
    return QStringLiteral("-");
}

/// 依附类型 → 面板上的短名。空串（老文件没有 `attachment`）时显示 unknown。
QString ScopeName(const std::string& attachment) {
    if (attachment.empty()) { return QStringLiteral("unknown"); }
    return QString::fromStdString(attachment);
}

/// 依附类型 → 小圆点颜色。沿用 ParaView 的直觉：point 绿、cell 蓝、其它灰。
QColor ScopeColor(const std::string& attachment) {
    if (attachment == "point") { return QColor(0x2e, 0x7d, 0x32); }
    if (attachment == "cell") { return QColor(0x15, 0x65, 0xc0); }
    return QColor(0x75, 0x75, 0x75);
}

/// 画一个 10x10 的实心圆点（不依赖资源文件）。
QIcon MakeDotIcon(const QColor& color) {
    QPixmap pm(10, 10);
    pm.fill(Qt::transparent);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QRectF(0.5, 0.5, 9.0, 9.0));
    painter.end();
    return QIcon(pm);
}

/// 一个属性的 Ranges。
///
///   * `components == 1` → `[min, max]`（长度 2，**不**重复存模长）
///   * `components >  1` → `[magMin, magMax, c0Min, c0Max, …]`（长度 2*(1+k)）
/// 显示成 ParaView 的 `[a, b], [c, d], …`：第一对是模长，后面逐分量。
QString FormatRanges(const igv::IGVArrayInfo& array) {
    const std::vector<double>& r = array.range;
    if (r.size() < 2) { return QStringLiteral("-"); }

    const QString first = QStringLiteral("[") + FormatDouble(r[0]) + QStringLiteral(", ") +
                          FormatDouble(r[1]) + QStringLiteral("]");
    if (array.components <= 1 || r.size() < 2 * (1 + static_cast<size_t>(array.components))) {
        return first;  // 标量：只有一对
    }

    QString text = first;
    for (int c = 0; c < array.components; ++c) {
        const size_t base = 2 + 2 * static_cast<size_t>(c);
        text += QStringLiteral(", [") + FormatDouble(r[base]) + QStringLiteral(", ") +
                FormatDouble(r[base + 1]) + QStringLiteral("]");
    }
    return text;
}

/// 三行 Bounds（与 ParaView 的 `a to b (delta: d)` 写法一致）。
QString FormatBounds(const double bounds[6]) {
    const char* axis[3] = {"X", "Y", "Z"};
    QString text;
    for (int a = 0; a < 3; ++a) {
        const double lo = bounds[2 * a];
        const double hi = bounds[2 * a + 1];
        if (a > 0) { text += QLatin1Char('\n'); }
        text += QString::fromLatin1(axis[a]) + QStringLiteral(": ") + FormatDouble(lo) +
                QStringLiteral(" to ") + FormatDouble(hi) + QStringLiteral("  (delta: ") +
                FormatDouble(hi - lo) + QStringLiteral(")");
    }
    return text;
}

/// 估计"这些数组进内存要多大"（与 ParaView 的 Memory 行同义）。
///
/// 只算**数据本体**：点坐标、连通性、偏移、单元类型、以及每个属性。
/// 这不等于文件大小（gzip 后更小），也不等于峰值内存（读出时还有副本），
/// 所以标签写 `Memory (data)` 而不是 `Memory`。
int64_t EstimateDataBytes(const igv::IGVMetadata& meta) {
    int64_t bytes = 0;

    const int64_t pointsBytes = meta.pointCount * igv::kPointsComponents *
                                static_cast<int64_t>(igv::SizeOfDType(igv::DType::F32));
    bytes += std::max<int64_t>(pointsBytes, 0);

    if (meta.hasConnectivityIdCount && meta.connectivityIdCount > 0) {
        bytes += meta.connectivityIdCount *
                 static_cast<int64_t>(igv::SizeOfDType(igv::DType::I32));
    }
    if (meta.hasCellCount && meta.cellCount > 0) {
        // Offsets 是 ncells+1 个 i64，Types 是 ncells 个 u8。
        bytes += (meta.cellCount + 1) * static_cast<int64_t>(igv::SizeOfDType(igv::DType::I64));
        bytes += meta.cellCount * static_cast<int64_t>(igv::SizeOfDType(igv::DType::U8));
    }
    for (const igv::IGVArrayInfo& a : meta.arrays) {
        bytes += a.count * static_cast<int64_t>(std::max(a.components, 1)) *
                 static_cast<int64_t>(a.valueSize > 0 ? a.valueSize : 4);
    }
    // 多面体的面表：面→点（i32）+ 面→点的偏移（i64，nfaces+1）。少了这一段，
    // 面板的 Memory 会把多面体网格低报一大截。
    if (meta.hasFaceCount && meta.faceCount > 0) {
        bytes += (meta.faceCount + 1) * static_cast<int64_t>(igv::SizeOfDType(igv::DType::I64));
        bytes += meta.faceCount * 3 * static_cast<int64_t>(igv::SizeOfDType(igv::DType::I32));
    }
    return bytes;
}

// =============================================================================
// 一个"标签 + 值"行（ParaView 那种左标签右值的网格）
// =============================================================================

/// 单行中间省略的标签。用于路径这类**很长又不可断行**的值：
/// 让它换行会把 `C:\Projects\...` 拆成两行（第一行只剩 "C:"），看起来像两个路径。
class ElidedLabel : public QLabel {
public:
    explicit ElidedLabel(QWidget* parent = nullptr) : QLabel(parent) { setWordWrap(false); }

    void SetFullText(const QString& text) {
        m_Full = text;
        setToolTip(text);
        UpdateElidedText();
    }
    [[nodiscard]] const QString& FullText() const { return m_Full; }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QLabel::resizeEvent(event);
        UpdateElidedText();
    }

private:
    void UpdateElidedText() {
        const int available = std::max(0, width() - 2);
        const QString elided = fontMetrics().elidedText(m_Full, Qt::ElideMiddle, available);
        // 只在真正变化时才 setText：否则 "setText → 布局 → resizeEvent → setText"
        // 会形成一次重入（不致命，但会让拖动窗口变卡）。
        if (elided != QLabel::text()) { QLabel::setText(elided); }
    }

    QString m_Full;
};

struct InfoRow {
    QLabel* label{nullptr};
    QLabel* value{nullptr};

    void SetVisible(bool visible) const {
        if (label != nullptr) { label->setVisible(visible); }
        if (value != nullptr) { value->setVisible(visible); }
    }
};

InfoRow AddInfoRow(QGridLayout* grid, int row, const QString& title, int labelWidth,
                   QLabel* valueWidget = nullptr) {
    auto* label = new QLabel(title);
    label->setMinimumWidth(labelWidth);
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    label->setStyleSheet(QStringLiteral("color: palette(mid);"));

    QLabel* value = valueWidget;
    if (value == nullptr) {
        value = new QLabel(QStringLiteral("-"));
        value->setWordWrap(true);
    }
    value->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    value->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    grid->addWidget(label, row, 0);
    grid->addWidget(value, row, 1);
    return InfoRow{label, value};
}

/// 建构一个带标题的分组（标题加粗 + 一条细分隔线，接近 ParaView 的分段感）。
QGridLayout* AddSection(QVBoxLayout* outer, const QString& title, QGroupBox** boxOut) {
    auto* box = new QGroupBox(title);
    box->setStyleSheet(QStringLiteral("QGroupBox { font-weight: bold; }"));
    auto* grid = new QGridLayout(box);
    grid->setColumnStretch(1, 1);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(6);
    outer->addWidget(box);
    *boxOut = box;
    return grid;
}

constexpr int kLabelWidth = 150;

// =============================================================================
// 面板
// =============================================================================

class InfoPanel : public QMainWindow {
public:
    explicit InfoPanel(const QString& initialFile) {
        BuildUi();
        if (!initialFile.isEmpty()) {
            LoadFile(initialFile);
        } else {
            Populate();
        }
    }

    void LoadFile(const QString& path) {
        m_Path = path;
        m_Info = Loaded{};

        QApplication::setOverrideCursor(Qt::WaitCursor);
        const std::string native = path.toStdString();
        // FileIO::ReadIGVMetadata 内部就是这两步（Reset + Reader），
        // 这里直接调 Reader 是为了不让这个工具的 TU 把 iGameFileIO.h 拉进来
        igv::IGVStats::Reset();
        std::string error;
        m_Info.ok = IGVMetadataReader::ReadFromFile(native, m_Info.meta, &error);
        m_Info.error = QString::fromStdString(error);
        m_Info.h5dreadCount = igv::IGVStats::h5dreadCount.load();

        // 用 QFileInfo 而不是 std::filesystem::file_size：这里已经依赖 Qt，
        // 而 QFileInfo 在 Windows 上对非 ASCII 路径的处理比 std::filesystem 更省心。
        m_Info.fileSize = QFileInfo(path).size();
        QApplication::restoreOverrideCursor();

        Populate();
    }

protected:
    void dragEnterEvent(QDragEnterEvent* event) override {
        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
        }
    }

    void dropEvent(QDropEvent* event) override {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (urls.isEmpty()) { return; }
        const QString local = urls.first().toLocalFile();
        if (local.isEmpty()) { return; }
        event->acceptProposedAction();
        LoadFile(local);
    }

private:
    struct Loaded {
        bool ok{false};
        QString error;
        int64_t fileSize{-1};
        uint64_t h5dreadCount{0};
        igv::IGVMetadata meta;
    };

    // ---- UI 建构 ----
    void BuildUi() {
        setAcceptDrops(true);
        setMinimumSize(560, 460);
        resize(820, 840);
        setWindowTitle(QStringLiteral("igvDump - Information"));

        // 菜单：把"能做的那几件事"都放进来（同时给出快捷键）。
        QMenu* fileMenu = menuBar()->addMenu(QStringLiteral("文件(&F)"));
        QAction* openAction = fileMenu->addAction(QStringLiteral("打开(&O)..."));
        openAction->setShortcut(QKeySequence::Open);
        connect(openAction, &QAction::triggered, this, [this] { OpenDialog(); });

        QAction* reloadAction = fileMenu->addAction(QStringLiteral("重新读取(&R)"));
        reloadAction->setShortcut(QKeySequence::Refresh);
        connect(reloadAction, &QAction::triggered, this, [this] {
            if (!m_Path.isEmpty()) { LoadFile(m_Path); }
        });
        fileMenu->addSeparator();
        QAction* quitAction = fileMenu->addAction(QStringLiteral("退出(&Q)"));
        quitAction->setShortcut(QKeySequence::Quit);
        connect(quitAction, &QAction::triggered, this, [this] { close(); });

        QMenu* editMenu = menuBar()->addMenu(QStringLiteral("编辑(&E)"));
        QAction* copyAction = editMenu->addAction(QStringLiteral("复制全部为文本(&C)"));
        copyAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+C")));
        connect(copyAction, &QAction::triggered, this, [this] { CopyAll(); });

        QMenu* helpMenu = menuBar()->addMenu(QStringLiteral("帮助(&H)"));
        QAction* aboutAction = helpMenu->addAction(QStringLiteral("关于 igvDump"));
        connect(aboutAction, &QAction::triggered, this, [this] { ShowAbout(); });

        // 工具条：打开 / 重新读取 / 复制 + 属性名过滤。
        QToolBar* toolbar = addToolBar(QStringLiteral("工具"));
        toolbar->setMovable(false);
        toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
        toolbar->addAction(openAction);
        toolbar->addAction(reloadAction);
        toolbar->addAction(copyAction);
        toolbar->addSeparator();

        toolbar->addWidget(new QLabel(QStringLiteral(" 过滤: ")));
        m_Filter = new QLineEdit();
        m_Filter->setPlaceholderText(QStringLiteral("过滤属性名…"));
        m_Filter->setClearButtonEnabled(true);
        m_Filter->setMaximumWidth(220);
        m_Filter->setMinimumWidth(160);
        connect(m_Filter, &QLineEdit::textChanged, this, [this](const QString&) { ApplyFilter(); });
        toolbar->addWidget(m_Filter);

        // 中央：错误/提示横幅 + 三个分组。
        auto* central = new QWidget();
        auto* outer = new QVBoxLayout(central);
        outer->setContentsMargins(10, 8, 10, 8);
        outer->setSpacing(8);

        m_Banner = new QLabel();
        m_Banner->setWordWrap(true);
        m_Banner->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_Banner->setVisible(false);
        outer->addWidget(m_Banner);

        // --- File Properties ---
        QGroupBox* fileBox = nullptr;
        QGridLayout* fileGrid = AddSection(outer, QStringLiteral("File Properties"), &fileBox);
        int row = 0;
        m_Name = AddInfoRow(fileGrid, row++, QStringLiteral("Name"), kLabelWidth);
        m_PathElided = new ElidedLabel();
        m_PathRow = AddInfoRow(fileGrid, row++, QStringLiteral("Path"), kLabelWidth, m_PathElided);
        m_FileSize = AddInfoRow(fileGrid, row++, QStringLiteral("Size on disk"), kLabelWidth);
        m_Version = AddInfoRow(fileGrid, row++, QStringLiteral("IGV Version"), kLabelWidth);
        m_IGVDataType = AddInfoRow(fileGrid, row++, QStringLiteral("IGVDataType"), kLabelWidth);
        m_Generator = AddInfoRow(fileGrid, row++, QStringLiteral("Generator"), kLabelWidth);

        // --- Data Statistics ---
        QGroupBox* statBox = nullptr;
        QGridLayout* statGrid = AddSection(outer, QStringLiteral("Data Statistics"), &statBox);
        row = 0;
        m_Type = AddInfoRow(statGrid, row++, QStringLiteral("Type"), kLabelWidth);
        m_Cells = AddInfoRow(statGrid, row++, QStringLiteral("# of Cells"), kLabelWidth);
        m_Faces = AddInfoRow(statGrid, row++, QStringLiteral("# of Faces"), kLabelWidth);
        m_Points = AddInfoRow(statGrid, row++, QStringLiteral("# of Points"), kLabelWidth);
        m_Conn = AddInfoRow(statGrid, row++, QStringLiteral("Connectivity ids"), kLabelWidth);
        m_Memory = AddInfoRow(statGrid, row++, QStringLiteral("Memory (data)"), kLabelWidth);
        m_Bounds = AddInfoRow(statGrid, row++, QStringLiteral("Bounds"), kLabelWidth);
        m_Dimensions = AddInfoRow(statGrid, row++, QStringLiteral("Dimensions"), kLabelWidth);
        m_FixedCellSize = AddInfoRow(statGrid, row++, QStringLiteral("FixedCellSize"), kLabelWidth);
        m_Time = AddInfoRow(statGrid, row++, QStringLiteral("Time"), kLabelWidth);

        // --- Data Arrays ---
        auto* arraysBox = new QGroupBox(QStringLiteral("Data Arrays"));
        arraysBox->setStyleSheet(QStringLiteral("QGroupBox { font-weight: bold; }"));
        auto* arraysLayout = new QVBoxLayout(arraysBox);
        arraysLayout->setSpacing(4);

        m_ArraysSummary = new QLabel(QStringLiteral("-"));
        m_ArraysSummary->setStyleSheet(QStringLiteral("font-weight: normal; color: palette(mid);"));
        arraysLayout->addWidget(m_ArraysSummary);

        m_Table = new QTableWidget(0, 5);
        m_Table->setHorizontalHeaderLabels(
                {QStringLiteral("Scope"), QStringLiteral("Name"), QStringLiteral("Type"),
                 QStringLiteral("Components"), QStringLiteral("Ranges")});
        m_Table->verticalHeader()->setVisible(false);
        m_Table->setAlternatingRowColors(true);
        m_Table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        // Ranges 一栏对多分量属性比较长（模长 + 逐分量），允许换行比省略更有用：
        // `[0.0147, 1.0015], [2.18e-11, 1], …` 折成两行，比被 "…" 截断清楚得多。
        m_Table->setWordWrap(true);
        m_Table->setTextElideMode(Qt::ElideRight);
        m_Table->setShowGrid(false);
        m_Table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        QHeaderView* header = m_Table->horizontalHeader();
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::Interactive);
        header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(4, QHeaderView::Stretch);
        header->setHighlightSections(false);
        m_Table->setColumnWidth(1, 180);
        arraysLayout->addWidget(m_Table, 1);

        outer->addWidget(arraysBox, 1);
        setCentralWidget(central);

        m_Status = new QLabel();
        statusBar()->addWidget(m_Status, 1);
    }

    void OpenDialog() {
        const QString start = m_Path.isEmpty() ? QString() : QFileInfo(m_Path).absolutePath();
        const QString path = QFileDialog::getOpenFileName(
                this, QStringLiteral("打开 IGV 文件"), start,
                QStringLiteral("IGV 文件 (*.igv);;所有文件 (*)"));
        if (!path.isEmpty()) { LoadFile(path); }
    }

    void CopyAll() {
        if (m_Path.isEmpty()) {
            SetStatus(QStringLiteral("没有可复制的内容（先打开一个 .igv）"));
            return;
        }
        QApplication::clipboard()->setText(BuildText());
        SetStatus(QStringLiteral("已复制到剪贴板（%1 个属性）")
                          .arg(static_cast<int>(m_Info.meta.arrays.size())));
    }

    void ShowAbout() {
        m_Banner->setText(
                QStringLiteral("igvDump --gui：只读 IGV 元数据（attribute + dataspace），"
                               "不解压任何 dataset。\n"
                               "面板显示的 Bounds / Ranges 是文件里**预计算**的值（IGV 的增值项）。\n"
                               "命令行用法见 igvDump --help。"));
        m_Banner->setStyleSheet(QStringLiteral("padding:6px; background:#eef3fb; color:#123;"));
        m_Banner->setVisible(true);
    }

    void SetStatus(const QString& text) { m_Status->setText(text); }

    void ShowBanner(const QString& text, bool isError) {
        m_Banner->setText(text);
        m_Banner->setStyleSheet(
                isError ? QStringLiteral("padding:6px; background:#fdecea; color:#611a15;")
                        : QStringLiteral("padding:6px; background:#eef3fb; color:#123;"));
        m_Banner->setVisible(!text.isEmpty());
    }

    /// 把 m_Info / m_Path 刷到界面上（加载成功、失败、空面板都走这一条路）。
    void Populate() {
        const bool hasFile = !m_Path.isEmpty();
        const bool ok = hasFile && m_Info.ok;
        const igv::IGVMetadata& meta = m_Info.meta;

        const QFileInfo info(m_Path);
        setWindowTitle(hasFile ? QStringLiteral("igvDump - Information - %1").arg(info.fileName())
                               : QStringLiteral("igvDump - Information"));

        // ---- File Properties ----
        m_Name.value->setText(hasFile ? info.fileName() : QStringLiteral("-"));
        m_PathElided->SetFullText(hasFile ? QDir::toNativeSeparators(info.absoluteFilePath())
                                          : QStringLiteral("-"));
        m_FileSize.value->setText(FormatBytes(m_Info.fileSize));
        m_Version.value->setText(
                ok ? QStringLiteral("%1.%2").arg(meta.versionMajor).arg(meta.versionMinor)
                   : QStringLiteral("-"));
        m_IGVDataType.value->setText(ok && !meta.igvDataType.empty()
                                             ? QString::fromStdString(meta.igvDataType)
                                             : QStringLiteral("-"));
        m_Generator.value->setText(ok && !meta.generator.empty()
                                           ? QString::fromStdString(meta.generator)
                                           : QStringLiteral("-"));

        // ---- Data Statistics ----
        if (ok) {
            const QString igvType = QString::fromStdString(meta.type);
            const QString igvDataType = QString::fromStdString(meta.igvDataType);
            m_Type.value->setText(igvDataType.isEmpty() || igvDataType == igvType
                                          ? igvType
                                          : QStringLiteral("%1  (%2)").arg(igvType, igvDataType));
            m_Cells.value->setText(meta.hasCellCount ? FormatCount(meta.cellCount)
                                                     : QStringLiteral("n/a"));
            m_Points.value->setText(FormatCount(meta.pointCount));
            // 多面体（二级索引）才有面数；一级索引文件里这一行隐藏，避免"No data"噪声。
            m_Faces.SetVisible(meta.hasFaceCount);
            if (meta.hasFaceCount) {
                m_Faces.value->setText(FormatCount(meta.faceCount));
            }
            if (!meta.topologyIndex.empty() &&
                meta.topologyIndex != igv::kTopologyIndexPrimary) {
                m_Faces.value->setToolTip(
                        QStringLiteral("TopologyIndex = %1（Connectivity 存的是体→面 的面 id，"
                                       "面→点 在 FaceConnectivity 里）")
                                .arg(QString::fromStdString(meta.topologyIndex)));
            }
            m_Memory.value->setText(FormatBytes(EstimateDataBytes(meta)));
            m_Bounds.value->setText(meta.hasBounds ? FormatBounds(meta.bounds)
                                                   : QStringLiteral("(文件未写 Bounds attribute)"));
            m_Conn.SetVisible(meta.hasConnectivityIdCount);
            if (meta.hasConnectivityIdCount) {
                m_Conn.value->setText(FormatCount(meta.connectivityIdCount));
            }
            m_Dimensions.SetVisible(meta.hasDimensions);
            if (meta.hasDimensions) {
                m_Dimensions.value->setText(
                        QStringLiteral("%1 x %2 x %3")
                                .arg(meta.dimensions[0])
                                .arg(meta.dimensions[1])
                                .arg(meta.dimensions[2]));
            }
            m_FixedCellSize.SetVisible(meta.fixedCellSize >= 0);
            if (meta.fixedCellSize >= 0) {
                m_FixedCellSize.value->setText(
                        QStringLiteral("%1（定长单元，读回走快速路径）").arg(meta.fixedCellSize));
            }
            const bool hasTime = meta.hasTimeValue || meta.hasTimeStep;
            m_Time.SetVisible(hasTime);
            if (hasTime) {
                QStringList parts;
                if (meta.hasTimeValue) { parts << QStringLiteral("TimeValue = %1").arg(FormatDouble(meta.timeValue)); }
                if (meta.hasTimeStep) { parts << QStringLiteral("TimeStep = %1").arg(meta.timeStep); }
                m_Time.value->setText(parts.join(QStringLiteral(", ")));
            }
        } else {
            for (InfoRow* r : {&m_Type, &m_Cells, &m_Points, &m_Memory, &m_Bounds, &m_Dimensions,
                              &m_FixedCellSize, &m_Time}) {
                r->value->setText(QStringLiteral("-"));
            }
            m_Conn.SetVisible(false);
            m_Dimensions.SetVisible(false);
            m_FixedCellSize.SetVisible(false);
            m_Time.SetVisible(false);
        }

        // ---- Data Arrays ----
        PopulateArrays(ok);

        // ---- 横幅 / 状态栏 ----
        if (!hasFile) {
            ShowBanner(QStringLiteral("拖放一个 .igv 文件到这里，或者用「文件 → 打开…」。"
                                      "本工具只读元数据，不加载网格数据。"),
                       false);
            SetStatus(QStringLiteral("就绪"));
        } else if (!ok) {
            ShowBanner(QStringLiteral("无法读取该文件的 IGV 元数据：\n") + m_Info.error, true);
            SetStatus(QStringLiteral("读取失败"));
        } else {
            ShowBanner(QString(), false);
            SetStatus(QStringLiteral("元数据快路径：共 %1 次 H5Dread（仅 Version），未解压任何 dataset · "
                                     "%2 个属性 · %3")
                              .arg(m_Info.h5dreadCount)
                              .arg(static_cast<int>(meta.arrays.size()))
                              .arg(FormatBytes(EstimateDataBytes(meta))));
        }

        ApplyFilter();
    }

    void PopulateArrays(bool ok) {
        m_Table->setUpdatesEnabled(false);
        m_Table->clearContents();
        m_Table->setRowCount(0);
        if (!ok) {
            m_ArraysSummary->setText(QStringLiteral("-"));
            m_Table->setUpdatesEnabled(true);
            return;
        }

        const std::vector<igv::IGVArrayInfo>& arrays = m_Info.meta.arrays;
        m_Table->setRowCount(static_cast<int>(arrays.size()));
        for (int i = 0; i < static_cast<int>(arrays.size()); ++i) {
            const igv::IGVArrayInfo& a = arrays[static_cast<size_t>(i)];
            const QString scope = ScopeName(a.attachment);

            auto* scopeItem = new QTableWidgetItem(MakeDotIcon(ScopeColor(a.attachment)), scope);
            scopeItem->setToolTip(QStringLiteral("attachment = %1").arg(scope));
            m_Table->setItem(i, 0, scopeItem);

            auto* nameItem = new QTableWidgetItem(QString::fromStdString(a.name));
            nameItem->setToolTip(QString::fromStdString(a.name));
            m_Table->setItem(i, 1, nameItem);

            auto* typeItem = new QTableWidgetItem(DTypeLabelOf(a));
            const igv::DType arrayDType = igv::ParseDTypeToken(a.dTypeToken);
            const QString declared = a.dTypeToken.empty()
                                             ? QStringLiteral("(not declared, inferred from HDF5)")
                                             : QString::fromStdString(a.dTypeToken);
            typeItem->setToolTip(
                    QStringLiteral("DataType = %1, %2 bit\n%3\nAttributeType = %4")
                            .arg(declared)
                            .arg(a.dTypeBits)
                            .arg(QString::fromStdString(std::string(igv::DTypeVTKName(arrayDType))))
                            .arg(QString::fromStdString(a.attributeType)));
            m_Table->setItem(i, 2, typeItem);

            auto* compItem = new QTableWidgetItem(QString::number(a.components));
            m_Table->setItem(i, 3, compItem);

            auto* rangeItem = new QTableWidgetItem(FormatRanges(a));
            rangeItem->setToolTip(rangeItem->text());
            m_Table->setItem(i, 4, rangeItem);
        }
        m_Table->setUpdatesEnabled(true);
        // 开了 wordWrap 之后必须显式重算行高，否则换行的 Ranges 会被裁掉半行。
        m_Table->resizeRowsToContents();
    }

    /// 按属性名过滤（只隐藏行，不重建 — 保留列的宽度与选中状态）。
    void ApplyFilter() {
        if (m_Table == nullptr || m_Filter == nullptr) { return; }
        const QString needle = m_Filter->text().trimmed();
        int visible = 0;
        for (int row = 0; row < m_Table->rowCount(); ++row) {
            QTableWidgetItem* item = m_Table->item(row, 1);
            const bool match = needle.isEmpty() || item == nullptr ||
                               item->text().contains(needle, Qt::CaseInsensitive);
            m_Table->setRowHidden(row, !match);
            if (match) { ++visible; }
        }
        const int total = m_Table->rowCount();
        if (total == 0) {
            m_ArraysSummary->setText(QStringLiteral("无属性"));
        } else if (visible == total) {
            m_ArraysSummary->setText(QStringLiteral("%1 个属性").arg(total));
        } else {
            m_ArraysSummary->setText(QStringLiteral("%1 / %2 个属性（已过滤）").arg(visible).arg(total));
        }
    }

    /// 面板内容的纯文本形式（`复制全部` 用）。
    ///
    /// 刻意与面板**同构**：贴到一个 issue 里，别人看到的就是你看到的，
    /// 不需要再让他去装一次工具。
    QString BuildText() const {
        const igv::IGVMetadata& meta = m_Info.meta;
        QString out;
        out += QStringLiteral("=== File Properties ===\n");
        out += QStringLiteral("Name            : %1\n").arg(m_Name.value->text());
        out += QStringLiteral("Path            : %1\n").arg(m_PathElided->FullText());
        out += QStringLiteral("Size on disk    : %1\n").arg(m_FileSize.value->text());
        out += QStringLiteral("IGV Version     : %1\n").arg(m_Version.value->text());
        out += QStringLiteral("IGVDataType     : %1\n").arg(m_IGVDataType.value->text());
        out += QStringLiteral("Generator       : %1\n").arg(m_Generator.value->text());

        out += QStringLiteral("\n=== Data Statistics ===\n");
        out += QStringLiteral("Type            : %1\n").arg(m_Type.value->text());
        out += QStringLiteral("# of Cells      : %1\n").arg(m_Cells.value->text());
        if (meta.hasFaceCount) {
            out += QStringLiteral("# of Faces      : %1\n").arg(m_Faces.value->text());
        }
        if (!meta.topologyIndex.empty()) {
            out += QStringLiteral("TopologyIndex   : %1\n")
                           .arg(QString::fromStdString(meta.topologyIndex));
        }
        out += QStringLiteral("# of Points     : %1\n").arg(m_Points.value->text());
        if (meta.hasConnectivityIdCount) {
            out += QStringLiteral("Connectivity ids: %1\n").arg(m_Conn.value->text());
        }
        out += QStringLiteral("Memory (data)   : %1\n").arg(m_Memory.value->text());
        out += QStringLiteral("Bounds          : %1\n")
                       .arg(m_Bounds.value->text().replace(QLatin1Char('\n'), QLatin1Char(';')));
        if (meta.hasDimensions) {
            out += QStringLiteral("Dimensions      : %1\n").arg(m_Dimensions.value->text());
        }
        if (meta.fixedCellSize >= 0) {
            out += QStringLiteral("FixedCellSize   : %1\n").arg(meta.fixedCellSize);
        }
        if (meta.hasTimeValue || meta.hasTimeStep) {
            out += QStringLiteral("Time            : %1\n").arg(m_Time.value->text());
        }

        out += QStringLiteral("\n=== Data Arrays (%1) ===\n").arg(static_cast<int>(meta.arrays.size()));
        out += QStringLiteral("  %1 %2 %3 %4 %5\n")
                       .arg(QStringLiteral("scope").leftJustified(9),
                            QStringLiteral("name").leftJustified(28),
                            QStringLiteral("type").leftJustified(9),
                            QStringLiteral("comp").leftJustified(5),
                            QStringLiteral("ranges"));
        for (const igv::IGVArrayInfo& a : meta.arrays) {
            // 复制出来的文本与面板同构：类型也带位宽（`Float64(64bit)`）。
            out += QStringLiteral("  %1 %2 %3 %4 %5\n")
                           .arg(ScopeName(a.attachment).leftJustified(9),
                                QString::fromStdString(a.name).leftJustified(28),
                                DTypeLabelOf(a).leftJustified(14),
                                QString::number(a.components).leftJustified(5),
                                FormatRanges(a));
        }
        out += QStringLiteral("\n(metadata only: %1 H5Dread call(s), no dataset was decompressed)\n")
                       .arg(m_Info.h5dreadCount);
        return out;
    }

    // ---- 三组"标签 + 值" ----
    InfoRow m_Name;
    InfoRow m_PathRow;
    ElidedLabel* m_PathElided{nullptr};
    InfoRow m_FileSize;
    InfoRow m_Version;
    InfoRow m_IGVDataType;
    InfoRow m_Generator;

    InfoRow m_Type;
    InfoRow m_Cells;
    InfoRow m_Faces;
    InfoRow m_Points;
    InfoRow m_Conn;
    InfoRow m_Memory;
    InfoRow m_Bounds;
    InfoRow m_Dimensions;
    InfoRow m_FixedCellSize;
    InfoRow m_Time;

    QLabel* m_Banner{nullptr};
    QLabel* m_ArraysSummary{nullptr};
    QLabel* m_Status{nullptr};
    QLineEdit* m_Filter{nullptr};
    QTableWidget* m_Table{nullptr};

    QString m_Path;
    Loaded m_Info;
};

}  // namespace

int RunIGVDumpInfoWindow(const std::string& initialFile, const std::string& screenshotPath) {
    // 高 DPI 必须在 QApplication 构造**之前**设置（Qt 5 的要求）。
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    int argc = 1;
    char executableName[] = "igvDump";
    char* argv[] = {executableName, nullptr};
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("igvDump"));
    QApplication::setApplicationVersion(QStringLiteral("1.0"));

    InfoPanel panel(QString::fromStdString(initialFile));
    panel.show();

    if (!screenshotPath.empty()) {
        // UI 冒烟：等一帧（让布局与样式表生效）后把窗口渲染成 PNG，然后退出。
        // 用 `QWidget::grab()` 而不是截屏，所以不依赖桌面 —— 配合
        // `QT_QPA_PLATFORM=offscreen` 就能在 CI/无头环境里跑。
        const QString out = QString::fromStdString(screenshotPath);
        QTimer::singleShot(0, &panel, [&panel, out] {
            const QPixmap pixmap = panel.grab();
            const bool ok = pixmap.save(out, "PNG");
            std::fprintf(stderr, ok ? "screenshot written: %s\n" : "screenshot FAILED: %s\n",
                         out.toStdString().c_str());
            QApplication::exit(ok ? 0 : 4);
        });
    }

    return QApplication::exec();
}
