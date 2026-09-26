#pragma once

#include <IQCore/igQtExportModule.h>
#include <TriangleStrip/iGameTriangleStripFilter.h>
#include <QWidget>

class QCheckBox;
class QDockWidget;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;

// Parameter/result panel for the existing TriangleStripFilter. The renderer
// still displays expanded triangles; the filter retains the strip point IDs.
class IG_QT_MODULE_EXPORT igQtTriangleStripWidget : public QWidget {
    Q_OBJECT
public:
    explicit igQtTriangleStripWidget(QWidget* parent = nullptr);
    static QDockWidget* createDockWidget(QWidget* parent);

    void setInput(iGame::DataObject::Pointer input);
    iGame::DataObject* input() const { return m_Input.get(); }
    bool isOutput(iGame::DataObject* object) const;
    bool apply();
    iGame::TriangleStripFilter* lastFilter() const { return m_Filter.get(); }
    iGame::UnstructuredMesh* polyLineOutput() const { return m_PolyLineOutput.get(); }

Q_SIGNALS:
    void resultReady(iGame::DataObject::Pointer surface, iGame::DataObject::Pointer lines);

private:
    void markParametersChanged();
    void clearStatistics();
    void showStatus(const QString& message, bool error = false);

    iGame::DataObject::Pointer m_Input;
    iGame::TriangleStripFilter::Pointer m_Filter;
    iGame::SurfaceMesh::Pointer m_SurfaceOutput;
    iGame::UnstructuredMesh::Pointer m_PolyLineOutput;

    QLabel* m_SourceLabel;
    QSlider* m_MaximumLengthSlider;
    QSpinBox* m_MaximumLength;
    QCheckBox* m_JoinPolyLines;
    QPushButton* m_ApplyButton;
    QLabel* m_StatusLabel;
    QLabel* m_TrianglesBefore;
    QLabel* m_TrianglesAfter;
    QLabel* m_StripCount;
    QLabel* m_OutputCellCount;
    QLabel* m_LongestStrip;
    QLabel* m_LineCount;
    QLabel* m_PointCount;
};
