// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
// Regression: the original model and histogram used separate charts, with no series controls.
// Keep both in one numeric coordinate system and preserve independent representation settings.
// Cover point/cell switching, magnitude, reapply, averages, normalization and nonfinite chart values.
// Fix commit title: fix: unify histogram charts and preserve signed bars
// Locate the first commit adding this test:
// git log --diff-filter=A --format="%h %s" -- Examples/Filter/Histogram/TestHistogramChartChecks.cpp
#include <Histogram/iGameHistogramFilter.h>
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQCore/igQtMainWindow.h>
#include <IQWidgets/igQtHistogramChartWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGamePointSet.h>
#include <iGameSceneManager.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QWheelEvent>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>

using namespace iGame;

struct HistogramRenderProbe : igQtRenderWidget {
    static void AttachCpuScene(igQtRenderWidget* widget, Scene::Pointer scene) {
        auto member = &HistogramRenderProbe::m_Scene;
        widget->*member = scene;
    }
};

int main(int argc, char** argv) {
    std::cout << std::unitbuf;
    QApplication app(argc, argv);
#ifdef _WIN32
    // The offscreen plugin needs an explicit font for readable exported previews on Windows.
    if (QGuiApplication::platformName() == "offscreen")
        QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/arial.ttf"));
#endif
    Q_INIT_RESOURCE(iGameQtMainWindow);
    auto input = FileIO::ReadFile("Models/ExtractLocationConnectedSteppedSolid.vtk");
    if (!input) return 1;
    input->SetName("ExtractLocationConnectedSteppedSolid.vtk");
    int pointIndex = -1, cellIndex = -1;
    for (size_t i = 0; i < input->GetAttributeSet()->GetNumberOfAttributes(); ++i) {
        auto& attribute = input->GetAttributeSet()->GetAttribute(i);
        const auto name = attribute.pointer->GetName();
        if (name == "PointValue" || name == "pointvalue") pointIndex = static_cast<int>(i);
        if (name == "CellValue" || name == "cellvalue") cellIndex = static_cast<int>(i);
    }
    auto filter = HistogramFilter::New();
    filter->SetInput(input);
    filter->SetAttributeIndex(pointIndex);
    filter->SetNumberOfBins(10);
    if (pointIndex < 0 || cellIndex < 0 || !filter->Execute()) return 1;
    auto histogram = filter->GetHistogramData();
    QMainWindow window;
    window.setCentralWidget(new QWidget(&window));
    igQtHistogramChartWidget widget;
    auto* settings = widget.settingsWidget();
    auto* settingsDock = new QDockWidget("Series Parameters", &window);
    settingsDock->setWidget(settings);
    window.addDockWidget(Qt::LeftDockWidgetArea, settingsDock);
    settings->show();
    auto* chartDock = new QDockWidget("Histogram", &window);
    chartDock->setWidget(&widget);
    window.addDockWidget(Qt::RightDockWidgetArea, chartDock);
    widget.SetHistogram(input, histogram, pointIndex, 0);
    window.resize(1280, 720);
    window.setWindowTitle("Histogram shared chart regression preview");
    window.show();
    app.processEvents();

    auto* source = settings->findChild<QComboBox*>("histogramSource");
    auto* attachment = settings->findChild<QComboBox*>("histogramAttachment");
    auto* useIndex = settings->findChild<QCheckBox*>("histogramUseIndex");
    auto* xArray = settings->findChild<QComboBox*>("histogramXArray");
    auto* table = settings->findChild<QTableWidget*>("histogramSeries");
    auto* chart = widget.findChild<QChartView*>("histogramPlot")->chart();
    auto seriesFor = [&](const QString& name) -> QAbstractSeries* {
        for (auto* series: chart->series())
            if (series->name() == name) return series;
        return nullptr;
    };
    int total = 0, failed = 0;
    auto check = [&](bool condition, const char* message) {
        ++total;
        if (!condition) ++failed;
        std::cout << (condition ? "PASS " : "FAIL ") << message << '\n';
    };
    auto rowFor = [&](const QString& name) {
        for (int row = 0; row < table->rowCount(); ++row)
            if (table->item(row, 2)->text().compare(name, Qt::CaseInsensitive) == 0) return row;
        return -1;
    };
    auto toggle = [&](const QString& name, bool visible) {
        const int row = rowFor(name);
        if (row < 0) {
            check(false, "requested series exists");
            return;
        }
        table->item(row, 0)->setCheckState(visible ? Qt::Checked : Qt::Unchecked);
    };
    auto sameAxes = [&]() {
        for (auto* series: chart->series())
            if (series->attachedAxes().size() != 2 || !series->attachedAxes().contains(chart->axes()[0]) ||
                !series->attachedAxes().contains(chart->axes()[1]))
                return false;
        return chart->axes().size() == 2;
    };

    // Layout regression: controls must not consume the plot's width/height (see the fix commit in the header).
    check(window.dockWidgetArea(settingsDock) == Qt::LeftDockWidgetArea && !widget.isAncestorOf(settings),
          "series settings are hosted separately on the left");
    auto* plot = widget.findChild<QChartView*>("histogramPlot");
    check(window.dockWidgetArea(chartDock) == Qt::RightDockWidgetArea && plot->height() >= widget.height() - 10 &&
                  plot->width() >= widget.width() - 10,
          "right dock contains a plot that fills the available area");

    check(source->count() == 2 && source->currentIndex() == 1, "source and result selectable");
    check(!useIndex->isChecked() && xArray->currentText() == "bin_extents", "result uses numeric bin centers");
    check(chart->series().size() == 1 && chart->series()[0]->name() == "bin_values", "default histogram only");
    auto* bars = qobject_cast<QAreaSeries*>(chart->series()[0]);
    check(bars && bars->upperSeries()->count() == 40 &&
                  std::abs((bars->upperSeries()->at(0).x() + bars->upperSeries()->at(2).x()) / 2 - 3.95) < 1e-6 &&
                  bars->upperSeries()->at(1).y() == 8,
          "bar is centered at 3.95 with frequency 8");
    {
        // BUG: chart views only painted data and consumed no navigation input (see the fix commit in the header).
        // Send real viewport events: scaling must preserve the anchor, panning must preserve spans,
        // and reset must restore the data range even after extreme magnification.
        auto* x = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal)[0]);
        auto* y = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0]);
        const double xMin = x->min(), xMax = x->max(), yMin = y->min(), yMax = y->max();
        const double xSpan = xMax - xMin, ySpan = yMax - yMin;
        auto nearlyEqual = [](double a, double b) { return std::abs(a - b) < 1e-9 * std::max(1.0, std::abs(b)); };
        auto point = [&]() {
            const QRectF area = chart->plotArea();
            return plot->mapFromScene(
                    chart->mapToScene(QPointF(area.left() + area.width() * 0.3, area.top() + area.height() * 0.4)));
        };
        auto wheel = [&](const QPoint& location, int delta) {
            QWheelEvent event(location, plot->viewport()->mapToGlobal(location), QPoint(), QPoint(0, delta),
                              Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
            QApplication::sendEvent(plot->viewport(), &event);
        };
        auto mouse = [&](QEvent::Type type, const QPoint& location, Qt::MouseButton button, Qt::MouseButtons buttons) {
            QMouseEvent event(type, location, button, buttons, Qt::NoModifier);
            QApplication::sendEvent(plot->viewport(), &event);
        };
        auto reset = [&]() { mouse(QEvent::MouseButtonDblClick, point(), Qt::LeftButton, Qt::LeftButton); };
        auto restored = [&]() {
            return nearlyEqual(x->min(), xMin) && nearlyEqual(x->max(), xMax) && nearlyEqual(y->min(), yMin) &&
                   nearlyEqual(y->max(), yMax);
        };
        const QPoint anchorPoint = point();
        const QPointF anchor = chart->mapFromScene(plot->mapToScene(anchorPoint));
        const QRectF area = chart->plotArea();
        const double fx = (anchor.x() - area.left()) / area.width();
        const double fy = (area.bottom() - anchor.y()) / area.height();
        wheel(anchorPoint, 120);
        check(x->max() - x->min() < xSpan && y->max() - y->min() < ySpan &&
                      nearlyEqual(x->min() + fx * (x->max() - x->min()), xMin + fx * xSpan) &&
                      nearlyEqual(y->min() + fy * (y->max() - y->min()), yMin + fy * ySpan),
              "wheel zooms both axes around the pointer");
        wheel(anchorPoint, -120);
        check(restored(), "reverse wheel restores the original scale");
        const QPoint start = point(), end = start + QPoint(30, 20);
        mouse(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        mouse(QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
        mouse(QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
        check(x->min() < xMin && y->min() > yMin && nearlyEqual(x->max() - x->min(), xSpan) &&
                      nearlyEqual(y->max() - y->min(), ySpan),
              "left drag pans both axes without changing scale");
        reset();
        check(restored(), "double click restores the full data range after panning");
        mouse(QEvent::MouseButtonPress, start, Qt::MiddleButton, Qt::MiddleButton);
        mouse(QEvent::MouseMove, end, Qt::NoButton, Qt::MiddleButton);
        mouse(QEvent::MouseButtonRelease, end, Qt::MiddleButton, Qt::NoButton);
        const double panned = x->min();
        mouse(QEvent::MouseMove, start, Qt::NoButton, Qt::NoButton);
        check(!nearlyEqual(panned, xMin) && nearlyEqual(x->min(), panned),
              "middle drag pans and release ends the gesture");
        reset();
        wheel(QPoint(1, 1), 120);
        check(restored(), "wheel outside the plot does not alter axes");
        for (int step = 0; step < 180; ++step) wheel(point(), 120);
        check(std::isfinite(x->min()) && std::isfinite(y->min()) && x->max() - x->min() >= xSpan * 0.99e-9 &&
                      y->max() - y->min() >= ySpan * 0.99e-9,
              "extreme zoom remains finite and bounded");
        reset();
        check(restored() && sameAxes() && histogram->GetBinValues()->GetValue(0) == 8,
              "reset after extreme zoom preserves series, axes and histogram counts");
    }
    source->setCurrentIndex(0);
    check(useIndex->isChecked() && rowFor("PointVector_Magnitude") >= 0 && rowFor("Points_X") >= 0,
          "source exposes vector components, magnitude and coordinates");
    toggle("PointValue", true);
    toggle("PointVector_Magnitude", true);
    check(chart->series().size() == 3 && sameAxes(), "source and histogram share one pair of axes");
    const int pointRow = rowFor("PointValue");
    table->item(pointRow, 3)->setText("Value legend");
    check(seriesFor("Value legend") != nullptr, "legend edits redraw series");
    source->setCurrentIndex(1);
    toggle("bin_values", false);
    check(chart->series().size() == 2, "hiding result preserves source series");
    toggle("bin_values", true);
    source->setCurrentIndex(0);
    check(table->item(rowFor("PointValue"), 0)->checkState() == Qt::Checked &&
                  table->item(rowFor("PointValue"), 3)->text() == "Value legend",
          "source settings persist across selection");
    useIndex->setChecked(false);
    xArray->setCurrentIndex(xArray->findText("PointValue"));
    check(!useIndex->isChecked() && sameAxes(), "source can choose an array for X");
    useIndex->setChecked(true);
    attachment->setCurrentIndex(1);
    check(rowFor("CellValue") >= 0 && rowFor("PointValue") < 0, "cell association has separate series");
    toggle("CellValue", true);
    check(chart->series().size() == 2 && sameAxes(), "cell values and result share axes");
    attachment->setCurrentIndex(0);
    check(chart->series().size() == 3, "switching back restores point series");

    filter->SetNormalize(true);
    filter->SetCalculateAverages(true);
    filter->SetNumberOfBins(5);
    if (!filter->Execute()) return 1;
    auto normalized = filter->GetHistogramData();
    widget.SetHistogram(input, normalized, pointIndex, 0, histogram.get());
    check(source->count() == 2 && chart->series().size() == 3, "reapply replaces result without duplicating sources");
    check(rowFor("PointVector_average_Magnitude") >= 0, "averages expose components and magnitude");
    bars = qobject_cast<QAreaSeries*>(seriesFor("bin_values"));
    check(bars && std::abs(bars->upperSeries()->at(1).y() - 0.2) < 1e-12,
          "normalized frequency keeps floating point precision");
    toggle("bin_values", false);
    auto emptyResult = HistogramData::New();
    auto extents = DoubleArray::New();
    auto values = DoubleArray::New();
    extents->SetName("bin_extents");
    values->SetName("bin_values");
    extents->AddValue(0);
    values->AddValue(std::numeric_limits<double>::quiet_NaN());
    emptyResult->SetBinExtents(extents);
    emptyResult->SetBinValues(values);
    widget.SetHistogram(input, emptyResult, pointIndex, 0);
    check(chart->series().size() == 2 && sameAxes(), "nonfinite chart values do not corrupt axes");
    toggle("bin_values", false);
    widget.SelectSource(input.get());
    toggle("PointValue", false);
    toggle("PointVector_Magnitude", false);
    check(chart->series().isEmpty() && sameAxes(), "all series hidden leaves valid empty axes");
    {
        // BUG: >1000 tuples became a line joining values, losing the area from each value to zero.
        // See the fix commit in the header. Inspect actual pixels above/below zero, not only the series type.
        auto signedInput = PointSet::New();
        signedInput->SetName("Signed large point set");
        auto coordinates = Points::New();
        for (int i = 0; i < 2048; ++i) coordinates->AddPoint(i < 1024 ? 2 : -3, 0, 0);
        signedInput->SetPoints(coordinates);
        igQtHistogramChartWidget denseWidget;
        denseWidget.resize(640, 420);
        denseWidget.SetHistogram(signedInput, emptyResult, -1, 0);
        auto* denseSource = denseWidget.findChild<QComboBox*>("histogramSource");
        auto* denseTable = denseWidget.findChild<QTableWidget*>("histogramSeries");
        denseSource->setCurrentIndex(0);
        for (int row = 0; row < denseTable->rowCount(); ++row)
            if (denseTable->item(row, 2)->text() == "Points_X") denseTable->item(row, 0)->setCheckState(Qt::Checked);
        denseWidget.show();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        app.processEvents();
        auto* densePlot = denseWidget.findChild<QChartView*>("histogramPlot");
        auto* denseChart = densePlot->chart();
        const auto image = denseWidget.grab().toImage();
        auto colorAt = [&](double index, double value) {
            auto* ax = qobject_cast<QValueAxis*>(denseChart->axes(Qt::Horizontal)[0]);
            auto* ay = qobject_cast<QValueAxis*>(denseChart->axes(Qt::Vertical)[0]);
            const auto area = denseChart->plotArea();
            const QPointF p(area.left() + (index - ax->min()) / (ax->max() - ax->min()) * area.width(),
                            area.bottom() - (value - ay->min()) / (ay->max() - ay->min()) * area.height());
            const auto local =
                    densePlot->viewport()->mapTo(&denseWidget, densePlot->mapFromScene(denseChart->mapToScene(p)));
            return image.pixelColor(qRound(local.x() * image.devicePixelRatio()),
                                    qRound(local.y() * image.devicePixelRatio()));
        };
        auto isBrown = [](QColor color) {
            return color.red() > color.green() + 20 && color.green() > color.blue() + 10;
        };
        check(denseChart->series().size() == 1 && qobject_cast<QAreaSeries*>(denseChart->series()[0]),
              "large series remain bars instead of switching to lines");
        check(isBrown(colorAt(513, 1.1)), "dense positive bars fill the region between zero and positive values");
        check(isBrown(colorAt(1537, -1.4)), "dense negative bars fill the region between zero and negative values");
        check(colorAt(1537, 1.1).lightness() > 235, "negative bars do not fill the positive half of the chart");
        // The axis includes padding before the first sample: only the baseline can color this pixel.
        const QColor baseline = colorAt(-20, 0);
        check(baseline.lightness() < 180, "signed data has a continuous visible zero baseline");
    }
    {
        // Exercise real menu/tab ownership without showing the main window or creating GL resources.
        // A parameter dialog deletes itself on close; its content must first leave the tool tab.
        // Upstream 9eaf8f90 switched close() to deleteLater() to avoid deleting an active callback.
        // Settle deferred deletion before checking ownership or counting reopened dialog shells.
        igQtMainWindow mainWindow;
        auto scene = SceneManager::Instance()->NewScene();
        HistogramRenderProbe::AttachCpuScene(mainWindow.rendererWidget, scene);
        scene->AddModel(input);
        QAction* action = nullptr;
        for (auto* candidate: mainWindow.findChildren<QAction*>())
            if (candidate->objectName() == QStringLiteral("action_filter_histogram")) action = candidate;
        auto* tabs = mainWindow.findChild<QTabWidget*>("LeftFieldTabs");
        auto* leftDock = mainWindow.findChild<QDockWidget*>("LeftFieldDock");
        if (!action || !tabs || !leftDock) return 1;
        action->trigger();
        QPointer<igQtFilterDialogDockWidget> parameters =
                mainWindow.findChild<igQtFilterDialogDockWidget*>("HistogramParametersDock");
        check(parameters && parameters->isHidden() && parameters->widget() == nullptr &&
                      tabs->currentWidget()->isAncestorOf(parameters->getWidget(1)) &&
                      mainWindow.dockWidgetArea(leftDock) == Qt::LeftDockWidgetArea,
              "histogram menu docks bin parameters in the existing left tool stack");
        if (!parameters) return 1;
        parameters->apply();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        auto* sharedChart = mainWindow.findChild<igQtHistogramChartWidget*>();
        auto* rightDock = mainWindow.findChild<QDockWidget*>("dockWidget_Histogram");
        check(!parameters && sharedChart && tabs->currentWidget()->isAncestorOf(sharedChart->settingsWidget()) &&
                      mainWindow.dockWidgetArea(rightDock) == Qt::RightDockWidgetArea &&
                      !sharedChart->findChild<QComboBox*>("histogramSource"),
              "apply deletes parameters safely, opens series on the left and leaves only the plot on the right");
        mainWindow.closeLeftToolWidget("histogramSeries");
        mainWindow.openLeftToolWidget("histogramSeries", sharedChart->settingsWidget(), QStringLiteral("直方图系列"));
        check(tabs->count() == 1 && tabs->currentWidget()->isAncestorOf(sharedChart->settingsWidget()),
              "series tab can close and reopen without losing controls");
        action->trigger();
        parameters = mainWindow.findChild<igQtFilterDialogDockWidget*>("HistogramParametersDock");
        auto* close = tabs->currentWidget()->findChild<QPushButton*>("closeButton");
        if (!close) return 1;
        close->click();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        check(!parameters && tabs->count() == 1, "parameter close button removes its tab and deletes the dialog");
        action->trigger();
        QMetaObject::invokeMethod(tabs, "tabCloseRequested", Qt::DirectConnection, Q_ARG(int, tabs->currentIndex()));
        action->trigger();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        check(tabs->count() == 2 &&
                      mainWindow.findChildren<igQtFilterDialogDockWidget*>("HistogramParametersDock").size() == 1,
              "reopening after tab close replaces the hidden parameter shell without duplicate tabs");
        tabs->currentWidget()->findChild<QPushButton*>("closeButton")->click();
    }
    std::cout << total - failed << '/' << total << " checks passed\n";

    // Render the real widget for visual review; these images do not replace the numerical checks above.
    const std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "--render-bicycle") {
        auto bicycle = FileIO::ReadFile("Models/Convert_Quad_Bicycle.vtk");
        if (!bicycle) return 1;
        widget.SetHistogram(bicycle, emptyResult, -1, 0);
        widget.SelectSource(bicycle.get());
        chartDock->setWidget(nullptr);
        widget.setParent(nullptr);
        widget.resize(650, 780);
        widget.show();
        for (const QString component: {QStringLiteral("Points_X"), QStringLiteral("Points_Y")}) {
            toggle(component, true);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            app.processEvents();
            QElapsedTimer timer;
            timer.start();
            const auto image = widget.grab();
            std::cout << component.toStdString() << ": " << bicycle->GetPoints()->GetNumberOfPoints()
                      << " values, rendered in " << timer.elapsed() << " ms\n";
            if (!image.save(QStringLiteral("build-msvc/histogram-bicycle-%1.png").arg(component))) return 1;
            toggle(component, false);
        }
        return failed ? 1 : 0;
    }
    if (mode == "--preview" || mode == "--render-preview") {
        filter->SetNormalize(false);
        filter->SetCalculateAverages(false);
        filter->SetNumberOfBins(10);
        if (!filter->Execute()) return 1;
        widget.SetHistogram(input, filter->GetHistogramData(), pointIndex, 0, normalized.get());
        source->setCurrentIndex(1);
        toggle("bin_values", true);
        source->setCurrentIndex(0);
        table->item(rowFor("PointValue"), 3)->setText("PointValue");
        for (int row = 0; row < table->rowCount(); ++row) table->item(row, 0)->setCheckState(Qt::Checked);
        if (mode == "--render-preview") {
            chartDock->setWidget(nullptr);
            widget.setParent(nullptr);
            widget.resize(550, 780);
            widget.show();
            // Chart axes release their graphics items with deferred deletion; settle before capture.
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            app.processEvents();
            if (!widget.grab().save("build-msvc/histogram-style-all-series.png")) return 1;
            source->setCurrentIndex(1);
            toggle("bin_values", false);
            source->setCurrentIndex(0);
            for (int row = 0; row < table->rowCount(); ++row) table->item(row, 0)->setCheckState(Qt::Unchecked);
            toggle("PointVector_Magnitude", true);
            toggle("PointVector_X", true);
            toggle("PointVector_Y", true);
            auto* xAxis = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal)[0]);
            xAxis->setRange(2.8, 16.7);
            xAxis->setTickInterval(2);
            auto* yAxis = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0]);
            yAxis->setRange(-0.1, 10.2);
            yAxis->setTickInterval(0.5);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            app.processEvents();
            if (!widget.grab().save("build-msvc/histogram-style-vector-series.png")) return 1;
            return failed ? 1 : 0;
        }
        return app.exec();
    }
    return failed ? 1 : 0;
}
