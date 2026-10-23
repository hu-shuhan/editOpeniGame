#pragma once

#include <IQCore/igQtExportModule.h>

#include <iGameModel.h>
#include <iGameSurfaceMesh.h>

#include <QDialog>

#include <functional>
#include <utility>
#include <vector>

class QCheckBox;
class QLabel;
class QLineEdit;
class igQtModelDialogWidget;
class igQtModelDrawWidget;

class IG_QT_MODULE_EXPORT igQtConnectedSurfacePropertiesWidget : public QDialog {
public:
    explicit igQtConnectedSurfacePropertiesWidget(igQtModelDrawWidget* rendererWidget,
                                                   igQtModelDialogWidget* modelTreeWidget,
                                                   iGame::Model::Pointer sourceModel,
                                                   QWidget* parent = nullptr);
    ~igQtConnectedSurfacePropertiesWidget() override;

    bool isReady() const { return !m_InputMesh.IsNull(); }
    bool setSourceModel(iGame::Model::Pointer sourceModel);
    void setOutputReadyCallback(std::function<void(iGame::DataObject*)> callback) {
        m_OutputReadyCallback = std::move(callback);
    }

private:
    struct OutputBinding {
        iGame::SurfaceMesh::Pointer output{};
        iGame::Model::Pointer source{};
        unsigned long observerTag{0};
    };

    void buildUi();
    void executeFilter();
    void detachOutputObservers();
    void handleOutputDeleted(iGame::SurfaceMesh* output);
    void showMessage(const QString& text, bool information = false);

    igQtModelDrawWidget* m_RendererWidget{nullptr};
    igQtModelDialogWidget* m_ModelTreeWidget{nullptr};
    iGame::Model::Pointer m_SourceModel{};
    iGame::SurfaceMesh::Pointer m_InputMesh{};
    iGame::SurfaceMesh::Pointer m_LatestOutput{};

    QCheckBox* m_SkipIdentificationCheck{nullptr};
    QLineEdit* m_ObjectIdsNameEdit{nullptr};
    QLabel* m_NumberOfObjectsLabel{nullptr};
    QLabel* m_AllValidLabel{nullptr};
    QLabel* m_TotalAreaLabel{nullptr};
    QLabel* m_TotalVolumeLabel{nullptr};
    QLabel* m_StatusLabel{nullptr};
    std::function<void(iGame::DataObject*)> m_OutputReadyCallback{};
    std::vector<OutputBinding> m_OutputBindings{};
};
