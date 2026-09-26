#pragma once

// ============================================================================
// igQtResampleToImageWidget —「重采样到图像 (Resample To Image)」参数面板
//
// 对应 VTK vtkResampleToImage / ParaView 的 Resample To Image：
//   - 参数：采样维度（各轴格点数）、是否使用输入包围盒、显式采样范围、
//           遇到不支持的单元类型是否终止、是否对 ID/整型数组禁用线性插值；
//   - 执行前显示规模：输入点数/单元数、单元类型统计（受支持/不受支持）、
//     以及预估输出格点数/单元数与内存量级；规模过大时弹窗二次确认；
//   - 执行后把 filter->GetMessage() 的诊断信息显示在面板上，避免静默的
//     不完整结果（不支持的单元类型、同名数组冲突、离散数组处理方式等）。
// ============================================================================

#include <IQCore/igQtExportModule.h>

#include <QWidget>

#include <Convert/iGameResampleToImageFilter.h>
#include <iGameDataObject.h>

class QDockWidget;

namespace iGame {
class Model;
}

namespace Ui {
class igQtResampleToImage;
}

class IG_QT_MODULE_EXPORT igQtResampleToImageWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtResampleToImageWidget(QWidget* parent = nullptr);
    ~igQtResampleToImageWidget() override;

    static QDockWidget* createDockWidget(QWidget* parent);

    /// 打开面板 / 切换模型时调用：刷新输入规模、单元类型统计与预估输出规模。
    void setCurrentModel(iGame::Model* model);

signals:
    void closeRequested();
    void resultReady(iGame::DataObject::Pointer result);

private slots:
    void onRunClicked();
    void refreshEstimate();

private:
    void initUI();
    void initConnections();
    bool buildFilter(iGame::ResampleToImageFilter::Pointer& filter, QString& error);

    Ui::igQtResampleToImage* ui{nullptr};
    iGame::Model* m_currentModel{nullptr};
    iGame::DataObject::Pointer m_input;
    iGame::DataObject::Pointer m_meshData; ///< 统一转换后的 UnstructuredMesh（用于单元类型统计）
    QString m_lastDiagnostic;              ///< 上次执行的诊断信息，与预估信息一起显示
};
