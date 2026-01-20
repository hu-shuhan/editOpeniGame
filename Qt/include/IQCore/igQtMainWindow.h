/**
 * @class   igQtMainWindow
 * @brief   igQtMainWindow's brief
 */

#ifndef IGAMEVIS_IGQTMAINWINDOW_H
#define IGAMEVIS_IGQTMAINWINDOW_H

#define QT_NO_OPENGL
#include <ui_iGameQtMainWindow.h>
#if __linux__
#include <QTabWidget>
#include <QtGui>
#else
#include <QtGUI/QtGui>
#include <QtWidgets/Qtabwidget.h>
#endif
#include <IQCore/igQtExportModule.h>
#include <QtWidgets/QMainWindow>
#undef QT_NO_OPENGL

class igQtModelDrawWidget;
class igQtFileLoader;
class igQtColorManagerWidget;
class igQtFilterDialogDockWidget;
class igQtSliceWidget;
class igQtProgressBarWidget;
class igQtModelDialogWidget;
class igQtModelClipWidget;
class igQtDeformationWidget;
class igQtAiChatWidget;
class igQtCommandManager;

class IG_QT_MODULE_EXPORT igQtMainWindow : public QMainWindow {
    Q_OBJECT
public:
    igQtMainWindow(QWidget* parent = Q_NULLPTR);
    ~igQtMainWindow() override;

public:
    void initAllUnDefinedComponents();
    void initToolbarComponent();
    void initAllComponents();
    void initAllDockWidgetConnectWithAction();
    void initAllMySignalConnections();
    void initAllFilters();
    void initAllSources();
    void initAllInteractor();

    void initArgs(const QStringList& args);
    void updateVortexMetricsLabelPos();

public:
    igQtModelDrawWidget* rendererWidget;
    igQtFileLoader* fileLoader;
    igQtModelDialogWidget* modelTreeWidget;

    igQtColorManagerWidget* ColorManagerWidget;
    igQtFilterDialogDockWidget* filterDialogDockWidget;
    QDockWidget* SliceDockWidget;
    QDockWidget* ContourDockWidget;
    igQtModelClipWidget* SliceWidget;
    QDockWidget* DeformationDockWidget;
    igQtDeformationWidget* DeformationWidget;

    igQtProgressBarWidget* progressBarWidget;
    QComboBox* viewStyleCombox;
    QComboBox* attributeViewIndexCombox;
    QComboBox* attributeViewDimCombox;
    
    // AI Chat DockWidget
    QDockWidget* aiChatDockWidget;
    igQtAiChatWidget* aiChatWidget;
    
    // Command Manager for MCP Server (端口 12345)
    igQtCommandManager* commandManager;

private slots:
    void updateRecentFilePaths();
    void updateColorBarShow();

    //void ChangeViewStyle();
    //void ChangeScalarView();
    //void ChangeScalarViewDim();
    //void updateViewStyleAndCloudPicture();
    //void updateCurrentDataObject();
    //void updateCurrentSceneWidget();

    void UpdateRenderingWidget();
    //void changePointSelectionInteractor();
    //void changePointsSelectionInteractor();
    //void changeFaceSelectionInteractor();
    //void changeFacesSelectionInteractor();
    void UpdateIcons();
    QString LoadExternalFonts();


private:
    Ui::MainWindow* ui;
    QLabel* vortexMetricsLabel = nullptr;
};


#endif //IGAMEVIS_IGQTMAINWINDOW_H
