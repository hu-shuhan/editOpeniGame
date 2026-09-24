#pragma once

// ============================================================================
// igQtPointSetToOctreeWidget —「点集转八叉树图像 (Point Set To Octree Image)」
// 参数面板
//
// 对应 VTK vtkPointSetToOctreeImageFilter / ParaView 的同名 Filter：
//   - 参数：每体素平均点数、是否处理点属性数组、单分量 Point Data 下拉框、
//           Last / Min / Max / Count / Sum / Mean 统计函数复选框；
//   - 执行前显示输入规模与预估体素数；
//   - 执行后显示 filter->GetMessage() 的诊断信息。
// ============================================================================

#include <IQCore/igQtExportModule.h>

#include <QWidget>

#include <Convert/iGamePointSetToOctreeFilter.h>
#include <iGameDataObject.h>

class QDockWidget;

namespace iGame {
class Model;
}

namespace Ui {
class igQtPointSetToOctree;
}

class IG_QT_MODULE_EXPORT igQtPointSetToOctreeWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtPointSetToOctreeWidget(QWidget* parent = nullptr);
    ~igQtPointSetToOctreeWidget() override;

    static QDockWidget* createDockWidget(QWidget* parent);

    /// 打开面板 / 切换模型时调用：刷新单分量点属性列表与规模提示。
    void setCurrentModel(iGame::Model* model);

signals:
    void closeRequested();
    void resultReady(iGame::DataObject::Pointer result);

private slots:
    void onRunClicked();
    void refreshInfo();

private:
    void initUI();
    void initConnections();
    void refreshPointArrayList();
    bool buildFilter(iGame::PointSetToOctreeFilter::Pointer& filter, QString& error);

    Ui::igQtPointSetToOctree* ui{nullptr};
    iGame::Model* m_currentModel{nullptr};
    iGame::DataObject::Pointer m_input;
    QString m_lastDiagnostic; ///< 上次执行的诊断信息，与规模提示一起显示
};
