#pragma once

#include <IQCore/igQtExportModule.h>
#include <TimeSeries/iGameExtractTimeStepsFilter.h>
#include <iGameDataObject.h>

#include <QWidget>

#include <vector>

class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTableWidgetItem;

class IG_QT_MODULE_EXPORT igQtExtractTimeStepsWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtExtractTimeStepsWidget(QWidget* parent = nullptr);
    ~igQtExtractTimeStepsWidget() override = default;

    void SetOriginDataObject(iGame::DataObject::Pointer object);

signals:
    void ExtractTimeStepsApplied(iGame::DataObject::Pointer output);

private slots:

    void Apply();

    void Reset();

    void UpdateModeUi();

private:
    void BuildUi();
    void RebuildTimeStepTable();
    void SetStatus(const QString& text, bool isError = false);
    void UpdateStatusPreview();
    void AppendIndexRow(int index = -1);
    void RefreshTimeValueCell(int row);
    bool CollectIndices(std::vector<int>& indices);

    iGame::DataObject::Pointer m_InputObject;
    iGame::ExtractTimeStepsFilter::Pointer m_Filter;

    QLabel* m_infoLabel{nullptr};
    QComboBox* m_modeCombo{nullptr};
    QWidget* m_indexPanel{nullptr};
    QTableWidget* m_indexTable{nullptr}; 
    QPushButton* m_addIndexButton{nullptr};
    QPushButton* m_removeIndexButton{nullptr};
    QPushButton* m_clearIndicesButton{nullptr};
    QWidget* m_rangePanel{nullptr};     
    QSpinBox* m_beginSpin{nullptr};
    QSpinBox* m_endSpin{nullptr};
    QSpinBox* m_intervalSpin{nullptr};
    QLabel* m_statusLabel{nullptr};     
    QPushButton* m_applyButton{nullptr};
    QPushButton* m_resetButton{nullptr};
};
