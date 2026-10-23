#include "IQWidgets/igQtHistogramChartWidget.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFontMetricsF>
#include <QFormLayout>
#include <QGraphicsLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QPolygonF>
#include <QPushButton>
#include <QResizeEvent>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QLegend>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <algorithm>
#include <cmath>
#include <iGameAttributeSet.h>
#include <iGamePoints.h>
#include <limits>

namespace
{
const QColor kColors[] = {QColor("#000000"), QColor("#E41A1C"), QColor("#377EB8"), QColor("#4DAF4A"),
                          QColor("#984EA3"), QColor("#FF7F00"), QColor("#A65628"), QColor("#F781BF")};

double TickInterval(double span, double pixels, double spacing) {
    const double raw = span / std::max(2.0, std::floor(pixels / spacing));
    if (!std::isfinite(raw) || raw <= 0) return 1;
    const double scale = std::pow(10.0, std::floor(std::log10(raw)));
    const double value = raw / scale;
    return scale * (value < 1.5 ? 1 : value < 3 ? 2 : value < 7 ? 5 : 10);
}

void SetAxisView(QValueAxis* axis, double minimum, double maximum, const QSize& size) {
    const bool horizontal = axis->orientation() == Qt::Horizontal;
    const double interval =
            TickInterval(maximum - minimum, horizontal ? size.width() - 55 : size.height() - 35, horizontal ? 100 : 40);
    const double anchor = std::floor(minimum / interval) * interval;
    // Qt uses integer tick offsets. Keep the anchor near the visible range and update in fixed
    // mode so neither a tiny interval on the old range nor a distant anchor can flood the layout.
    axis->setTickType(QValueAxis::TicksFixed);
    axis->setTickAnchor(std::isfinite(anchor) ? anchor : minimum);
    axis->setTickInterval(interval);
    axis->setLabelFormat("%.12g");
    axis->setRange(minimum, maximum);
    if (!horizontal && minimum < 0 && maximum > 0) {
        const int decimals = std::clamp(static_cast<int>(-std::floor(std::log10(interval))), 0, 12);
        axis->setLabelFormat(interval >= 1e-12 && interval < 1e7 ? QStringLiteral("%1.%2f").arg('%').arg(decimals)
                                                                 : QStringLiteral("%.12g"));
    }
    axis->setTickType(QValueAxis::TicksDynamic);
}

void UpdateTicks(QChart* chart, const QSize& size) {
    for (auto* item: chart->axes())
        if (auto* axis = qobject_cast<QValueAxis*>(item)) SetAxisView(axis, axis->min(), axis->max(), size);
}

void DrawDenseBars(QPainter* painter, QChart* chart, const QRectF& plot) {
    auto* axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).value(0));
    auto* axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).value(0));
    if (!axisX || !axisY || plot.isEmpty()) return;
    const double xScale = plot.width() / (axisX->max() - axisX->min());
    const double yScale = plot.height() / (axisY->max() - axisY->min());
    const double zero = plot.bottom() + axisY->min() * yScale;
    const double dpr = painter->device()->devicePixelRatioF();
    painter->save();
    painter->setClipRect(plot, Qt::IntersectClip);
    for (auto* series: chart->series()) {
        auto* bars = qobject_cast<QAreaSeries*>(series);
        if (!bars || !bars->isVisible()) continue;
        const auto points = bars->property("denseBarPoints").value<QPolygonF>();
        if (points.isEmpty()) continue;
        const double width = bars->property("denseBarWidth").toDouble() * xScale;
        const double offset = bars->property("denseBarOffset").toDouble();
        if (width * dpr >= 1) {
            QVector<QRectF> rectangles;
            for (const auto& point: points) {
                const double x = plot.left() + (point.x() + offset - axisX->min()) * xScale;
                if (x + width / 2 < plot.left() || x - width / 2 > plot.right()) continue;
                const double y = zero - point.y() * yScale;
                rectangles.append(QRectF(x - width * 0.46, std::min(y, zero), width * 0.92, std::abs(y - zero)));
            }
            painter->setBrush(bars->color());
            painter->setPen(QPen(bars->color().darker(150), 0.6));
            painter->drawRects(rectangles.constData(), rectangles.size());
        } else {
            // Subpixel bars still originate at zero. Keep both signs in each device pixel;
            // averaging or connecting successive values would change the visible distribution.
            struct Extents {
                double positive = 0, negative = 0;
                bool used = false;
            };
            std::vector<Extents> columns(static_cast<size_t>(std::ceil(plot.width() * dpr)) + 1);
            for (const auto& point: points) {
                const double x = (point.x() + offset - axisX->min()) * xScale;
                if (!std::isfinite(x) || x + width * 0.46 < 0 || x - width * 0.46 > plot.width()) continue;
                const int left = static_cast<int>(std::floor((x - width * 0.46) * dpr));
                const int right = static_cast<int>(std::floor((x + width * 0.46) * dpr));
                if (right < 0 || left >= static_cast<int>(columns.size())) continue;
                for (int pixel = std::max(0, left); pixel <= std::min(right, static_cast<int>(columns.size()) - 1);
                     ++pixel) {
                    auto& column = columns[pixel];
                    column.positive = std::max(column.positive, point.y());
                    column.negative = std::min(column.negative, point.y());
                    column.used = true;
                }
            }
            QVector<QLineF> strokes;
            for (size_t pixel = 0; pixel < columns.size(); ++pixel) {
                if (!columns[pixel].used) continue;
                const double x = plot.left() + (pixel + 0.5) / dpr;
                strokes.append(
                        QLineF(x, zero - columns[pixel].positive * yScale, x, zero - columns[pixel].negative * yScale));
            }
            painter->setRenderHint(QPainter::Antialiasing, false);
            QPen pen(bars->color(), 1 / dpr);
            pen.setCapStyle(Qt::FlatCap);
            painter->setPen(pen);
            painter->drawLines(strokes.constData(), strokes.size());
        }
    }
    painter->restore();
}

// Draw the legend over the plot so it never reserves a separate column beside the data.
class HistogramChartView final : public QChartView {
public:
    explicit HistogramChartView(QChart* chart, QWidget* parent) : QChartView(chart, parent) { setMouseTracking(true); }

    void captureDefaultView() {
        m_DefaultRanges.clear();
        for (auto* axis: chart()->axes())
            if (auto* valueAxis = qobject_cast<QValueAxis*>(axis))
                m_DefaultRanges.push_back({valueAxis, valueAxis->min(), valueAxis->max()});
        m_DragButton = Qt::NoButton;
        unsetCursor();
    }

protected:
    void wheelEvent(QWheelEvent* event) override {
        const QRectF plot = chart()->plotArea();
        const QPointF position = chart()->mapFromScene(mapToScene(event->pos()));
        if (!plot.contains(position) || plot.isEmpty()) {
            event->ignore();
            return;
        }
        const double steps =
                event->angleDelta().y() != 0 ? event->angleDelta().y() / 120.0 : event->pixelDelta().y() / 120.0;
        if (steps == 0) {
            event->ignore();
            return;
        }
        const double scale = std::pow(1.2, -std::clamp(steps, -8.0, 8.0));
        std::vector<Range> ranges;
        for (const auto& original: m_DefaultRanges) {
            if (!original.axis) continue;
            auto* axis = original.axis.data();
            const double span = axis->max() - axis->min();
            const double defaultSpan = original.maximum - original.minimum;
            // Bound magnification and numeric precision so extreme wheel input stays responsive.
            if (span * scale < defaultSpan * 1e-9 || span * scale > defaultSpan * 1e6) {
                event->accept();
                return;
            }
            const double fraction = axis->orientation() == Qt::Horizontal
                                            ? (position.x() - plot.left()) / plot.width()
                                            : (plot.bottom() - position.y()) / plot.height();
            const double anchor = axis->min() + fraction * span;
            ranges.push_back({axis, anchor + (axis->min() - anchor) * scale, anchor + (axis->max() - anchor) * scale});
        }
        applyRanges(ranges);
        event->accept();
    }

    void mousePressEvent(QMouseEvent* event) override {
        if ((event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) &&
            chart()->plotArea().contains(chart()->mapFromScene(mapToScene(event->pos())))) {
            m_DragButton = event->button();
            m_LastPosition = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
        QChartView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        const QRectF plot = chart()->plotArea();
        if (m_DragButton != Qt::NoButton && event->buttons().testFlag(m_DragButton) && !plot.isEmpty()) {
            const QPoint delta = event->pos() - m_LastPosition;
            std::vector<Range> ranges;
            for (auto* item: chart()->axes()) {
                auto* axis = qobject_cast<QValueAxis*>(item);
                if (!axis) continue;
                const double shift = (axis->orientation() == Qt::Horizontal ? -delta.x() / plot.width()
                                                                            : delta.y() / plot.height()) *
                                     (axis->max() - axis->min());
                ranges.push_back({axis, axis->min() + shift, axis->max() + shift});
            }
            applyRanges(ranges);
            m_LastPosition = event->pos();
            event->accept();
            return;
        }
        m_DragButton = Qt::NoButton;
        if (plot.contains(chart()->mapFromScene(mapToScene(event->pos())))) setCursor(Qt::OpenHandCursor);
        else
            unsetCursor();
        QChartView::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == m_DragButton) {
            m_DragButton = Qt::NoButton;
            unsetCursor();
            event->accept();
            return;
        }
        QChartView::mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton &&
            chart()->plotArea().contains(chart()->mapFromScene(mapToScene(event->pos())))) {
            m_DragButton = Qt::NoButton;
            unsetCursor();
            applyRanges(m_DefaultRanges);
            event->accept();
            return;
        }
        QChartView::mouseDoubleClickEvent(event);
    }

    void resizeEvent(QResizeEvent* event) override {
        QChartView::resizeEvent(event);
        UpdateTicks(chart(), size());
    }

    void drawForeground(QPainter* painter, const QRectF& rect) override {
        QChartView::drawForeground(painter, rect);
        const QRectF plot = chart()->mapRectToScene(chart()->plotArea());
        DrawDenseBars(painter, chart(), plot);
        auto* yAxis = qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).value(0));
        if (yAxis && yAxis->min() < 0 && yAxis->max() > 0) {
            const double zero = plot.bottom() + yAxis->min() / (yAxis->max() - yAxis->min()) * plot.height();
            painter->save();
            painter->setPen(QPen(Qt::black, 0.8));
            painter->drawLine(QLineF(plot.left(), zero, plot.right(), zero));
            painter->setFont(QFont(QStringLiteral("Arial"), 9));
            const double height = QFontMetricsF(painter->font()).height() + 2;
            const QRectF label(0, zero - height / 2, plot.left() - 5, height);
            painter->fillRect(label, Qt::white);
            painter->drawText(label, Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("0"));
            painter->restore();
        }
        struct Entry {
            QString name;
            QColor color;
        };
        std::vector<Entry> entries;
        for (auto* series: chart()->series()) {
            if (!series->isVisible()) continue;
            const auto* bars = qobject_cast<QAreaSeries*>(series);
            const auto* line = qobject_cast<QLineSeries*>(series);
            if (bars || line) entries.push_back({series->name(), bars ? bars->color() : line->color()});
        }
        if (entries.empty()) return;
        std::stable_sort(entries.begin(), entries.end(),
                         [](const auto& a, const auto& b) { return a.name.compare(b.name, Qt::CaseInsensitive) < 0; });
        const QFont font(QStringLiteral("Arial"), 9);
        const QFontMetricsF metrics(font);
        const double rowHeight = metrics.height() + 2;
        double textWidth = 0;
        for (const auto& entry: entries) textWidth = std::max(textWidth, metrics.horizontalAdvance(entry.name));
        const double width = std::min(textWidth + 35, plot.width() - 12);
        const int rows = std::min(static_cast<int>(entries.size()), static_cast<int>((plot.height() - 20) / rowHeight));
        if (width < 45 || rows < 1) return;
        const QRectF box(plot.right() - width, plot.top(), width, rows * rowHeight + 8);
        painter->save();
        painter->setFont(font);
        painter->setPen(QPen(Qt::black, 0.7));
        painter->setBrush(Qt::white);
        painter->drawRect(box);
        for (int i = 0; i < rows; ++i) {
            const double y = box.top() + 4 + i * rowHeight;
            painter->setPen(QPen(QColor("#333333"), 0.6));
            painter->setBrush(entries[i].color);
            painter->drawRect(QRectF(box.left() + 5, y + (rowHeight - 11) / 2, 20, 11));
            painter->setPen(Qt::black);
            const QString name = i == rows - 1 && rows < static_cast<int>(entries.size())
                                         ? QStringLiteral("… (+%1)").arg(entries.size() - rows + 1)
                                         : entries[i].name;
            painter->drawText(QRectF(box.left() + 28, y, width - 32, rowHeight), Qt::AlignVCenter | Qt::AlignLeft,
                              metrics.elidedText(name, Qt::ElideRight, width - 32));
        }
        painter->restore();
    }

private:
    struct Range {
        QPointer<QValueAxis> axis;
        double minimum;
        double maximum;
    };

    void applyRanges(const std::vector<Range>& ranges) {
        for (const auto& range: ranges) {
            const double span = range.maximum - range.minimum;
            const double precision = std::max(std::abs(range.minimum), std::abs(range.maximum)) *
                                     std::numeric_limits<double>::epsilon() * 32;
            if (!range.axis || !std::isfinite(range.minimum) || !std::isfinite(range.maximum) || !std::isfinite(span) ||
                span <= precision)
                return;
        }
        for (const auto& range: ranges) {
            auto* axis = range.axis.data();
            SetAxisView(axis, range.minimum, range.maximum, size());
        }
        viewport()->update();
    }

    std::vector<Range> m_DefaultRanges;
    Qt::MouseButton m_DragButton = Qt::NoButton;
    QPoint m_LastPosition;
};
double ValueAt(const iGame::ArrayObject::Pointer& array, IGsize index, int component) {
    if (component < array->GetDimension()) return array->GetElementValue(index, component);
    double value = 0;
    for (int c = 0; c < array->GetDimension(); ++c) value = std::hypot(value, array->GetElementValue(index, c));
    return value;
}
} // namespace

igQtHistogramChartWidget::igQtHistogramChartWidget(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("histogramChart"));
    m_Controls = new QWidget(this);
    auto* controls = m_Controls;
    controls->setObjectName(QStringLiteral("histogramSettings"));
    controls->hide();
    auto* form = new QFormLayout();
    m_Source = new QComboBox(controls);
    m_Source->setObjectName(QStringLiteral("histogramSource"));
    m_Source->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_Source->setMinimumContentsLength(12);
    m_Attachment = new QComboBox(controls);
    m_Attachment->setObjectName(QStringLiteral("histogramAttachment"));
    m_Attachment->addItem(QStringLiteral("Point Data"), IG_POINT);
    m_Attachment->addItem(QStringLiteral("Cell Data"), IG_CELL);
    m_UseIndex = new QCheckBox(QStringLiteral("Use Index for X Axis"), controls);
    m_UseIndex->setObjectName(QStringLiteral("histogramUseIndex"));
    m_XArray = new QComboBox(controls);
    m_XArray->setObjectName(QStringLiteral("histogramXArray"));
    m_XArray->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_XArray->setMinimumContentsLength(12);
    form->addRow(QStringLiteral("数据源"), m_Source);
    form->addRow(QStringLiteral("Attribute Type"), m_Attachment);
    form->addRow(m_UseIndex);
    form->addRow(QStringLiteral("X Array Name"), m_XArray);
    m_Series = new QTableWidget(controls);
    m_Series->setObjectName(QStringLiteral("histogramSeries"));
    m_Series->setColumnCount(4);
    m_Series->setHorizontalHeaderLabels({QStringLiteral("显示"), QStringLiteral("颜色"), QStringLiteral("Variable"),
                                         QStringLiteral("Legend Name")});
    m_Series->verticalHeader()->hide();
    for (int i = 0; i < 3; ++i) m_Series->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    m_Series->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_Series->setMinimumWidth(300);
    auto* buttons = new QHBoxLayout();
    for (bool visible: {true, false}) {
        auto* button = new QPushButton(visible ? QStringLiteral("全选") : QStringLiteral("全不选"), controls);
        buttons->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, visible]() {
            const int index = m_Source->currentIndex();
            if (index < 0) return;
            auto& representation = m_Representations[index];
            for (auto& column: representation.columns)
                if (column.attachment == representation.attachment) column.visible = visible;
            RefreshControls();
            Redraw();
        });
    }
    auto* controlLayout = new QVBoxLayout(controls);
    controlLayout->addLayout(form);
    controlLayout->addWidget(new QLabel(QStringLiteral("Series Parameters"), controls));
    controlLayout->addWidget(m_Series);
    controlLayout->addLayout(buttons);
    auto* chart = new QChart();
    chart->setBackgroundBrush(Qt::white);
    chart->setBackgroundPen(Qt::NoPen);
    chart->setBackgroundRoundness(0);
    chart->setMargins(QMargins(10, 14, 16, 8));
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(Qt::white);
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::NoAnimation);
    m_ChartView = new HistogramChartView(chart, this);
    m_ChartView->setFrameShape(QFrame::NoFrame);
    m_ChartView->setBackgroundBrush(Qt::white);
    m_ChartView->setObjectName(QStringLiteral("histogramPlot"));
    m_ChartView->setRenderHint(QPainter::Antialiasing);
    m_ChartView->setMinimumSize(280, 240);
    m_ChartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_ChartView, 1);

    connect(m_Source, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        if (!m_Refreshing) RefreshControls();
    });
    connect(m_Attachment, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (m_Refreshing || m_Source->currentIndex() < 0) return;
        m_Representations[m_Source->currentIndex()].attachment = m_Attachment->itemData(index).toInt();
        RefreshControls();
        Redraw();
    });
    connect(m_UseIndex, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_Refreshing || m_Source->currentIndex() < 0) return;
        m_Representations[m_Source->currentIndex()].useIndex = checked;
        m_XArray->setEnabled(!checked);
        Redraw();
    });
    connect(m_XArray, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (m_Refreshing || m_Source->currentIndex() < 0 || index < 0) return;
        m_Representations[m_Source->currentIndex()].xColumn = m_XArray->itemData(index).toString();
        Redraw();
    });
    connect(m_Series, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (m_Refreshing || m_Source->currentIndex() < 0) return;
        auto& column = m_Representations[m_Source->currentIndex()]
                               .columns[m_Series->item(item->row(), 0)->data(Qt::UserRole).toInt()];
        if (item->column() == 0) column.visible = item->checkState() == Qt::Checked;
        if (item->column() == 3) column.legend = item->text();
        Redraw();
    });
    connect(m_Series, &QTableWidget::cellDoubleClicked, this, [this](int row, int cell) {
        if (cell != 1 || m_Source->currentIndex() < 0) return;
        const int columnIndex = m_Series->item(row, 0)->data(Qt::UserRole).toInt();
        const int sourceIndex = m_Source->currentIndex();
        const QColor color = QColorDialog::getColor(m_Representations[sourceIndex].columns[columnIndex].color, this,
                                                    QStringLiteral("系列颜色"));
        if (!color.isValid()) return;
        m_Representations[sourceIndex].columns[columnIndex].color = color;
        RefreshControls();
        Redraw();
    });
}

void igQtHistogramChartWidget::AddColumns(Representation& representation, iGame::ArrayObject::Pointer array,
                                          IGenum attachment) {
    if (!array || array->GetDimension() < 1) return;
    const int dimension = array->GetDimension();
    auto add = [&](int component, const QString& suffix) {
        Column column;
        column.array = array;
        column.component = component;
        column.attachment = attachment;
        column.name = QString::fromStdString(array->GetName()) + suffix;
        column.legend = column.name;
        const size_t colorIndex = std::count_if(
                representation.columns.begin(), representation.columns.end(), [attachment](const auto& item) {
                    return item.attachment == attachment && item.array->GetName() != "Points";
                });
        column.color = kColors[colorIndex % (sizeof(kColors) / sizeof(kColors[0]))];
        if (array->GetName() == "Points") {
            column.color = component == dimension ? QColor("#FF7F00")
                           : component == 0       ? QColor("#A65628")
                           : component == 1       ? Qt::black
                                                  : QColor("#E41A1C");
        }
        representation.columns.push_back(column);
    };
    if (dimension > 1) add(dimension, QStringLiteral("_Magnitude"));
    for (int c = 0; c < dimension; ++c) {
        const QString suffix = dimension == 1 ? QString()
                                              : (c < 3 ? QStringLiteral("_%1").arg(QStringLiteral("XYZ")[c])
                                                       : QStringLiteral("_%1").arg(c));
        add(c, suffix);
    }
}

void igQtHistogramChartWidget::SetHistogram(iGame::DataObject::Pointer input, iGame::HistogramData::Pointer result,
                                            int attributeIndex, int component, iGame::HistogramData* previousResult) {
    if (!input || !result) return;
    const auto source = std::find_if(m_Representations.begin(), m_Representations.end(),
                                     [&](const auto& item) { return item.object.get() == input.get(); });
    if (source == m_Representations.end()) {
        Representation representation;
        representation.object = input;
        representation.name = QString::fromStdString(input->GetName());
        representation.attachment = result->GetSourceAttachmentType();
        // Coordinates are series too, matching the source representation's Points_* entries.
        if (auto points = input->GetPoints(); points && points->GetNumberOfPoints() > 0) {
            auto coordinates = iGame::DoubleArray::New();
            coordinates->SetName("Points");
            coordinates->SetDimension(3);
            for (IGsize i = 0; i < points->GetNumberOfPoints(); ++i) {
                const auto& point = points->GetPoint(i);
                for (int c = 0; c < 3; ++c) coordinates->AddValue(point[c]);
            }
            AddColumns(representation, coordinates, IG_POINT);
        }
        auto* attributes = input->GetAttributeSet();
        for (size_t i = 0; attributes && i < attributes->GetNumberOfAttributes(); ++i) {
            auto& attribute = attributes->GetAttribute(i);
            if (!attribute.isDeleted && attribute.pointer &&
                (attribute.attachmentType == IG_POINT || attribute.attachmentType == IG_CELL))
                AddColumns(representation, attribute.pointer, attribute.attachmentType);
        }
        if (attributes && attributeIndex >= 0 &&
            static_cast<size_t>(attributeIndex) < attributes->GetNumberOfAttributes()) {
            const auto selected = attributes->GetAttribute(attributeIndex).pointer;
            for (const auto& column: representation.columns)
                if (column.array == selected && column.component == component) representation.xColumn = column.name;
        }
        m_Representations.push_back(std::move(representation));
    }
    Representation output;
    output.object = result;
    output.histogram = true;
    output.useIndex = false;
    output.xColumn = QStringLiteral("bin_extents");
    AddColumns(output, result->GetBinExtents(), IG_POINT);
    AddColumns(output, result->GetBinValues(), IG_POINT);
    AddColumns(output, result->GetBinAccumulation(), IG_POINT);
    for (const auto& pair: result->GetAverageColumns()) {
        AddColumns(output, pair.first, IG_POINT);
        AddColumns(output, pair.second, IG_POINT);
    }
    for (auto& column: output.columns) {
        column.visible = column.name == QStringLiteral("bin_values");
        if (column.visible) column.color = QColor("#E41A1C");
    }
    auto previous = std::find_if(m_Representations.begin(), m_Representations.end(), [&](const auto& item) {
        return item.histogram && item.object.get() == previousResult;
    });
    int outputIndex;
    m_Refreshing = true;
    if (previous != m_Representations.end()) {
        outputIndex = static_cast<int>(previous - m_Representations.begin());
        output.name = previous->name;
        output.useIndex = previous->useIndex;
        for (auto& column: output.columns) {
            for (const auto& old: previous->columns) {
                if (old.name == column.name) {
                    column.visible = old.visible;
                    column.color = old.color;
                    column.legend = old.legend;
                }
            }
        }
        if (std::any_of(output.columns.begin(), output.columns.end(),
                        [&](const auto& column) { return column.name == previous->xColumn; }))
            output.xColumn = previous->xColumn;
        *previous = std::move(output);
    } else {
        outputIndex = static_cast<int>(m_Representations.size());
        output.name =
                QStringLiteral("Histogram%1 — %2").arg(++m_ResultCount).arg(QString::fromStdString(input->GetName()));
        m_Representations.push_back(std::move(output));
    }
    m_Source->clear();
    for (const auto& representation: m_Representations) m_Source->addItem(representation.name);
    m_Source->setCurrentIndex(outputIndex);
    m_Refreshing = false;
    RefreshControls();
    Redraw();
}

void igQtHistogramChartWidget::SelectSource(iGame::DataObject* object) {
    for (size_t i = 0; i < m_Representations.size(); ++i) {
        if (m_Representations[i].object.get() == object) {
            m_Source->setCurrentIndex(static_cast<int>(i));
            return;
        }
    }
}

void igQtHistogramChartWidget::RefreshControls() {
    const int index = m_Source->currentIndex();
    if (index < 0) return;
    m_Refreshing = true;
    auto& representation = m_Representations[index];
    m_Attachment->setEnabled(!representation.histogram);
    m_Attachment->setCurrentIndex(representation.attachment == IG_CELL ? 1 : 0);
    m_UseIndex->setChecked(representation.useIndex);
    m_XArray->clear();
    m_Series->setRowCount(0);
    for (size_t i = 0; i < representation.columns.size(); ++i) {
        const auto& column = representation.columns[i];
        if (column.attachment != representation.attachment) continue;
        m_XArray->addItem(column.name, column.name);
        const int row = m_Series->rowCount();
        m_Series->insertRow(row);
        auto* check = new QTableWidgetItem();
        check->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        check->setCheckState(column.visible ? Qt::Checked : Qt::Unchecked);
        check->setData(Qt::UserRole, static_cast<int>(i));
        m_Series->setItem(row, 0, check);
        auto* color = new QTableWidgetItem();
        color->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        color->setBackground(column.color);
        color->setToolTip(QStringLiteral("双击修改颜色"));
        m_Series->setItem(row, 1, color);
        auto* name = new QTableWidgetItem(column.name);
        name->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_Series->setItem(row, 2, name);
        m_Series->setItem(row, 3, new QTableWidgetItem(column.legend));
    }
    int xIndex = m_XArray->findData(representation.xColumn);
    if (xIndex < 0 && m_XArray->count() > 0) {
        xIndex = 0;
        representation.xColumn = m_XArray->itemData(0).toString();
    }
    m_XArray->setCurrentIndex(xIndex);
    m_XArray->setEnabled(!representation.useIndex);
    m_Refreshing = false;
}

void igQtHistogramChartWidget::Redraw() {
    auto* chart = m_ChartView->chart();
    chart->removeAllSeries();
    for (auto* axis: chart->axes()) {
        chart->removeAxis(axis);
        delete axis;
    }
    auto* axisX = new QValueAxis();
    auto* axisY = new QValueAxis();
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    double minX = std::numeric_limits<double>::infinity(), maxX = -minX;
    double minY = 0, maxY = 0;
    int visibleCount = 0;
    size_t skipped = 0;
    QStringList xNames;
    int slotCount = 0;
    for (const auto& representation: m_Representations)
        slotCount += static_cast<int>(
                std::count_if(representation.columns.begin(), representation.columns.end(), [&](const auto& column) {
                    return column.visible && column.attachment == representation.attachment;
                }));
    for (const auto& representation: m_Representations) {
        const Column* xColumn = nullptr;
        for (const auto& column: representation.columns)
            if (column.attachment == representation.attachment && column.name == representation.xColumn) {
                xColumn = &column;
                break;
            }
        if (!representation.useIndex && !xColumn) continue;
        const int groupCount = static_cast<int>(
                std::count_if(representation.columns.begin(), representation.columns.end(), [&](const auto& column) {
                    return column.visible && column.attachment == representation.attachment;
                }));
        int group = 0;
        for (const auto& column: representation.columns) {
            if (!column.visible || column.attachment != representation.attachment) continue;
            IGsize count = column.array->GetNumberOfValues() / column.array->GetDimension();
            if (!representation.useIndex)
                count = std::min(count, xColumn->array->GetNumberOfValues() / xColumn->array->GetDimension());
            if (count == 0) continue;
            QVector<QPointF> points;
            points.reserve(static_cast<int>(count));
            for (IGsize i = 0; i < count; ++i) {
                const double x = representation.useIndex ? static_cast<double>(i)
                                                         : ValueAt(xColumn->array, i, xColumn->component);
                const double y = ValueAt(column.array, i, column.component);
                if (!std::isfinite(x) || !std::isfinite(y)) {
                    ++skipped;
                    continue;
                }
                points.append(QPointF(x, y));
            }
            if (points.isEmpty()) continue;
            // Both representations use numeric X coordinates, preserving bin centers and source indices.
            std::stable_sort(points.begin(), points.end(), [](const auto& a, const auto& b) { return a.x() < b.x(); });
            double spacing = std::numeric_limits<double>::infinity();
            for (int i = 1; i < points.size(); ++i) {
                const double delta = points[i].x() - points[i - 1].x();
                if (delta > 0) spacing = std::min(spacing, delta);
            }
            if (!std::isfinite(spacing)) spacing = 1;
            const bool scalar = !representation.histogram && column.array->GetDimension() == 1;
            const double width = scalar ? spacing * 0.6 : spacing * 0.8 / std::max(1, slotCount);
            const double offset = scalar ? 0 : (group - (groupCount - 1) * 0.5) * width;
            const bool dense = points.size() > 1000;
            auto* upper = new QLineSeries();
            auto* lower = new QLineSeries();
            auto* bars = new QAreaSeries(upper, lower);
            if (dense) {
                bars->setProperty("denseBarPoints", QVariant::fromValue(QPolygonF(points)));
                bars->setProperty("denseBarWidth", width);
                bars->setProperty("denseBarOffset", offset);
                upper->append(points.first().x(), 0);
                upper->append(points.last().x(), 0);
                lower->append(points.first().x(), 0);
                lower->append(points.last().x(), 0);
                minX = std::min(minX, points.first().x() + offset - width * 0.46);
                maxX = std::max(maxX, points.last().x() + offset + width * 0.46);
            } else {
                for (const auto& point: points) {
                    const double x = point.x() + offset;
                    const double left = x - width * 0.46, right = x + width * 0.46;
                    upper->append(left, 0);
                    upper->append(left, point.y());
                    upper->append(right, point.y());
                    upper->append(right, 0);
                    minX = std::min(minX, left);
                    maxX = std::max(maxX, right);
                }
                lower->append(upper->at(0).x(), 0);
                lower->append(upper->at(upper->count() - 1).x(), 0);
            }
            for (const auto& point: points) {
                minX = std::min(minX, point.x());
                maxX = std::max(maxX, point.x());
                minY = std::min(minY, point.y());
                maxY = std::max(maxY, point.y());
            }
            bars->setName(column.legend);
            bars->setProperty("sourceName", representation.name);
            chart->addSeries(bars);
            bars->setColor(column.color);
            bars->setPen(dense ? QPen(Qt::NoPen) : QPen(column.color.darker(150), 0.6));
            bars->attachAxis(axisX);
            bars->attachAxis(axisY);
            ++visibleCount;
            ++group;
            const QString xName = representation.useIndex ? QStringLiteral("索引") : representation.xColumn;
            if (!xNames.contains(xName)) xNames.append(xName);
        }
    }
    if (!std::isfinite(minX)) {
        minX = 0;
        maxX = 1;
    }
    const double xPad = std::max(0.01, (maxX - minX) * 0.02);
    const double yPad = std::max(1e-6, (maxY - minY) * 0.025);
    axisX->setRange(minX - xPad, maxX + xPad);
    axisY->setRange(minY < 0 ? minY - yPad : 0, maxY + yPad);
    for (auto* axis: {axisX, axisY}) {
        axis->setLabelFormat("%g");
        axis->setLabelsFont(QFont(QStringLiteral("Arial"), 9));
        axis->setLabelsColor(Qt::black);
        axis->setGridLinePen(QPen(QColor("#F1F1F1"), 0.6));
        axis->setLinePen(QPen(Qt::black, 0.8));
        axis->setMinorTickCount(0);
    }
    UpdateTicks(chart, m_ChartView->size());
    static_cast<HistogramChartView*>(m_ChartView)->captureDefaultView();
    m_ChartView->viewport()->update();
    m_ChartView->setToolTip(
            QStringLiteral("滚轮缩放；左键或中键拖动平移；双击恢复全图\n%1 个系列；X: %3%2")
                    .arg(visibleCount)
                    .arg(skipped ? QStringLiteral("；已跳过 %1 个非有限图表值").arg(static_cast<qulonglong>(skipped))
                                 : QString())
                    .arg(xNames.join(QStringLiteral(" / "))));
}
