#ifndef IGQTVARIABLEDENSITYWIDGET_H
#define IGQTVARIABLEDENSITYWIDGET_H

#include <QWidget>
#include <QRadioButton>
#include <iGameModel.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <QMouseEvent>
#include <QPainter>
#include <iGameVariableDensityData.h>
#include <QButtonGroup>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace iGame;
namespace Ui {
class igQtVariableDensityWidget;
}

class igQtVariableDensityWidget_VariableChooseButton : public QRadioButton {
    Q_OBJECT
public:
    explicit igQtVariableDensityWidget_VariableChooseButton(QWidget* parent = nullptr);
    int m_VariableIndex{};
};

class igQtVariableDensityWidget : public QWidget
{
    Q_OBJECT
private:
    Ui::igQtVariableDensityWidget *ui;

public:
    enum ImageShowDirection { Vertical = 0, Horizontal };

public:
    explicit igQtVariableDensityWidget(QWidget *parent = nullptr);
    ~igQtVariableDensityWidget();

    void SetModel(Model::Pointer model);

private:
    /* data choose */
    void RangeChooseObj(const QRect& chooseRange, const QRect& frameRange, std::vector<igIndex>& ids, IGenum& type);
    void EndRangeChoose();
    void StartRangeChoose(const QPoint& pos);
    void MoveRangeChooseEndPoint(const QPoint& pos);
    void DrawRangeChooseRect();
    bool m_RangeChooseOn{};
    QPoint m_RangeChooseStartPoint;
    QPoint m_RangeChooseEndPoint;
    bool m_RangeChoosing{};

private slots:
    void RangeChooseButtonClicked(bool checked);

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void paintEvent(QPaintEvent* QPE);
    void mouseMoveEvent(QMouseEvent* event);
    bool eventFilter(QObject* watched, QEvent* event);
    void handleMouseMove(const QPoint& pos);

private:
    /* main */
    void GenerateVariableDensityDatas();
    void SetUiData();
    void SetDataTypeChoose();
    void ClearVariableChoose();
    void GenerateVariableChoose();
    void ClearImage();
    void GenerateFirstDensityImage();
    void GenerateFirstChoosedDensityImage();
    void GenerateSecondDensityImage();
    void GenerateSecondChoosedDensityImage();
    void GenerateBackgroundColor();
    void Draw();
    void SetDensityColor();
    void SetChoosedDensityColor();
    void SetVariableNameLabel();
    void ClearVariableNameLabel();
    void SetVariablePosLabel(int x, int y);
    void ClearVariablePosLabel();
    void UpdateChoosedData(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope);
    void ClearChoosedData();

private:
    /* sub */
    VariableDensityData::Pointer _GenerateVariableDensityDatas(IGenum dataType);
    QImage
    _DrawDensityImage(int variableIndex, const std::vector<std::vector<int>>& density, int maxDensity,
                      const std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>& densityColor,
                      int alpha);
    void
    _DrawDensityImage(int variableIndex, const std::vector<std::vector<int>>& density, int maxDensity,
                      const std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>& densityColor,
                      int alpha, const QRect& drawFrame, std::shared_ptr<QPainter> painter);
    void _CalculateDrawFrame(int w, int h, QRect& drawFrame);
    void _CalculatePaintDrawFrame(QRect& bigDrawFrame, QRect& smallDrawFrame);
    void _CalculateFrameCenterCut(const QRect& frame, QRect& leftFrame, QRect& rightFrame, QRect& topFrame,
                                  QRect& bottomFrame);
    void _DrawCoordinateRect(const QRect& range);
    void _DrawCenterLine(const QRect& range);
    void _DrawBackground(const QRect& range);
    void _DrawImages(const QRect& range);
    void _DrawDensityRect(int density, int maxDensity, int copyIndex, int copyNum,
                          const std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>& color, int alpha,
                          const QRect& drawFrame, std::shared_ptr<QPainter> painter);
    bool _GetVariablePosMsg(int variableIndex, int x, int y, QRect& frame, double& value, int& densityNum,
                            int& choosedDensityNum);
    void _UpdateDataCopyNum();

private:
    void SetSelectionCallback();
    void SetClearSelectionCallback();

public:
    void SelectionCallbackEvent(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope);
    void ClearSelectionCallback();

private:
    Model::Pointer m_Model;
    UnstructuredMesh::Pointer m_Mesh;
    std::vector<VariableDensityData::Pointer> m_VariableDensityDatas;
    int m_CurrentModelDataIndex{-1};
    std::tuple<int, int, int> m_BackgroundColor{};

    QImage m_FirstDensityImage;          //Use small rect
    QImage m_FirstChoosedDensityImage; //Use small rect
    QImage m_FirstDensityImage_T;        //Use small rect
    QImage m_FirstChoosedDensityImage_T; //Use small rect

    QImage m_SecondDensityImage;//Use small rect
    QImage m_SecondChoosedDensityImage; //Use small rect
    QImage m_SecondDensityImage_T;      //Use small rect
    QImage m_SecondChoosedDensityImage_T; //Use small rect

    std::pair<int, int> m_CurrentShowVariable{};
    QButtonGroup m_VariableFirstChooseButtons;
    QButtonGroup m_VariableSecondChooseButtons;
    //std::vector<igQtVariableDensityWidget_VariableChooseButton*> m_VariableFirstChooseButtons;
    //std::vector<igQtVariableDensityWidget_VariableChooseButton*> m_VariableSecondChooseButtons;
    ImageShowDirection m_ImageShowDirection{};

    int m_BoxNum{};
    
private slots:
    void ChoosedAlphaSliderChanged(int value);
    void UnChoosedAlphaSliderChanged(int value);
    void ChoosedAlphaSpinBoxChanged(int value);
    void UnChoosedAlphaSpinBoxChanged(int value);
    void ChoosedLightSliderChanged(int value);
    void UnChoosedLightSliderChanged(int value);
    void ChoosedLightSpinBoxChanged(int value);
    void UnChoosedLightSpinBoxChanged(int value);
    void BoxNumSliderChanged(int value);
    void BoxNumSpinBoxChanged(int value);
private slots:
    void VariableFirstChooseButtonClicked(bool checked);
    void VariableSecondChooseButtonClicked(bool checked);
private slots:
    void DataChooseChanged(int choosedIndex);
    void FlipDirectionClicked();
    void RefreshData();
signals:
    void SIGNAL_RefreshDataClicked();
};

#endif // IGQTVARIABLEDENSITYWIDGET_H
