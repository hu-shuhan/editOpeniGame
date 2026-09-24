#include "IQWidgets/igQtScalarViewWidget.h"
#include "IQWidgets/igQtColorBarWidget.h"
#include "iGamePointSet.h"
#include "iGameSceneManager.h"
#include <QApplication>
#include <QMouseEvent>
#include <QRadioButton>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace iGame;

namespace {
class TestPoints : public PointSet {
public:
    static SmartPointer<TestPoints> New() { return new TestPoints; }
    float red(IGsize point) { return m_Colors->GetValue(point * 4); }
};

void require(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}
bool almostEqual(double a, double b) { return std::abs(a - b) < 1e-4; }

void drag(QWidget* slider, const QPoint& from, const QPoint& to) {
    QMouseEvent press(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(slider, &press);
    QMouseEvent move(QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(slider, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(slider, &release);
}

void testRangeInteraction(bool remote) {
    std::cerr << "STAGE: " << (remote ? "C/S" : "local") << " create scene" << std::endl;
    auto scene = SceneManager::Instance()->NewScene();
    auto points = TestPoints::New();
    points->SetRemoteRenderingEnabled(remote);
    auto values = FloatArray::New();
    values->SetName("temperature");
    for (int i = 0; i < 5; ++i) {
        points->AddPoint(Point(static_cast<float>(i), 0, 0));
        values->AddValue(25.0f * i);
    }
    points->GetAttributeSet()->AddScalar(IG_POINT, values);
    std::cerr << "STAGE: add model" << std::endl;
    const auto pointsModelId = scene->AddModel(points);
    std::cerr << "STAGE: select scalar and prepare CPU colors" << std::endl;
    points->ViewCloudPicture(scene, 0, 0);
    points->ConvertToDrawableData();
    auto mapper = points->GetColorMapper();
    const float originalRed = points->red(1);

    // Core commits 8e89e0139/078b50b56 preserve a newer mapper range instead of
    // reproducing the original overwrite. This local regression update is pending commit.
    mapper->SetRange(25, 75);
    points->ConvertToDrawableData();
    require(almostEqual(mapper->GetRange()[0], 25) && almostEqual(mapper->GetRange()[1], 75),
            "core redraw overwrote the newer mapper range");
    // Restore the full range explicitly so the UI interaction starts at both endpoints.
    mapper->SetRange(0, 100);
    points->ConvertToDrawableData();
    std::cerr << "STAGE: newer mapper range preserved; begin UI interaction" << std::endl;

    igQtScalarViewWidget panel;
    panel.resize(400, 500);
    panel.showScalarView();
    panel.show();
    QApplication::processEvents();
    auto* slider = panel.findChild<igQtDataRangeSlider*>("widget_DataRangeSlider");
    require(slider != nullptr, "missing data range slider");
    slider->grab(); // Initialize the handle geometry via the real paint path.
    double selectedMin = -1, selectedMax = -1;
    QObject::connect(slider, &igQtDataRangeSlider::DataRangeChanged,
                     [&selectedMin, &selectedMax](double min, double max) {
                         selectedMin = min;
                         selectedMax = max;
                     });
    int legendUpdates = 0;
    QObject::connect(&panel, &igQtScalarViewWidget::updateCurrentModelColor,
                     [&legendUpdates]() { ++legendUpdates; });
    const int length = slider->width() - 20;
    drag(slider, QPoint(10, 7), QPoint(10 + length / 4, 7));
    require(selectedMin > 20 && selectedMin < 30, "lower handle did not move");
    points->ConvertToDrawableData();
    require(almostEqual(mapper->GetRange()[0], selectedMin), "redraw overwrote dragged minimum");
    require(!almostEqual(points->red(1), originalRed), "drag did not change actual point colors");
    require(legendUpdates > 0, "range edit did not request a legend refresh");

    drag(slider, QPoint(10 + length, 7), QPoint(10 + length * 3 / 4, 7));
    points->ConvertToDrawableData();
    require(selectedMax > 70 && selectedMax < 80, "upper handle did not move");
    require(almostEqual(mapper->GetRange()[0], selectedMin) && almostEqual(mapper->GetRange()[1], selectedMax),
            "redraw overwrote narrowed range");
    auto* linear = panel.findChild<QRadioButton*>("radioButton_Liner");
    auto* step = panel.findChild<QRadioButton*>("radioButton_Step");
    require(linear != nullptr && step != nullptr, "missing interpolation control");
    step->setChecked(true);
    require(!linear->isChecked(), "interpolation mode did not change");
    points->ConvertToDrawableData();
    require(almostEqual(mapper->GetRange()[0], selectedMin) && almostEqual(mapper->GetRange()[1], selectedMax),
            "interpolation change reset the dragged range");

    const auto beforeEdit = points->InspectCpuDisplayCache();
    slider->DataRangeChanged(10, 60);
    const auto edited = points->InspectCpuDisplayCache();
    require(edited.signature != beforeEdit.signature && !edited.ready,
            "C/S prepared cache did not detect a color mapper edit");
    points->ConvertToDrawableData();
    require(almostEqual(mapper->GetRange()[0], 10) && almostEqual(mapper->GetRange()[1], 60),
            "subsequent redraw reset the manual range");
    auto dataRange = points->GetAttributeSet()->GetAttribute(0).GetDataRange();
    require(almostEqual(dataRange->GetValue(2), 0) && almostEqual(dataRange->GetValue(3), 100),
            "manual range changed the original attribute range");

    panel.showCustomScaleRangeWidget();
    QWidget* rangeDialog = nullptr;
    for (auto* widget : QApplication::topLevelWidgets()) {
        if (widget->findChild<QLineEdit*>("lineEdit_min")) { rangeDialog = widget; break; }
    }
    require(rangeDialog != nullptr, "missing custom range dialog");
    rangeDialog->findChild<QLineEdit*>("lineEdit_min")->setText("15");
    rangeDialog->findChild<QLineEdit*>("lineEdit_max")->setText("65");
    panel.setCustomScaleRange();
    points->ConvertToDrawableData();
    require(almostEqual(mapper->GetRange()[0], 15) && almostEqual(mapper->GetRange()[1], 65),
            "redraw overwrote custom range");

    igQtColorBarWidget legend;
    legend.grab(); // Allow its paint handler to settle the size.
    const QImage unlockedLegend = legend.grab().toImage();
    auto& attr = points->GetAttributeSet()->GetAttribute(0);
    attr.rangeLocked = true;
    const QImage lockedLegend = legend.grab().toImage();
    attr.rangeLocked = false;
    require(unlockedLegend == lockedLegend,
            "locked attribute range overrode the manual mapping range in the legend");
    delete rangeDialog;

    panel.rescaleRange();
    points->ConvertToDrawableData();
    require(almostEqual(mapper->GetRange()[0], 0) && almostEqual(mapper->GetRange()[1], 100),
            "rescale did not restore the original range");

    // C/S reattachment changes the selected model without emitting CloudPictureChanged.
    // The read-only sync must bind the new mapper and leave prepared data untouched.
    auto cached = TestPoints::New();
    cached->SetRemoteRenderingEnabled(true);
    cached->AddPoint(Point(0, 0, 0));
    cached->AddPoint(Point(1, 0, 0));
    auto cachedValues = FloatArray::New();
    cachedValues->SetName("cached temperature");
    cachedValues->AddValue(1000);
    cachedValues->AddValue(2000);
    cached->GetAttributeSet()->AddScalar(IG_POINT, cachedValues);
    scene->AddModel(cached);
    cached->ViewCloudPicture(scene, 0, 0);
    cached->ConvertToDrawableData();
    const auto prepared = cached->InspectCpuDisplayCache();
    panel.syncScalarViewFromCurrentModel();
    require(prepared.signature == cached->InspectCpuDisplayCache().signature,
            "cache selection sync changed prepared display data");
    slider->DataRangeChanged(1100, 1800);
    cached->ConvertToDrawableData();
    require(almostEqual(cached->GetColorMapper()->GetRange()[0], 1100) &&
            almostEqual(cached->GetColorMapper()->GetRange()[1], 1800),
            "cached model drag did not change the current model");
    require(almostEqual(mapper->GetRange()[0], 0) && almostEqual(mapper->GetRange()[1], 100),
            "cached model drag changed the previous model");
    scene->SetCurrentModel(static_cast<int>(pointsModelId));
    points->ViewCloudPicture(scene, -1);
    panel.showScalarView();
    require(panel.getCurrentSelectedScalarIdx() == -1 && slider->isHidden(),
            "no scalar selection left an active range slider");
    slider->DataRangeChanged(1, 2); // A queued notification after deselection is harmless.
    std::cout << "PASS: " << (remote ? "C/S" : "local")
              << " range drag, actual colors, interpolation, cache invalidation and rescale\n";
}
} // namespace

int main(int argc, char** argv) {
    std::cerr << "STAGE: before QApplication" << std::endl;
    QApplication app(argc, argv);
    std::cerr << "STAGE: after QApplication" << std::endl;
    try {
        testRangeInteraction(false);
        testRangeInteraction(true);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
