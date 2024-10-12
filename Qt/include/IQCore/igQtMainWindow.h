/**
 * @class   igQtMainWindow
 * @brief   igQtMainWindow's brief
 */

#ifndef OPENIGAME_IGQTMAINWINDOW_H
#define OPENIGAME_IGQTMAINWINDOW_H

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

class IG_QT_MODULE_EXPORT igQtMainWindow : public QMainWindow {
    Q_OBJECT
public:
    igQtMainWindow(QWidget* parent = Q_NULLPTR);
    ~igQtMainWindow() override;

public:
    void initToolbarComponent();
    void initAllComponents();
    void initAllDockWidgetConnectWithAction();
    void initAllMySignalConnections();
    void initAllFilters();
    void initAllSources();
    void initAllInteractor();

public:
    igQtModelDrawWidget* rendererWidget;
    igQtFileLoader* fileLoader;
    igQtModelDialogWidget* modelTreeWidget;

    igQtColorManagerWidget* ColorManagerWidget;
    igQtFilterDialogDockWidget* filterDialogDockWidget;
    QDockWidget* SliceDockWidget;
    igQtModelClipWidget* SliceWidget;
    igQtProgressBarWidget* progressBarWidget;
    QComboBox* viewStyleCombox;
    QComboBox* attributeViewIndexCombox;
    QComboBox* attributeViewDimCombox;

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

private:
    Ui::MainWindow* ui;
};


#endif //OPENIGAME_IGQTMAINWINDOW_H
