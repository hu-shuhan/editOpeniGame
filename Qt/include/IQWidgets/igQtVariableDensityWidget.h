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
    explicit igQtVariableDensityWidget(QWidget *parent = nullptr);
    ~igQtVariableDensityWidget();

    void SetModel(Model::Pointer model);

protected:
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
    void GenerateDensityImage();
    void GenerateChoosedDensityImage();
    void GenerateBackgroundColor();
    void Draw();
    void SetDensityColor();
    void SetChoosedDensityColor();
    void SetVariableNameLabel();
    void ClearVariableNameLabel();
    void SetVariablePosLabel(int x, int y);
    void ClearVariablePosLabel();
    void UpdateChoosedData(const std::vector<Selection::Event>& _events);
    void ClearChoosedData();

private:
    /* sub */
    VariableDensityData::Pointer _GenerateVariableDensityDatas(IGenum dataType);
    QImage _DrawDensityImage();
    QImage _DrawChoosedDensityImage();
    void _CalculateDrawFrame(int w, int h, QRect& drawFrame);
    void _CalculatePaintDrawFrame(QRect& bigDrawFrame, QRect& smallDrawFrame);
    void _DrawDensityImage(int variableIndex, const QRect& drawFrame, std::shared_ptr<QPainter> painter);
    void _DrawChoosedDensityImage(int variableIndex, const QRect& drawFrame, std::shared_ptr<QPainter> painter);
    void _DrawCoordinate(const QRect& range);
    void _DrawBackground(const QRect& range);
    void _DrawImages(const QRect& range);
    void _DrawDensityRect(int density, int maxDensity, int copyIndex, int copyNum,
                          const std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>& color, int alpha,
                          const QRect& drawFrame, std::shared_ptr<QPainter> painter);

private:
    void SetSelectionCallback();
    void SetClearSelectionCallback();

public:
    void SelectionCallbackEvent(const std::vector<Selection::Event>& _events);
    void ClearSelectionCallback();

private:
    Model::Pointer m_Model;
    UnstructuredMesh::Pointer m_Mesh;
    std::vector<VariableDensityData::Pointer> m_VariableDensityDatas;
    int m_CurrentModelDataIndex{-1};
    std::tuple<int, int, int> m_BackgroundColor{};
    QImage m_DensityImage;//Use small rect
    QImage m_ChoosedDensityImage;//Use small rect
    int m_CurrentShowVariable{};
    std::vector<igQtVariableDensityWidget_VariableChooseButton*> m_VariableChooseButtons;
    
private slots:
    void ChoosedAlphaSliderChanged(int value);
    void UnChoosedAlphaSliderChanged(int value);
    void ChoosedAlphaSpinBoxChanged(int value);
    void UnChoosedAlphaSpinBoxChanged(int value);
    void ChoosedLightSliderChanged(int value);
    void UnChoosedLightSliderChanged(int value);
    void ChoosedLightSpinBoxChanged(int value);
    void UnChoosedLightSpinBoxChanged(int value);
private slots:
    void VariableChooseButtonClicked(bool checked);
private slots:
    void DataChooseChanged(int choosedIndex);
    void RefreshData();
};

#endif // IGQTVARIABLEDENSITYWIDGET_H
