#pragma once

#include <IQCore/igQtExportModule.h>

#include <iGameModel.h>
#include <iGamePointSet.h>

#include <QDialog>

#include <array>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class igQtModelDialogWidget;
class igQtModelDrawWidget;

class IG_QT_MODULE_EXPORT igQtTextureMapToPlaneWidget : public QDialog {
public:
    explicit igQtTextureMapToPlaneWidget(igQtModelDrawWidget* rendererWidget,
                                         igQtModelDialogWidget* modelTreeWidget,
                                         iGame::Model::Pointer sourceModel,
                                         QWidget* parent = nullptr);

    bool isReady() const { return !m_Input.IsNull(); }

private:
    void buildUi();
    void applyPlanePreset(int preset);
    void setManualControlsEnabled(bool enabled);
    void executeFilter();
    iGame::Point controlPoint(int row) const;
    void setControlPoint(int row, const iGame::Point& point);
    void showMessage(const QString& text, bool information = false);

    igQtModelDrawWidget* m_RendererWidget{nullptr};
    igQtModelDialogWidget* m_ModelTreeWidget{nullptr};
    iGame::Model::Pointer m_SourceModel{};
    iGame::PointSet::Pointer m_Input{};

    QCheckBox* m_AutomaticCheck{nullptr};
    QComboBox* m_PresetCombo{nullptr};
    QGroupBox* m_ManualGroup{nullptr};
    std::array<std::array<QDoubleSpinBox*, 3>, 3> m_PlaneControls{};
    QLabel* m_StatusLabel{nullptr};
};
