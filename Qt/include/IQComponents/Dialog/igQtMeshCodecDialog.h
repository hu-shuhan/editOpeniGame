/**
 * @class   igMeshCodecDialog
 * @brief   网格编解码参数设置窗口
 */
#pragma once
#include <ui_igMeshCodecDialog.h>
#include <QtWidgets/qtablewidget.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qheaderview.h>
#include <QtWidgets/qtabwidget.h>
#include <QtWidgets/qcheckbox.h>
#include <QtCharts/QChart>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtWidgets/qbuttongroup.h>
#include <QtWidgets/qgraphicsproxywidget.h>
#include <QDialog>
#include <QListWidgetItem>
#include <QVector>
#include <QMap>
#include <QGridLayout>
#include <functional>
#include "iGameDataObject.h"
#include "../../iGameCore/Filters/iGameMeshCodec/iGameMeshCodecParamSet.h"
#include "../../iGameCore/Filters/iGameMeshCodec/iGameMeshCodecFeature.h"
#include "../../iGameCore/Filters/iGameMeshCodec/iGameMeshLoomEncoder.h"
#include "iGameSmartPointer.h"
#include "iGamePointSet.h"
#include <QMessageBox>
#include <QDoubleValidator>
#include <QDebug>
#include <algorithm>
#include <qfiledialog.h>
#include <qmessagebox.h>

QT_CHARTS_USE_NAMESPACE

class igQtMeshCodecDialog : public QDialog {
    Q_OBJECT
public:
    igQtMeshCodecDialog(QWidget* parent = Q_NULLPTR, iGame::DataObject::Pointer obj = nullptr);
    static void GenUiControlParams(iGame::UIControlParams& params);

signals:
    // 刷新直方图请求信号
    void refreshHistogramRequested(const QString& attributeName, const QString& analysisType);

private slots:
    void on_listAttributes_currentRowChanged(int currentRow);
    void on_tabDataDist_currentChanged(int index);
    
    void on_btnRefreshDataDist_clicked();
    void on_btnStartCompress_clicked();
    void on_btnCancel_clicked();
    void on_btnSetGlobalCompressMode_clicked();

    void on_radioLossless_toggled(bool checked);
    void on_radioDefaultErrorBound_toggled(bool checked);
    void on_radioKeyErrorBound_toggled(bool checked);

    void on_radioMantissaTruncation_toggled(bool checked);
    void on_radioLogQuantization_toggled(bool checked);

    void on_txtDefaultError_textChanged(const QString& text);
    void on_txtKeyError_textChanged(const QString& text);
    void on_txtNonKeyError_textChanged(const QString& text);

    void on_cbVisualizeError_stateChanged(int state);
    void on_cbShowReport_stateChanged(int state);

private:
    Ui::MeshCodecDialog* ui;
    iGame::DataObject::Pointer m_dataObj;
    iGame::UIControlParams m_params;
    std::string m_GeomName = "顶点坐标";

    // 属性列表
    int m_DataNum;
    int m_featureNum;
    int m_binNum = 8;

    // 标签页名称列表（保持顺序）
    QVector<QString> m_featureNames = {
        //"涡度",      // Vortex
        "梯度 Frobenius Norm",      // Gradient
        "拉普拉斯算子值 L2 Norm" // Laplacian
    };

    enum class FeatureHistoGenStatus {
        No,
        Cant,
        Yes
    };

    // 每种分析类型的直方图及复选框
    struct FeatureTab {
        QChartView* chartView;         // 图表视图
        QVector<QCheckBox*> checkBoxes; // 复选框列表
        QWidget* checkBoxContainer;
    };

    QVector<FeatureTab> m_featureTabs;

    enum class FeatureName {
        Gradient = 0,
        Laplacian = 1
    };

    //enum class FeatureName {
    //    Vortex = 0,
    //    Gradient = 1,
    //    Laplacian = 2
    //};

    struct AttrFeatureData {
        QChart* histogram;
        std::vector<bool> checkStatus;
        FeatureHistoGenStatus genStatus;

        std::vector<std::vector<int>> idInBins;
    };

    QVector<QVector<AttrFeatureData>> m_attrFeatureDatas;

private:
    void onCheckBoxStateChanged(int index, int state);

    bool IsVaildAttrIndex(int);

    bool IsVaildFeatureIndex(int);

    void InitIntro();

    void InitUIControlParams();

    void InitAttributeList();

    int GetCurrentFeatureIndex() const;

    int GetCurrentDataIndex() const;

    void ClearCurrentHistogram();

    void HideAllCheckBoxes();

    void DisableAllCheckBoxes();

    void EnableAllCheckBoxes();

    void LoadAllCheckBoxes();

    void LoadAttrFeatureWidget();

    void InitFeatureTabs();

    void InitAttrFeatureDatas();

    void SetupErrorInputValidators();

    void updateAttributeDisplay();

    void ShowReportDialog(const std::vector<std::pair<std::string, std::string>>& report);

    void DrawFeatureHistogram(QChart* chart);

    void CalFeatureHistogram(std::vector<float>& xAxis, std::vector<int>& yAxis);

    void FrobeniusNorm(const std::vector<std::vector<std::array<float, 3>>>& gradient, std::vector<float>& result);

    void L2Norm(const std::vector<std::vector<float>>& datas, std::vector<float>& result);
};