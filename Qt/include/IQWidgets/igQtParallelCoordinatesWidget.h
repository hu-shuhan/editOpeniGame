/**
 * @class   igQtParallelCoordinatesWidget
 * @brief   igQtParallelCoordinatesWidget's brief
 */

#pragma once
#include <ui_ParallelCoordinatesView.h>
#include <IQCore/igQtExportModule.h>
#include <vector>
#include <utility>
#include <set>
#include <algorithm>
#include <map>
#include <string>
#include <QPainter>
#include <memory>
#include <iGameParallelCoordinatesData.h>
#include <IQComponents/igQtParallelCoordinatesObjectFilter.h>
#include <iGameModel.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <QImage>
#include <mutex>

using namespace iGame;
class IG_QT_MODULE_EXPORT igQtParallelCoordinatesWidget : public QWidget {

    Q_OBJECT

public:
    igQtParallelCoordinatesWidget(QWidget* parent = nullptr);
    ~igQtParallelCoordinatesWidget() override;

private:
    Ui::ParallelCoordinatesView* ui;

protected:
    void paintEvent(QPaintEvent* QPE);

public:
    void SetParallelCoordinates(Model::Pointer model);

private:
    //Updata data
    void UpdateData();
    void UpdateColor();
    void UpdateChoosedColor();
    void UpdateUnChoosedColor();
    void UpdateBackgroundColor();
    void UpdateChoosedData(const std::vector<Selection::Event>& _events);
    void ClearChoosedData();
    //Set Ui Data
    void SetUiData();
    //Set ComboBox
    void SetDataChoosedComboBox();
    void SetColorComboBox();
    //Generates
    std::vector<double> GetObjectData(IGenum dataType, int objId);
    void GenerateModelDatas();
    ParallelCoordinatesData::Pointer GeneratePointData();
    ParallelCoordinatesData::Pointer GenerateCellData();
    ParallelCoordinatesData::Pointer GenerateData(IGenum dataType);
    //Frame
    bool GetDrawFramePoints(int variableSortSize, std::vector<QRect>& variableMaxFontPoints,
                            std::vector<QRect>& variableMinFontPoints, std::vector<QRect>& variableNameFontPoints,
                            QRect& linkImageArea, QRect& background);
    //Filter
    void SetObjectFilters(const std::vector<int>& variableSort, const std::vector<std::string>& variableName,
                          const std::vector<double>& filterMaxValue, const std::vector<double>& filterMinValue);
    void ClearObjectFilters();
    bool ShoultBeFilted(const std::vector<double>& obj);
    std::vector<igQtParallelCoordinatesObjectFilter*> m_PcObjFilters;
    //Draw
    void DrawParallelCoordinates();
    int GetLinePointLocation(int top, int bottom, double currentValue, double maxValue, double minValue);
    void DrawBackground(const QRect& range);
    void DrawStrs(std::vector<QRect>& variableMaxFontPoints, std::vector<QRect>& variableMinFontPoints,
                  std::vector<QRect>& variableNameFontPoints);
    void DrawLinkImage(QRect& linkImageArea);
    //Choose
    bool IsChoosedObj(IGenum dataType, int objId);
    //Generate DrawImage
    void SetUpdateLinkImage();
    void UpdatingLinkImage();
    void UpdatingChoosedLinkImage();
    bool CalculateLinkPointInImage(int variableSortSize, int imageW, int imageH, std::vector<QPoint>& linkTopPoints,
                                   std::vector<QPoint>& linkBottomPoints);
    void GenerateDrawLinkImage(int leftLine, int rightLine, int top, int bottom, double leftValue, double rightValue,
                               double leftMaxValue, double leftMinValue, double rightMaxValue, double rightMinValue,
                               const std::shared_ptr<QPainter>& painter);
    void GenerateDrawLinksImage(std::vector<QPoint>& linkTopPoints, std::vector<QPoint>& linkBottomPoints,
                                const std::shared_ptr<QPainter>& painter);
    void GenerateChoosedDrawLinksImage(std::vector<QPoint>& linkTopPoints, std::vector<QPoint>& linkBottomPoints,
                                       const std::shared_ptr<QPainter>& painter);

private:
    void SetSelectionCallback();
    void SetClearSelectionCallback();

public:
    void SelectionCallbackEvent(const std::vector<Selection::Event>& _events);
    void ClearSelectionCallback();

private:
    Model::Pointer m_Model;
    UnstructuredMesh::Pointer m_Mesh;
    std::vector<ParallelCoordinatesData::Pointer> m_ParallelCoordinatesDatas;
    int m_CurrentModelDataIndex{-1};
    int m_ColorVariableIndex{-1};
    std::tuple<int, int, int> m_BackgroundColor{};
    QImage m_LinkImage;
    QImage m_ChoosedLinkImage;
    bool m_ImageLoading{};
    std::mutex m_LinkImageMutex;
signals:
    void SIGNAL_WaitImageLoading();
    void SIGNAL_CompleteImageLoading();

private slots:
    void ChoosedAlphaSliderChanged(int value);
    void UnChoosedAlphaSliderChanged(int value);
    void ChoosedAlphaSpinBoxChanged(int value);
    void UnChoosedAlphaSpinBoxChanged(int value);
    void ChoosedLightSliderChanged(int value);
    void UnChoosedLightSliderChanged(int value);
    void ChoosedLightSpinBoxChanged(int value);
    void UnChoosedLightSpinBoxChanged(int value);
public slots:
    void FilterMaxValueChanged(int number, double value);
    void FilterMinValueChanged(int number, double value);
private slots:
    void DataChooseChanged(int choosedIndex);
    void ColorChooseChanged(int choosedIndex);
    void RefreshData();
    void SetVariableSort();
    void GetVariableSortFromDialog(const std::vector<int>& choosedSort);
    void WaitImageLoading();
    void CompleteImageLoading();
};
