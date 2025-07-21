#ifndef IGQTVARIABLECORRELATIONWIDGET_H
#define IGQTVARIABLECORRELATIONWIDGET_H

#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QRadioButton>
#include <QRect>
#include <QWidget>
#include <iGameModel.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVariableCorrelationData.h>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace iGame;
namespace Ui
{
class igQtVariableCorrelationWidget;
}

class igQtVariableCorrelationWidget_VariableChooseButton : public QRadioButton {
    Q_OBJECT
public:
    explicit igQtVariableCorrelationWidget_VariableChooseButton(QWidget* parent = nullptr);
    int m_VariableIndex{};
};

class igQtVariableCorrelationWidget_VariableCorrelationLabel : public QLabel {
    Q_OBJECT
public:
    explicit igQtVariableCorrelationWidget_VariableCorrelationLabel(QWidget* parent = nullptr);
    int m_VariableIndex{};
};

class igQtVariableCorrelationWidget : public QWidget {
    Q_OBJECT

private:
    Ui::igQtVariableCorrelationWidget* ui;

public:
    explicit igQtVariableCorrelationWidget(QWidget* parent = nullptr);
    ~igQtVariableCorrelationWidget();

    void SetModel(Model::Pointer model);

protected:
    void paintEvent(QPaintEvent* QPE);
    void mouseMoveEvent(QMouseEvent* event);
    bool eventFilter(QObject* watched, QEvent* event);
    void handleMouseMove(const QPoint& pos);

private:
    /* main */
    void GenerateVariableCorrelationDatas();
    void SetUiData();
    void SetDataTypeChoose();
    void ClearMainVariableChoose();
    void GenerateMainVariableChoose();
    void ClearSubVariableChoose();
    void GenerateSubVariableChoose(int mainVariableIndex);
    void ClearImage();
    void GenerateCorImage();
    void GenerateChoosedCorImage();
    void GenerateBackgroundColor();
    void Draw();
    void SetVariableCorrelationDataColor(int variableIndex);
    void SetChoosedVariableCorrelationDataColor(int variableIndex);
    void SetMainSubNameLabel();
    void ClearMainSubNameLabel();
    void SetMainSubPosLabel(int x, int y);
    void ClearMainSubPosLabel();
    void UpdateChoosedData(const std::vector<Selection::Event>& _events);
    void ClearChoosedData();

private:
    /* sub */
    VariableCorrelationData::Pointer _GenerateVariableCorrelationDatas(IGenum dataType);
    QImage _DrawCorImage();
    QImage _DrawChoosedCorImage();
    void _CalculateDrawFrame(int w, int h, QRect& drawFrame);
    void _CalculatePaintDrawFrame(QRect& bigDrawFrame, QRect& smallDrawFrame);
    void _DrawCorImage(int mainVariableIndex, int subVariableIndex, const QRect& drawFrame,
                       std::shared_ptr<QPainter> painter);
    void _DrawChoosedCorImage(int mainVariableIndex, int subVariableIndex, const QRect& drawFrame,
                              std::shared_ptr<QPainter> painter);
    void _DrawCoordinate(const QRect& range);
    void _DrawBackground(const QRect& range);
    void _DrawImages(const QRect& range);
    void _DrawPoint(double mainVariableData, double subVariableData, double mainVariableMaxData,
                    double mainVariableMinData, double subVariableMaxData, double subVariableMinData,
                    const std::tuple<int, int, int>& color, int alpha, const QRect& drawFrame,
                    std::shared_ptr<QPainter> painter, int pointSize);

private:
    void SetSelectionCallback();
    void SetClearSelectionCallback();

public:
    void SelectionCallbackEvent(const std::vector<Selection::Event>& _events);
    void ClearSelectionCallback();

private:
    Model::Pointer m_Model;
    UnstructuredMesh::Pointer m_Mesh;
    std::vector<VariableCorrelationData::Pointer> m_VariableCorrelationDatas;
    int m_CurrentModelDataIndex{-1};
    std::tuple<int, int, int> m_BackgroundColor{};
    QImage m_CorImage;
    QImage m_ChoosedCorImage;
    bool m_ImageLoading{};
    std::mutex m_CorImageMutex;
    std::pair<int, int> m_CurrentShowVariable{};
    std::mutex m_CurrentShowVariableMutex;
    std::vector<igQtVariableCorrelationWidget_VariableChooseButton*> m_MainVariableChooseButtons;
    std::vector<igQtVariableCorrelationWidget_VariableCorrelationLabel*> m_VariableCorLabels;
    std::vector<igQtVariableCorrelationWidget_VariableCorrelationLabel*> m_ChoosedVariableCorLabels;
    std::vector<igQtVariableCorrelationWidget_VariableChooseButton*> m_SubVariableChooseButtons;
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
private slots:
    void MainVariableChooseButtonClicked(bool checked);
    void SubVariableChooseButtonClicked(bool checked);
private slots:
    void DataChooseChanged(int choosedIndex);
    void RefreshData();
    void WaitImageLoading();
    void CompleteImageLoading();
};

#endif // IGQTVARIABLECORRELATIONWIDGET_H
