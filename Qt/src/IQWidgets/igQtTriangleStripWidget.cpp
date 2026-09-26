#include <IQWidgets/igQtTriangleStripWidget.h>

#include <Convert/iGameConvertToSurfaceMeshFilter.h>
#include <DataProcessing/iGameMeshTriangulationFilter.h>
#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include <exception>
#include <iostream>
#include <stdexcept>

namespace {
QString countText(IGsize value) { return QString::number(static_cast<qulonglong>(value)); }

void require(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}

// Restore the cursor even if a filter throws. Do not process events during the
// synchronous operation: selection changes must not replace its input midway.
struct BusyCursor {
    BusyCursor() { QApplication::setOverrideCursor(Qt::WaitCursor); }
    ~BusyCursor() { QApplication::restoreOverrideCursor(); }
};
}

igQtTriangleStripWidget::igQtTriangleStripWidget(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("triangleStripPanel"));
    setMinimumWidth(390);
    setStyleSheet(QStringLiteral(
        "QWidget#triangleStripPanel { background: #1f1f1f; color: #ffffff; }"
        "QWidget#triangleStripPanel QLabel { color: #ffffff; background: transparent; }"
        "QWidget#triangleStripPanel QLabel[section=\"true\"] { font-weight: 500; padding: 6px 0; border-bottom: 1px solid #3a3a3a; }"
        "QWidget#triangleStripPanel QLabel#stripFilterName { background: #2d2d2d; border: 1px solid #3a3a3a; border-radius: 3px; padding: 8px 12px; color: #ffffff; }"
        "QSpinBox { background: #2b2b2b; color: #ffffff; border: 1px solid #4a4a4a; padding: 3px; min-height: 24px; }"
        "QSpinBox:focus { border-color: #2898ed; }"
        "QSlider::groove:horizontal { background: #484848; height: 6px; border-radius: 3px; }"
        "QSlider::sub-page:horizontal { background: #2898ed; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #168ff0; width: 12px; margin: -4px 0; border-radius: 6px; }"
        "QCheckBox { color: #ffffff; background: transparent; spacing: 6px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
        "QCheckBox::indicator:unchecked { background: #2b2b2b; border: 1px solid #909090; border-radius: 2px; }"
        "QPushButton#applyTriangleStrip { background: #1890ff; color: white; border: none; border-radius: 6px; padding: 8px 20px; }"
        "QPushButton#applyTriangleStrip:hover { background: #40a9ff; }"
        "QPushButton#applyTriangleStrip:disabled { background: #3a3a3a; color: #aaaaaa; }"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 8, 14, 16);
    layout->setSpacing(8);
    auto section = [&](const QString& text) {
        auto* label = new QLabel(text, this);
        label->setProperty("section", true);
        layout->addWidget(label);
    };

    section(QStringLiteral("模型"));
    m_SourceLabel = new QLabel(QStringLiteral("请先选择一个模型"), this);
    m_SourceLabel->setObjectName(QStringLiteral("triangleStripSource"));
    m_SourceLabel->setWordWrap(true);
    m_SourceLabel->setTextFormat(Qt::PlainText);
    layout->addWidget(m_SourceLabel);
    section(QStringLiteral("过滤"));
    auto* filterName = new QLabel(QStringLiteral("三角带转换"), this);
    filterName->setObjectName(QStringLiteral("stripFilterName"));
    layout->addWidget(filterName);
    section(QStringLiteral("属性设置"));

    auto* parameters = new QGridLayout;
    parameters->setHorizontalSpacing(12);
    parameters->setVerticalSpacing(14);
    auto* lengthLabel = new QLabel(QStringLiteral("最长三角带长度"), this);
    m_MaximumLengthSlider = new QSlider(Qt::Horizontal, this);
    m_MaximumLengthSlider->setObjectName(QStringLiteral("maximumStripLengthSlider"));
    m_MaximumLength = new QSpinBox(this);
    m_MaximumLength->setObjectName(QStringLiteral("maximumStripLength"));
    m_MaximumLengthSlider->setRange(1, 100000);
    m_MaximumLength->setRange(1, 100000);
    m_MaximumLength->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_MaximumLengthSlider->setValue(1000);
    m_MaximumLength->setValue(1000);
    m_MaximumLength->setKeyboardTracking(false);
    lengthLabel->setBuddy(m_MaximumLength);
    const QString lengthTip = QStringLiteral("单条三角带的最大三角形数（1–100000）。");
    m_MaximumLength->setToolTip(lengthTip);
    m_MaximumLengthSlider->setToolTip(lengthTip);
    parameters->addWidget(lengthLabel, 0, 0);
    parameters->addWidget(m_MaximumLengthSlider, 0, 1);
    parameters->addWidget(m_MaximumLength, 0, 2);

    auto* joinLabel = new QLabel(QStringLiteral("合并折线"), this);
    m_JoinPolyLines = new QCheckBox(this);
    m_JoinPolyLines->setObjectName(QStringLiteral("joinContiguousPolyLines"));
    m_JoinPolyLines->setAccessibleName(QStringLiteral("合并折线"));
    m_JoinPolyLines->setChecked(false);
    joinLabel->setBuddy(m_JoinPolyLines);
    m_JoinPolyLines->setToolTip(QStringLiteral("按端点 ID 合并输入中已有的连续线/折线单元。"));
    parameters->addWidget(joinLabel, 1, 0);
    parameters->addWidget(m_JoinPolyLines, 1, 1, 1, 2);
    parameters->setColumnStretch(1, 1);
    layout->addLayout(parameters);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    m_ApplyButton = new QPushButton(QStringLiteral("✓ Apply"), this);
    m_ApplyButton->setObjectName(QStringLiteral("applyTriangleStrip"));
    m_ApplyButton->setEnabled(false);
    buttons->addWidget(m_ApplyButton);
    layout->addLayout(buttons);

    m_StatusLabel = new QLabel(this);
    m_StatusLabel->setObjectName(QStringLiteral("triangleStripStatus"));
    m_StatusLabel->setWordWrap(true);
    m_StatusLabel->setTextFormat(Qt::PlainText);
    layout->addWidget(m_StatusLabel);
    section(QStringLiteral("数据统计"));
    auto* statistics = new QGridLayout;
    statistics->setVerticalSpacing(10);
    int row = 0;
    auto addStatistic = [&](const QString& title, const char* objectName) {
        auto* value = new QLabel(QStringLiteral("—"), this);
        value->setObjectName(QString::fromLatin1(objectName));
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        statistics->addWidget(new QLabel(title, this), row, 0);
        statistics->addWidget(value, row++, 1, Qt::AlignRight);
        return value;
    };
    m_TrianglesBefore = addStatistic(QStringLiteral("转换前三角形数"), "trianglesBefore");
    m_TrianglesAfter = addStatistic(QStringLiteral("转换后三角形数"), "trianglesAfter");
    m_StripCount = addStatistic(QStringLiteral("三角带数量"), "stripCount");
    m_OutputCellCount = addStatistic(QStringLiteral("输出 Cell 数"), "outputCellCount");
    m_LongestStrip = addStatistic(QStringLiteral("实际最长三角带"), "longestStrip");
    m_LineCount = addStatistic(QStringLiteral("输入线段 → 输出折线"), "polyLineCount");
    m_PointCount = addStatistic(QStringLiteral("表面点数"), "stripPointCount");
    layout->addLayout(statistics);
    layout->addStretch();

    connect(m_MaximumLengthSlider, &QSlider::valueChanged, m_MaximumLength, &QSpinBox::setValue);
    connect(m_MaximumLength, QOverload<int>::of(&QSpinBox::valueChanged), m_MaximumLengthSlider, &QSlider::setValue);
    connect(m_MaximumLength, QOverload<int>::of(&QSpinBox::valueChanged), this, &igQtTriangleStripWidget::markParametersChanged);
    connect(m_JoinPolyLines, &QCheckBox::toggled, this, &igQtTriangleStripWidget::markParametersChanged);
    connect(m_ApplyButton, &QPushButton::clicked, this, [this] { apply(); });
}

QDockWidget* igQtTriangleStripWidget::createDockWidget(QWidget* parent) {
    auto* dock = new QDockWidget(QStringLiteral("三角带转换"), parent);
    dock->setObjectName(QStringLiteral("dockWidget_TriangleStrip"));
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setFeatures(QDockWidget::DockWidgetClosable);
    auto* title = new QWidget(dock);
    title->setObjectName(QStringLiteral("triangleStripTitle"));
    title->setStyleSheet(QStringLiteral("QWidget#triangleStripTitle { background: #252525; } QLabel, QToolButton { color: white; background: transparent; border: none; font-size: 14px; font-weight: 500; } QToolButton:hover { background: #3a3a3a; }"));
    auto* titleLayout = new QHBoxLayout(title);
    titleLayout->setContentsMargins(14, 8, 10, 8);
    titleLayout->addWidget(new QLabel(QStringLiteral("三角带转换"), title));
    titleLayout->addStretch();
    auto* close = new QToolButton(title);
    close->setText(QStringLiteral("×"));
    close->setToolTip(QStringLiteral("关闭面板"));
    connect(close, &QToolButton::clicked, dock, &QDockWidget::hide);
    titleLayout->addWidget(close);
    dock->setTitleBarWidget(title);
    auto* scroll = new QScrollArea(dock);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: #1f1f1f; border: none; }"));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(new igQtTriangleStripWidget(scroll));
    dock->setWidget(scroll);
    return dock;
}

void igQtTriangleStripWidget::setInput(iGame::DataObject::Pointer input) {
    using namespace iGame;
    if (m_Input == input) { return; }
    m_Input = input;
    m_Filter = nullptr;
    m_SurfaceOutput = nullptr;
    m_PolyLineOutput = nullptr;
    clearStatistics();
    m_SourceLabel->setText(input ? QString::fromStdString(input->GetName()) : QStringLiteral("请先选择一个模型"));
    m_SourceLabel->setToolTip(m_SourceLabel->text());
    const auto type = input ? input->GetDataObjectType() : IG_NONE;
    const bool supported = type == IG_SURFACE_MESH || type == IG_UNSTRUCTURED_MESH ||
                           type == IG_VOLUME_MESH || type == IG_STRUCTURED_MESH;
    m_ApplyButton->setEnabled(supported);
    showStatus(!input ? QStringLiteral("请先选择一个模型。") : supported ? QStringLiteral("设置参数后点击 Apply。") : QStringLiteral("请选择表面网格或体网格；当前数据类型不支持。"), input && !supported);
}

bool igQtTriangleStripWidget::isOutput(iGame::DataObject* object) const {
    return object && (object == m_SurfaceOutput.get() || object == m_PolyLineOutput.get());
}

void igQtTriangleStripWidget::clearStatistics() {
    for (auto* label : {m_TrianglesBefore, m_TrianglesAfter, m_StripCount,
                        m_OutputCellCount, m_LongestStrip, m_LineCount,
                        m_PointCount}) {
        label->setText(QStringLiteral("—"));
    }
}

void igQtTriangleStripWidget::showStatus(const QString& message, bool error) {
    m_StatusLabel->setText(message);
    m_StatusLabel->setStyleSheet(error ? QStringLiteral("color: #ff8080;") : QStringLiteral("color: #ffffff;"));
}

void igQtTriangleStripWidget::markParametersChanged() {
    if (m_Input) { showStatus(QStringLiteral("参数已修改，统计为上次结果。")); }
}

bool igQtTriangleStripWidget::apply() {
    if (!m_Input || !m_ApplyButton->isEnabled()) { return false; }
    BusyCursor cursor;
    using namespace iGame;
    try {
        // vtkStripper operates on line cells already present in the input; it
        // does not manufacture lines from polygon boundary edges. Preserve
        // explicit lines while the surface portion is extracted/triangulated.
        auto explicitLines = CellArray::New();
        auto explicitLineTypes = UnsignedIntArray::New();
        bool hasVolumeCells = false;
        if (auto unstructured = DynamicCast<UnstructuredMesh>(m_Input)) {
            for (IGsize i = 0; i < unstructured->GetNumberOfCells(); ++i) {
                const IGenum cellType = unstructured->GetCellType(i);
                hasVolumeCells = hasVolumeCells ||
                        Cell::GetCellDimension(cellType) >= 3;
                if (cellType != IG_LINE && cellType != IG_POLY_LINE) continue;
                const igIndex* pointIds = nullptr;
                const int pointCount = unstructured->GetCells()->GetCellIds(
                        i, pointIds);
                require(pointIds && pointCount >= 2,
                        "输入中存在无效的线/折线单元。");
                explicitLines->AddCellIds(pointIds, pointCount);
                explicitLineTypes->AddValue(cellType);
            }
        }
        auto extract = ConvertToSurfaceMeshFilter::New();
        extract->SetInput(m_Input);
        extract->SetConvertMethod(
                explicitLines->GetNumberOfCells() > 0 && !hasVolumeCells
                        ? ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_CELL
                        : ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_MESH);
        require(extract->Execute(), "表面提取失败，请检查输入网格。");
        auto surface = DynamicCast<SurfaceMesh>(extract->GetOutput());
        require(surface && surface->GetNumberOfFaces() > 0, "输入中没有可处理的表面。");

        auto triangulate = MeshTriangulationFilter::New();
        triangulate->SetInput(surface);
        require(triangulate->Execute(), "表面三角化失败，请检查退化或无效多边形。");
        auto triangles = DynamicCast<SurfaceMesh>(triangulate->GetOutput());
        require(triangles && triangles->GetNumberOfFaces() > 0, "三角化结果为空。");
        for (IGsize i = 0; i < triangles->GetNumberOfFaces(); ++i) {
            require(triangles->GetFaces()->GetCellSize(i) == 3, "三角化结果中仍有非三角形面。");
        }

        DataObject::Pointer filterInput = triangles;
        if (explicitLines->GetNumberOfCells() > 0) {
            auto mixedInput = UnstructuredMesh::New();
            auto cells = CellArray::New();
            auto types = UnsignedIntArray::New();
            for (IGsize i = 0; i < triangles->GetNumberOfFaces(); ++i) {
                const igIndex* pointIds = nullptr;
                const int pointCount = triangles->GetFaces()->GetCellIds(
                        i, pointIds);
                cells->AddCellIds(pointIds, pointCount);
                types->AddValue(IG_TRIANGLE);
            }
            for (IGsize i = 0; i < explicitLines->GetNumberOfCells(); ++i) {
                const igIndex* pointIds = nullptr;
                const int pointCount = explicitLines->GetCellIds(i, pointIds);
                cells->AddCellIds(pointIds, pointCount);
                types->AddValue(explicitLineTypes->GetValue(i));
            }
            mixedInput->SetName(triangles->GetName());
            mixedInput->SetPoints(triangles->GetPoints());
            mixedInput->SetCells(cells, types);
            mixedInput->SetAttributeSet(
                    AttributeSet::Pointer(triangles->GetAttributeSet()));
            filterInput = mixedInput;
        }

        auto filter = TriangleStripFilter::New();
        filter->SetInput(filterInput);
        filter->SetMaximumLength(m_MaximumLength->value());
        filter->SetJoinContiguousSegments(m_JoinPolyLines->isChecked());
        require(filter->Execute(), "三角带转换失败，请检查网格拓扑。");
        auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
        require(output != nullptr, "过滤器没有生成表面结果。");
        IGsize triangleCount = 0;
        for (IGsize i = 0; i < filter->GetNumberOfStrips(); ++i) {
            const auto size = filter->GetStrips()->GetCellSize(i);
            require(size >= 3, "过滤器生成了无效三角带。");
            triangleCount += size - 2;
        }
        require(triangleCount == triangles->GetNumberOfFaces() && triangleCount == output->GetNumberOfFaces(),
                "转换前后三角形数量不一致，未添加结果模型。");
        require(filter->GetLongestStripLength() <= m_MaximumLength->value(), "三角带长度超过设定上限。");

        // Lines are not part of SurfaceMesh::Faces. Publish a separate drawable
        // UnstructuredMesh, preserving each polyline as one IG_POLY_LINE cell.
        auto* lines = filter->GetPolyLines();
        IGsize segmentCount = 0;
        UnstructuredMesh::Pointer lineOutput;
        if (lines && lines->GetNumberOfCells() > 0) {
            lineOutput = UnstructuredMesh::New();
            lineOutput->SetPoints(output->GetPoints());
            auto types = UnsignedIntArray::New();
            for (IGsize i = 0; i < lines->GetNumberOfCells(); ++i) {
                const int size = lines->GetCellSize(i);
                require(size >= 2, "过滤器生成了无效折线。");
                segmentCount += size - 1;
                types->AddValue(size == 2 ? IG_LINE : IG_POLY_LINE);
            }
            lineOutput->SetCells(lines, types);
            lineOutput->SetName(m_Input->GetName() + "_TriangleStrip_PolyLines");
            lineOutput->SetShellRenderingOption(false);
            lineOutput->SetViewStyle(IG_WIREFRAME);
            lineOutput->SetLineWidth(2.5f);
            lineOutput->SetLineColor(igm::vec3(0.1f, 0.56f, 1.0f));
        }
        output->SetName(m_Input->GetName() + "_TriangleStrips");
        m_Filter = filter;
        m_SurfaceOutput = output;
        m_PolyLineOutput = lineOutput;
        m_TrianglesBefore->setText(countText(triangles->GetNumberOfFaces()));
        m_TrianglesAfter->setText(countText(triangleCount));
        m_StripCount->setText(countText(filter->GetNumberOfStrips()));
        m_OutputCellCount->setText(countText(filter->GetNumberOfOutputCells()));
        m_LongestStrip->setText(countText(filter->GetLongestStripLength()));
        m_LineCount->setText(countText(segmentCount) + QStringLiteral(" → ") + countText(lines ? lines->GetNumberOfCells() : 0));
        m_PointCount->setText(countText(output->GetNumberOfPoints()));
        showStatus(QStringLiteral("转换完成。"));
        std::cout << "[TriangleStrip] Triangles before: " << triangles->GetNumberOfFaces()
                  << ", after: " << triangleCount << ", strips: " << filter->GetNumberOfStrips()
                  << ", output cells (ParaView): " << filter->GetNumberOfOutputCells()
                  << ", longest: " << filter->GetLongestStripLength()
                  << ", input line segments: " << segmentCount
                  << ", polylines: " << (lines ? lines->GetNumberOfCells() : 0) << '\n';
        Q_EMIT resultReady(output, lineOutput);
        return true;
    } catch (const std::exception& error) {
        showStatus(QString::fromUtf8(error.what()), true);
        return false;
    }
}
