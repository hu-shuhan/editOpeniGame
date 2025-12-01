#ifndef IGQTDATACHANGEWIDGET_H
#define IGQTDATACHANGEWIDGET_H

#include <QWidget>
#include <iGameRadialStyle.h>
#include <iGameModel.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <iGameInteractor.h>
#include <iGamePlotLineData.h>
#include <QCheckBox>
#include <QMouseEvent>
#include <QPainter>
#include <mutex>
#include <memory>
#include <iGameScene.h>
using namespace iGame;
namespace Ui {
class igQtDataChangeWidget;
}

class igQtDataChangeWidget_CheckBox :public QCheckBox {
    Q_OBJECT
public:
    explicit igQtDataChangeWidget_CheckBox(QWidget* parent = nullptr);
    int m_VariableIndex{};
};

class igQtDataChangeWidget_ColorWidget : public QWidget {
    Q_OBJECT
public:
    explicit igQtDataChangeWidget_ColorWidget(QWidget* parent = nullptr);
    int m_VariableIndex{};
    std::tuple<int, int, int> m_Color;

protected:
    void paintEvent(QPaintEvent* QPE);
};

class igQtDataChangeWidget : public QWidget
{
    Q_OBJECT
private:
    Ui::igQtDataChangeWidget *ui;

public:
    explicit igQtDataChangeWidget(QWidget *parent = nullptr);
    ~igQtDataChangeWidget();

    void InitRadialStyle(SmartPointer<Interactor> interactor);

    RadialStyle::Pointer GetRadialStyle();

    void SetModel(Model::Pointer model);

    void SetInteractorName(const std::string& name);
    const std::string& GetInteractorName();

public:
    void SetScene(iGame::Scene* scene);

private:
    void TrySetRadialByScene();
    iGame::Scene* m_Scene{nullptr};

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
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* QPE) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void handleMouseMove(const QPoint& pos);//TODO

    void ClearPosLabel();
    void SetPosLabel(int x, int y);

protected:
    void hideEvent(QHideEvent* event) override;

signals:
    void Hided();

private:
    /* main */
    void SetRadialPoint();
    void SetRadialPoint(const iGame::BoundingBox& box);
    void ShowRadial(bool show);
    void DrawRadial();
    void GenerateBackgroundColor();
    void SetUiData();
    void SetDataTypeChoose();
    void ClearVariableChoose();
    void ResetVariableButton();
    void ResetVariableImage();
    void GenerateVariableImage();
    void GenerateChoosedVariableImage();
    void UpdateChoosedData(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope);
    void ClearChoosedData();
    void GenerateDataChangeDatas();
    void SetRadialData();//temp used by button
    void Draw();

private:
    /* sub */
    PlotLineData::Pointer _GenerateDataChangeDatas(IGenum dataType);
    void _SetRadialData(PlotLineData::Pointer Data);
    void _ClearVariableButton();
    void _SetVariableButton(int variableNum, const std::vector<std::string>& variableName);
    void _ClearVariableColorWidget();
    void _SetVariableColorWidget(int variableNum, const std::vector<std::tuple<int, int, int>>& variableColor);
    void _SetVariableColorWidgetColor(int variableNum, const std::vector<std::tuple<int, int, int>>& variableColor);
    void _ClearChoosedVariableColorWidget();
    void _SetChoosedVariableColorWidget(int variableNum, const std::vector<std::tuple<int, int, int>>& variableColor);
    void _SetChoosedVariableColorWidgetColor(int variableNum,
                                             const std::vector<std::tuple<int, int, int>>& variableColor);
    void _GenerateVariableImage(int variableIndex, PlotLineData::Pointer dataChangeData);
    void _GenerateChoosedVariableImage(int variableIndex, const PlotLineData::Pointer dataChangeData);
    void _GenerateVariableImage(const std::vector<bool>& variableShow,
                                PlotLineData::Pointer dataChangeData);
    void _GenerateChoosedVariableImage(const std::vector<bool>& variableShow,
                                       PlotLineData::Pointer dataChangeData);
    void _ResetVariableImage(int variableNum);
    void _SetLightUi(int unchoosedLight, int choosedLight);
    void _CalculateDrawFrame(int w, int h, QRect& drawFrame);
    void _CalculatePaintDrawFrame(QRect& bigDrawFrame, QRect& smallDrawFrame);
    QImage _DrawVariableImage(double minValue, double maxValue, double minDistance, double maxDistance,
                              const std::vector<int>& objDrawSort, int variableIndex,
                              const std::vector<double>& objDistance, const std::tuple<int, int, int>& color, int alpha,
                              const std::set<int> choosedObjIds, const std::map<int, int>& objIndexs,
                              CtxPresObjData_Main* theData);
    QImage _DrawChoosedVariableImage(double minValue, double maxValue, double minDistance, double maxDistance,
                                     const std::vector<int>& objDrawSort, int variableIndex,
                                     const std::vector<double>& objDistance, const std::tuple<int, int, int>& color,
                                     int alpha, const std::set<int> choosedObjIds, const std::map<int, int>& objIndexs,
                                     CtxPresObjData_Main* theData);
    void _DrawPoint(double minValue, double maxValue, double minDistance, double maxDistance, double value,
                    double distance, std::shared_ptr<QPainter> painter, const QRect& drawFrame);
    void _DrawBackground(const QRect& range);
    void _DrawCoordinateRect(const QRect& range);
    void _DrawImages(const QRect& range);

private:
    void SetSelectionCallback();
    void SetClearSelectionCallback();
    void SetRadialPointMoveCallBack();

public:
    void SelectionCallbackEvent(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope);
    void ClearSelectionCallback();
    void RadialPointMoveCallBack();

private:
    RadialStyle::Pointer m_RadialStyle;
    std::string m_RadialIntName;
    Model::Pointer m_Model;
    UnstructuredMesh::Pointer m_Mesh;
    std::vector<PlotLineData::Pointer> m_DataChangeDatas;
    int m_CurrentModelDataIndex{-1};
    std::tuple<int, int, int> m_BackgroundColor{};
    std::vector<QImage> m_VariableImages;
    std::vector<QImage> m_ChoosedVariableImages;
    std::vector<bool> m_VariableShow;
    std::vector<igQtDataChangeWidget_ColorWidget*> m_VariableColorWidgets;
    std::vector<igQtDataChangeWidget_ColorWidget*> m_ChoosedVariableColorWidgets;
    std::vector<igQtDataChangeWidget_CheckBox*> m_VariableCheckBoxs;

private slots:
    void ChoosedLightSliderChanged(int value);
    void UnChoosedLightSliderChanged(int value);
    void ChoosedLightSpinBoxChanged(int value);
    void UnChoosedLightSpinBoxChanged(int value);
private slots:
    void VariableCheckButtonClicked(bool checked);
private slots:
    void DataChooseChanged(int choosedIndex);
    void RefreshData();
    void DataGetToolClicked(bool checked);
    void TempSlot_SetRadialData();
signals:
    void SIGNAL_RefreshDataClicked();
};

#endif // IGQTDATACHANGEWIDGET_H
