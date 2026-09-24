// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TriangleStrip/TestTriangleStripWidget.cpp
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#include <IQWidgets/igQtTriangleStripWidget.h>
#include <iGameFileIO.h>

#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QFontDatabase>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
constexpr const char* ModelFilePath = "Models/TriangleStripTestModel.vtk";
constexpr const char* ExpectedTriangleCount = "8";
constexpr int ExpectedJoinedPointCount = 5;

void Check(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}

template <class T>
T* Control(QWidget* panel, const char* name) {
    auto* result = panel->findChild<T*>(QString::fromLatin1(name));
    Check(result != nullptr, name);
    return result;
}

}

int main(int argc, char** argv) {
    Q_INIT_RESOURCE(iGameQtMainWindow);
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
#if defined(Q_OS_WIN)
        // The deployed Qt 5 Windows runtime provides qwindows.dll but does
        // not necessarily ship a qoffscreen platform plugin.
        qputenv("QT_QPA_PLATFORM", "windows");
#else
        qputenv("QT_QPA_PLATFORM", "offscreen");
#endif
    }
    QApplication app(argc, argv);
    try {
        // Windows' offscreen platform does not enumerate system fonts. Use the
        // same bundled Chinese font as the application for meaningful layout QA.
        const int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/Styles/Styles/SourceHanSansCN-Normal.otf"));
        Check(fontId >= 0, "Cannot load the bundled UI font");
        const auto families = QFontDatabase::applicationFontFamilies(fontId);
        Check(!families.isEmpty(), "The bundled font has no family");
        app.setFont(QFont(families.first(), 10));
        iGame::Log::Init();
        std::unique_ptr<QDockWidget> dock(igQtTriangleStripWidget::createDockWidget(nullptr));
        auto* panel = dock->findChild<igQtTriangleStripWidget*>();
        Check(panel != nullptr, "Missing triangle-strip panel");
        auto* length = Control<QSpinBox>(panel, "maximumStripLength");
        auto* slider = Control<QSlider>(panel, "maximumStripLengthSlider");
        auto* join = Control<QCheckBox>(panel, "joinContiguousPolyLines");
        auto* apply = Control<QPushButton>(panel, "applyTriangleStrip");
        Check(length->value() == 1000 && !join->isChecked(), "Wrong parameter defaults");
        Check(!apply->isEnabled() && !panel->apply(), "Empty input should not execute");
        length->setValue(0);
        Check(length->value() == 1 && slider->value() == 1, "Invalid length was not clamped");
        slider->setValue(1000);
        Check(length->value() == 1000, "Slider/spinbox are not synchronized");

        int resultCount = 0;
        iGame::DataObject::Pointer lastSurface;
        iGame::DataObject::Pointer lastLines;
        QObject::connect(panel, &igQtTriangleStripWidget::resultReady,
                         [&](iGame::DataObject::Pointer surface, iGame::DataObject::Pointer lines) {
                             ++resultCount;
                             lastSurface = surface;
                             lastLines = lines;
                             Check(panel->isOutput(surface), "Surface output is not recognized");
                             Check(!lines || panel->isOutput(lines), "Line output is not recognized");
                         });

        auto testModel = iGame::FileIO::ReadFile(ModelFilePath);
        Check(testModel != nullptr, "Cannot read the triangle-strip test model");
        panel->setInput(testModel);
        Check(apply->isEnabled(), "Valid input did not enable Apply");
        apply->click();
        Check(resultCount == 1 && panel->lastFilter(), "Apply was not connected to the filter");
        Check(panel->lastFilter()->GetMaximumLength() == 1000, "Length was not passed to filter");
        Check(Control<QLabel>(panel, "trianglesBefore")->text() == ExpectedTriangleCount, "Wrong input triangle count");
        Check(Control<QLabel>(panel, "trianglesAfter")->text() == ExpectedTriangleCount, "Wrong output triangle count");
        Check(panel->lastFilter()->GetNumberOfStrips() == 1, "Model was not converted into one full strip");
        Check(Control<QLabel>(panel, "outputCellCount")->text() == QStringLiteral("1"),
              "Wrong ParaView-compatible output-cell count");
        Check(panel->lastFilter()->GetLongestStripLength() == 8, "Full strip has the wrong length");
        auto publishedSurface = iGame::DynamicCast<iGame::SurfaceMesh>(lastSurface);
        iGame::CellArray::Pointer publishedStrips;
        iGame::CellArray::Pointer publishedSourceFaceIds;
        Check(publishedSurface != nullptr &&
                      iGame::TriangleStripFilter::ReadOutputStrips(
                              publishedSurface, publishedStrips,
                              publishedSourceFaceIds),
              "Published output did not retain triangle-strip metadata");
        Check(publishedStrips->GetNumberOfCells() == 1 &&
                      publishedStrips->GetCellSize(0) == 10 &&
                      publishedSourceFaceIds->GetCellSize(0) == 8,
              "Published triangle-strip topology is invalid");
        Check(!lastLines && panel->polyLineOutput() == nullptr,
              "Triangle boundaries were incorrectly published as a line model");
        Check(Control<QLabel>(panel, "polyLineCount")->text() == QStringLiteral("0 → 0"),
              "Surface-only input has incorrect line statistics");
        Check(panel->input() == testModel.get(), "Apply changed the source to its output");

        length->setValue(4);
        Check(panel->apply() && resultCount == 2, "Cannot reapply modified parameters");
        Check(panel->lastFilter()->GetNumberOfStrips() == 2, "Length limit did not split the full strip in two");
        Check(Control<QLabel>(panel, "outputCellCount")->text() == QStringLiteral("2"),
              "Length-limited output-cell count is incorrect");
        Check(panel->lastFilter()->GetLongestStripLength() == 4, "New length limit ignored");
        publishedSurface = iGame::DynamicCast<iGame::SurfaceMesh>(lastSurface);
        Check(publishedSurface != nullptr &&
                      iGame::TriangleStripFilter::ReadOutputStrips(
                              publishedSurface, publishedStrips,
                              publishedSourceFaceIds) &&
                      publishedStrips->GetNumberOfCells() == 2 &&
                      publishedSourceFaceIds->GetNumberOfCells() == 2,
              "Published output lost length-limited strips or mappings");
        Check(Control<QLabel>(panel, "trianglesAfter")->text() == ExpectedTriangleCount, "Reapply lost triangles");

        join->setChecked(true);
        apply->click();
        Check(panel->lastFilter()->GetJoinContiguousSegments(), "Merge checkbox not passed to filter");
        Check(!lastLines && panel->polyLineOutput() == nullptr,
              "Join option manufactured lines from triangle boundaries");

        // ParaView/vtkStripper only joins line cells explicitly present in the
        // input. Add four contiguous lines and verify that this input does
        // produce a separate drawable line result in iGameVis.
        auto sourceMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(testModel);
        Check(sourceMesh != nullptr, "Test model is not an UnstructuredMesh");
        auto mixedCells = iGame::CellArray::New();
        auto mixedTypes = iGame::UnsignedIntArray::New();
        for (IGsize cellId = 0; cellId < sourceMesh->GetNumberOfCells(); ++cellId) {
            const igIndex* pointIds = nullptr;
            const int pointCount = sourceMesh->GetCells()->GetCellIds(
                    cellId, pointIds);
            mixedCells->AddCellIds(pointIds, pointCount);
            mixedTypes->AddValue(sourceMesh->GetCellType(cellId));
        }
        for (igIndex pointId = 0; pointId < 4; ++pointId) {
            const igIndex line[2]{pointId, pointId + 1};
            mixedCells->AddCellIds(line, 2);
            mixedTypes->AddValue(iGame::IG_LINE);
        }
        auto mixedInput = iGame::UnstructuredMesh::New();
        mixedInput->SetName("TriangleStripExplicitLines");
        mixedInput->SetPoints(sourceMesh->GetPoints());
        mixedInput->SetCells(mixedCells, mixedTypes);
        mixedInput->SetAttributeSet(
                iGame::AttributeSet::Pointer(sourceMesh->GetAttributeSet()));
        panel->setInput(mixedInput);
        Check(panel->apply() && resultCount == 4,
              "Mixed surface/line input did not execute");

        auto* lines = panel->polyLineOutput();
        Check(lines && lines->GetNumberOfCells() == 1,
              "Explicit contiguous input lines did not join");
        Check(lines->GetCellType(0) == iGame::IG_POLY_LINE &&
                      lines->GetCells()->GetCellSize(0) == ExpectedJoinedPointCount,
              "Joined output must be one open polyline, not a polygon");
        const igIndex* ids = nullptr;
        lines->GetCells()->GetCellIds(0, ids);
        Check(ids[0] != ids[ExpectedJoinedPointCount - 1],
              "Open input lines were incorrectly closed");
        Check(Control<QLabel>(panel, "polyLineCount")->text() == QStringLiteral("4 → 1"),
              "Wrong joined input-line statistics");
        Check(Control<QLabel>(panel, "outputCellCount")->text() == QStringLiteral("3"),
              "Mixed strip/polyline output-cell count is incorrect");

        // A line-only input still fails because there is no surface to strip.
        iGame::UnstructuredMesh::Pointer explicitLines = lines;
        panel->setInput(explicitLines);
        const int successes = resultCount;
        Check(!panel->apply() && resultCount == successes,
              "Line-only input should fail without publishing an empty surface");
        Check(!Control<QLabel>(panel, "triangleStripStatus")->text().isEmpty(), "No input error shown");
        panel->setInput(iGame::DataObject::New());
        Check(!apply->isEnabled(), "Unsupported data enabled Apply");
        panel->setInput(nullptr);
        Check(!apply->isEnabled() && panel->lastFilter() == nullptr, "Clearing source left an active result");

        panel->setInput(testModel);
        length->setValue(1000);
        join->setChecked(false);
        Check(panel->apply(), "Final triangle-strip test model run failed");
        std::cout << "Triangle-strip Qt panel tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Triangle-strip Qt panel test failed: " << error.what() << '\n';
        return 1;
    }
}
