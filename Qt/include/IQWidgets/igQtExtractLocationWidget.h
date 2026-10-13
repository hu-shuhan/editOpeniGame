#pragma once

#include <IQCore/igQtExportModule.h>

#include <iGameModel.h>
#include <iGamePointSet.h>
#include <iGameUnstructuredMesh.h>

#include <QDialog>

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QTableWidget;
class igQtModelDialogWidget;
class igQtModelDrawWidget;

class IG_QT_MODULE_EXPORT igQtExtractLocationWidget : public QDialog {
public:
    explicit igQtExtractLocationWidget(igQtModelDrawWidget* rendererWidget,
                                       igQtModelDialogWidget* modelTreeWidget,
                                       iGame::Model::Pointer sourceModel,
                                       QWidget* parent = nullptr);
    ~igQtExtractLocationWidget() override;

    bool isReady() const { return !m_InputMesh.IsNull(); }

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildUi();
    void createQueryPoint();
    void cleanupQueryPoint();
    void syncQueryPointFromControls();
    void executeFilter();
    void refreshArrayTable();
    void showSelectedArrayValue();
    void showMessage(const QString& text, bool information = false);

    igQtModelDrawWidget* m_RendererWidget{nullptr};
    igQtModelDialogWidget* m_ModelTreeWidget{nullptr};
    iGame::Model::Pointer m_SourceModel{};
    iGame::UnstructuredMesh::Pointer m_InputMesh{};
    iGame::UnstructuredMesh::Pointer m_LatestOutput{};
    iGame::PointSet::Pointer m_QueryPoint{};
    IGuint m_QueryPointModelId{0};
    bool m_QueryPointAdded{false};

    iGame::Point m_BoundsCenter{0.0, 0.0, 0.0};
    QDoubleSpinBox* m_XSpin{nullptr};
    QDoubleSpinBox* m_YSpin{nullptr};
    QDoubleSpinBox* m_ZSpin{nullptr};
    QCheckBox* m_ShowPointCheck{nullptr};
    QComboBox* m_DragAxisCombo{nullptr};
    QTableWidget* m_DataTable{nullptr};
    QComboBox* m_FieldCombo{nullptr};
    QLabel* m_FindResultLabel{nullptr};
    QLabel* m_StatusLabel{nullptr};
};
