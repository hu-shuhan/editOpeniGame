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
#include <ParallelCoordinatesFilters/iGameParallelCoordinatesData.h>
#include <IQComponents/igQtParallelCoordinatesObjectFilter.h>

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
    void SetParallelCoordinates(ParallelCoordinatesData::Pointer parallelCoordinatesData);

private:
    ParallelCoordinatesData::Pointer m_parallelCoordinatesData;

private:
    void SetObjectFilters(int variableNum, const std::vector<std::string>& variableName,
                          const std::vector<double>& maxValueInVariables,
                          const std::vector<double>& minValueInVariables);
    void ClearObjectFilters();
    bool ShoultBeFilted(int variableNum, const std::vector<double>& obj);
    std::vector<double> m_FilterMaxValue, m_FilterMinValue;
    std::vector<igQtParallelCoordinatesObjectFilter*> m_PcObjFilters;
    QSpacerItem* m_SpaceItem{nullptr};
public slots:
    void FilterMaxValueChanged(int number, double value);
    void FilterMinValueChanged(int number, double value);

private:
    bool SetParallelCoordinates(int variableNum, const std::vector<std::string>& variableName,
                                const std::vector<std::vector<double>>& linkStrings,
                                const std::vector<bool>& linkStringsChooseCondition,
                                const std::vector<double>& maxValueInVariables,
                                const std::vector<double>& minValueInVariables);

    bool GetDrawFramePoints(int variableNum, std::vector<QRect>& variableMaxFontPoints,
                            std::vector<QRect>& variableMinFontPoints, std::vector<QRect>& variableNameFontPoints,
                            std::vector<QPoint>& linkTopPoints, std::vector<QPoint>& linkBottomPoints);
    std::shared_ptr<QPainter> GetLinePainter(const QColor& color);
    int GetLinePointLocation(int top, int bottom, double currentValue, double maxValue, double minValue);
    void DrawLink(int leftLine, int rightLine, int top, int bottom, double leftValue, double rightValue,
                  double leftMaxValue, double leftMinValue, double rightMaxValue, double rightMinValue,
                  const QColor& color);
    void DrawLinks(int variableNum, const std::vector<std::vector<double>>& linkStrings,
                   const std::vector<bool>& linkStringsChooseCondition, std::vector<QPoint>& linkTopPoints,
                   std::vector<QPoint>& linkBottomPoints, const std::vector<double>& maxValueInVariables,
                   const std::vector<double>& minValueInVariables, const QColor& choosedColor,
                   const QColor& unChoosedColor);
    void DrawStrs(int variableNum, const std::vector<std::string>& variableName,
                  const std::vector<double>& maxValueInVariables, const std::vector<double>& minValueInVariables,
                  std::vector<QRect>& variableMaxFontPoints, std::vector<QRect>& variableMinFontPoints,
                  std::vector<QRect>& variableNameFontPoints);
    int m_ChoosedAlpha{10};
    int m_UnChoosedAlpha{10};
private slots:
    void ChoosedAlphaSliderChanged(int value);
    void UnChoosedAlphaSliderChanged(int value);
    void ChoosedAlphaSpinBoxChanged(int value);
    void UnChoosedAlphaSpinBoxChanged(int value);
};
