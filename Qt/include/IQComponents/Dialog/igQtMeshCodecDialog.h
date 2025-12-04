/**
 * @class   igMeshCodecDialog
 * @brief   网格编解码参数设置窗口
 */
#pragma once
#include "MeshCodec/Utils/iGameMeshCodecFeature.h"
#include "MeshCodec/iGameMeshEncoderFilter.h"
#include "iGameDataObject.h"
#include "iGamePointSet.h"
#include "iGameSmartPointer.h"
#include <QDebug>
#include <QDialog>
#include <QDoubleValidator>
#include <QGridLayout>
#include <QListWidgetItem>
#include <QMap>
#include <QMessageBox>
#include <QVector>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qbuttongroup.h>
#include <QtWidgets/qcheckbox.h>
#include <QtWidgets/qgraphicsproxywidget.h>
#include <QtWidgets/qheaderview.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qstackedwidget.h>
#include <QtWidgets/qtablewidget.h>
#include <QtWidgets/qtabwidget.h>

// forward declarations for pointer parameters
class QComboBox;
class QLabel;
#include "IGC/iGameIGCWriter.h"
#include <algorithm>
#include <functional>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <ui_igMeshCodecDialog.h>

// --------------------------------------------------------------------------------------
// UI 数据类别枚举（仅用于 UI 层）
enum class UIDataCategory {
    AllData,    // 全部数据（虚拟项，用于批量设置）
    Geom,       // 顶点坐标
    Attr        // 属性数据
};

// UI 数据项模型（纯 UI 层数据结构）
struct UIDataItem {
    // 基本信息
    UIDataCategory category = UIDataCategory::AllData;
    QString displayName;
    int attrIndex = -1;         // 仅当 category == Attr 时有效

    // 量化参数（UI 状态）
    iGame::QuantizeMode errorMode = iGame::QuantizeMode::None;
    int globalQuantizeLevel = 0;
    int criticalQuantizeLevel = 0;
    int normalQuantizeLevel = 0;

    // 元素信息（用于直方图计算）
    IGsize elementCount = 0;
    int dimension = 0;

    // 关键元素标记
    std::vector<bool> isKeyElement;

    bool isAllData() const { return category == UIDataCategory::AllData; }
    bool isGeom() const { return category == UIDataCategory::Geom; }
    bool isAttr() const { return category == UIDataCategory::Attr; }
};

Q_DECLARE_METATYPE(UIDataItem*)

QT_CHARTS_USE_NAMESPACE

class igQtMeshCodecDialog : public QDialog {
    Q_OBJECT
public:
    igQtMeshCodecDialog(QWidget* parent = Q_NULLPTR, iGame::DataObject::Pointer obj = nullptr);

signals:
    // 刷新直方图请求信号
    void refreshHistogramRequested(const QString& attributeName, const QString& analysisType);

private slots:
    void on_combo_boxFloatSelect_currentIndexChanged(int index);

    void on_btnStartCompress_clicked();
    void on_button_cancel_clicked();

    void on_radio_losslessMode_toggled(bool checked);
    void on_radio_globalMode_toggled(bool checked);
    void on_radio_areaModel_toggled(bool checked);

    // 原有输入框事件处理函数，已注释
    // void on_txtDefaultError_textChanged(const QString& text);
    // void on_txtKeyError_textChanged(const QString& text);
    // void on_txtNonKeyError_textChanged(const QString& text);

    // ComboBox量化等级选择事件处理函数
    void on_comboBox_globalLevel_currentIndexChanged(int index);
    void on_comboBox_criticalLevel_currentIndexChanged(int index);
    void on_comboBox_normalLevel_currentIndexChanged(int index);

    void on_checkbox_showReport_stateChanged(int state);
    void on_comboBox_compressLevel_currentIndexChanged(int index);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    Ui::MeshCodecDialog* ui;
    iGame::DataObject::Pointer m_dataObj;

    // UI 数据模型
    QVector<UIDataItem> m_uiDataItems;      // UI 数据项列表
    bool m_showReport = false;
    int m_compressLevel = 11;

    // 直方图分箱数量：固定为 10 个 bin
    int m_binNum = 10;

    QVector<QString> m_quantizeLevel = {"无损", "低", "中", "高"};

    // 压缩模式标记：无损/全局/分区
    // 无损：黑色实心星(★)；全局：▲；分区：■
    QVector<QString> m_modeMark = {
            "★", // 无损
            "▲", // 全局
            "■"  // 分区
    };

    enum class FeatureHistoGenStatus { No, Cant, Yes };

    // 梯度直方图相关（使用每项各自的 QChart* 缓存）
    QChartView* m_chartView = nullptr;      // 图表视图
    QVector<QCheckBox*> m_checkBoxes;       // 复选框列表
    QWidget* m_checkBoxContainer = nullptr; // 复选框容器
    QPushButton* m_refreshButton = nullptr; // 悬浮的刷新按钮
    // 用堆叠页切换关键区域（空页不占高度）
    QStackedWidget* m_keyAreaStack = nullptr;
    QWidget* m_keyAreaEmptyPage = nullptr;
    int m_keyAreaLayoutIndex = -1; // 在父布局中的位置

    // 统一的尺寸重计算
    void RecomputeDialogSize();

    // 每个属性数据的梯度特征数据
    struct AttrFeatureData {
        std::vector<bool> checkStatus;
        FeatureHistoGenStatus genStatus;

        std::vector<std::vector<int>> idInBins;
        // 缓存直方图的 x/y 轴数据，切换时复用避免重复计算
        std::vector<float> xAxis;
        std::vector<int> yAxis;
    };

    QVector<AttrFeatureData> m_attrFeatureDatas;

private:
    void onCheckBoxStateChanged(int index, int state);

    void InitIntro();

    void InitDataItems();

    void InitAttributeList();

    // 计算不同模式对应的mark
    QString GetModeMark(iGame::QuantizeMode mode) const;
    // 刷新某一项Combo的前缀mark
    void RefreshComboItemMark(int dataIndex);
    // 刷新全部Combo项的前缀mark
    void RefreshAllComboItemMarks();

    // 获取当前选中项的 UIDataItem 指针
    UIDataItem* GetCurrentDataItem();
    const UIDataItem* GetCurrentDataItem() const;

    // 桥接器：将 UI 数据模型转换为编码器参数
    iGame::CodecControlParams BuildCodecParams() const;

    void ClearCurrentHistogram();

    void HideAllCheckBoxes();

    void DisableAllCheckBoxes();

    void EnableAllCheckBoxes();

    void LoadAllCheckBoxes();

    void LoadAttrFeatureWidget();

    void InitHistogramView();

    void InitAttrFeatureDatas();

    void GenerateHistogram();

    void ShowRefreshButton();

    void HideRefreshButton();

    void PositionRefreshButton();

    // 根据模式显示/隐藏“关键区域选择”并自适应窗口高度
    void UpdateKeyAreaVisibility(bool show);
    // 统一设置某组量化控件使能/禁用及文字灰化
    void SetComboAndLabelEnabled(QComboBox* combo, QLabel* label, bool enabled);
    // 切换三种模式时统一更新三个量化区域的使能与样式
    void ApplyModeUI(bool lossless, bool global, bool area);
    // 根据当前数据索引与模式，统一更新“产生直方图”按钮显示/隐藏
    void UpdateRefreshButtonStateForCurrent();
    // 根据参数的 errorMode 同步三种单选按钮（防止触发多余信号）
    void SetRadiosFromErrorMode(iGame::QuantizeMode mode);

    // void SetupErrorInputValidators(); // 原有验证函数，已注释

    // 应用设置到全体数据或当前数据的辅助函数
    template<typename Func>
    void ApplySettingToData(Func setter);

    void ShowReportDialog(const std::vector<std::pair<std::string, std::string>>& report);

    void DrawFeatureHistogram(QChart* chart);
    void DrawFeatureHistogramFromData(QChart* chart, const std::vector<float>& xAxis, const std::vector<int>& yAxis);

    void CalFeatureHistogram(std::vector<float>& xAxis, std::vector<int>& yAxis);

    void FrobeniusNorm(const std::vector<std::vector<std::array<float, 3>>>& gradient, std::vector<float>& result);
    void SpectralNorm(const std::vector<std::vector<std::array<float, 3>>>& gradient, std::vector<float>& result);

    void L2Norm(const std::vector<std::vector<float>>& datas, std::vector<float>& result);

    // 旧实现无需根据非均匀 bin 调整复选框列
    // 将当前属性的分箱勾选状态应用到关键元素掩码
    void ApplyCheckStatusToKeyElements(int dataIndex);
};
