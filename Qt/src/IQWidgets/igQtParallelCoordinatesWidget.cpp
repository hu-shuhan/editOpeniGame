#include "iGameSceneManager.h"
#include <IQWidgets/igQtParallelCoordinatesWidget.h>
#include <iomanip>
/**
 * @class   igQtParallelCoordinatesWidget
 * @brief   igQtParallelCoordinatesWidget's brief
 */
using namespace std;
igQtParallelCoordinatesWidget::igQtParallelCoordinatesWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::ParallelCoordinatesView) {
    ui->setupUi(this);
    connect(ui->choosedAlphaSlider, &QSlider::valueChanged, this,
            &igQtParallelCoordinatesWidget::ChoosedAlphaSliderChanged);
    connect(ui->choosedAlphaSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,  
           &igQtParallelCoordinatesWidget::ChoosedAlphaSpinBoxChanged);
    connect(ui->unChoosedAlphaSlider, &QSlider::valueChanged, this,
            &igQtParallelCoordinatesWidget::UnChoosedAlphaSliderChanged);
    connect(ui->unChoosedAlphaSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtParallelCoordinatesWidget::UnChoosedAlphaSpinBoxChanged);
    ui->choosedAlphaSlider->setValue(m_ChoosedAlpha);
    ui->choosedAlphaSpinBox->setValue(m_ChoosedAlpha);
    ui->unChoosedAlphaSlider->setValue(m_UnChoosedAlpha);
    ui->unChoosedAlphaSpinBox->setValue(m_UnChoosedAlpha);
}
igQtParallelCoordinatesWidget::~igQtParallelCoordinatesWidget() {
    //ClearObjectFilters();
}

void igQtParallelCoordinatesWidget::paintEvent(QPaintEvent* QPE) {
    SetParallelCoordinates(
            m_parallelCoordinatesData->GetVariableNum(), m_parallelCoordinatesData->GetVariableName(),
            m_parallelCoordinatesData->GetLinkStrings(), m_parallelCoordinatesData->GetLinkStringChooseCondition(),
            m_parallelCoordinatesData->GetMaxValueInVariables(), m_parallelCoordinatesData->GetMinValueInVariables());
}

void igQtParallelCoordinatesWidget::SetParallelCoordinates(ParallelCoordinatesData::Pointer parallelCoordinatesData) {
    m_parallelCoordinatesData = parallelCoordinatesData;
    ClearObjectFilters();
    SetObjectFilters(m_parallelCoordinatesData->GetVariableNum(), m_parallelCoordinatesData->GetVariableName(),
                     m_parallelCoordinatesData->GetMaxValueInVariables(),
                     m_parallelCoordinatesData->GetMinValueInVariables());
    update();
}

void igQtParallelCoordinatesWidget::SetObjectFilters(int variableNum, const std::vector<std::string>& variableName,
                                                     const std::vector<double>& maxValueInVariables,
                                                     const std::vector<double>& minValueInVariables) {
    m_FilterMaxValue = maxValueInVariables;
    m_FilterMinValue = minValueInVariables;
    for (int i = 0; i < variableNum; i++) {
        igQtParallelCoordinatesObjectFilter* pcObjFilter = new igQtParallelCoordinatesObjectFilter(
                i, maxValueInVariables[i], minValueInVariables[i], variableName[i], this);
        connect(pcObjFilter, &igQtParallelCoordinatesObjectFilter::ChangeMaxValue, this,
                &igQtParallelCoordinatesWidget::FilterMaxValueChanged);
        connect(pcObjFilter, &igQtParallelCoordinatesObjectFilter::ChangeMinValue, this,
                &igQtParallelCoordinatesWidget::FilterMinValueChanged);
        m_PcObjFilters.push_back(pcObjFilter);
        ui->coreHorizontalLayout->addWidget(pcObjFilter);
    }
    m_SpaceItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    ui->coreHorizontalLayout->addItem(m_SpaceItem);
}

void igQtParallelCoordinatesWidget::ClearObjectFilters() {
    for (auto& pcObjFilter: m_PcObjFilters) { pcObjFilter->deleteLater(); }
    m_PcObjFilters.clear();
    if (m_SpaceItem != nullptr) {
        delete m_SpaceItem;
        m_SpaceItem = nullptr;
    }
}

bool igQtParallelCoordinatesWidget::ShoultBeFilted(int variableNum, const std::vector<double>& obj) {
    for (int i = 0; i < variableNum; i++) {
        if (obj[i] < m_FilterMinValue[i] || m_FilterMaxValue[i] < obj[i]) return true;
    }
    return false;
}

void igQtParallelCoordinatesWidget::FilterMaxValueChanged(int number, double value) {
    m_FilterMaxValue[number] = value;
    update();
}

void igQtParallelCoordinatesWidget::FilterMinValueChanged(int number, double value) {
    m_FilterMinValue[number] = value;
    update();
}

bool igQtParallelCoordinatesWidget::SetParallelCoordinates(int variableNum,
                                                           const std::vector<std::string>& variableName,
                                                           const std::vector<std::vector<double>>& linkStrings,
                                                           const std::vector<bool>& linkStringsChooseCondition,
                                                           const std::vector<double>& maxValueInVariables,
                                                           const std::vector<double>& minValueInVariables) {
    std::vector<QRect> variableMaxFontPoints;
    std::vector<QRect> variableMinFontPoints;
    std::vector<QRect> variableNameFontPoints;
    std::vector<QPoint> linkTopPoints;
    std::vector<QPoint> linkBottomPoints;
    bool drawAble = GetDrawFramePoints(variableNum, variableMaxFontPoints, variableMinFontPoints,
                                       variableNameFontPoints, linkTopPoints, linkBottomPoints);
    if (!drawAble) return false;
    DrawStrs(variableNum, variableName, maxValueInVariables, minValueInVariables, variableMaxFontPoints,
             variableMinFontPoints, variableNameFontPoints);
    DrawLinks(variableNum, linkStrings, linkStringsChooseCondition, linkTopPoints, linkBottomPoints,
              maxValueInVariables, minValueInVariables, QColor(255, 0, 0, m_ChoosedAlpha),
              QColor(0, 0, 255, m_UnChoosedAlpha));
    return true;
}

std::shared_ptr<QPainter> igQtParallelCoordinatesWidget::GetLinePainter(const QColor& color) {
    std::shared_ptr<QPainter> painter = make_shared<QPainter>(this);
    painter->setPen(QPen(color, 1));
    return painter;
}

int igQtParallelCoordinatesWidget::GetLinePointLocation(int top, int bottom, double currentValue, double maxValue,
                                                        double minValue) {
    if (maxValue == minValue) return (top + bottom) / 2;
    return (currentValue - minValue) * (top - bottom) / (maxValue - minValue) + bottom;
}

void igQtParallelCoordinatesWidget::DrawLink(int leftLine, int rightLine, int top, int bottom, double leftValue,
                                             double rightValue, double leftMaxValue, double leftMinValue,
                                             double rightMaxValue, double rightMinValue, const QColor& color) {
    auto painter = GetLinePainter(color);
    int leftPoint = GetLinePointLocation(top, bottom, leftValue, leftMaxValue, leftMinValue);
    int rightPoint = GetLinePointLocation(top, bottom, rightValue, rightMaxValue, rightMinValue);
    painter->drawLine(leftLine, leftPoint, rightLine, rightPoint);
}

void igQtParallelCoordinatesWidget::DrawLinks(int variableNum, const std::vector<std::vector<double>>& linkStrings,
                                              const std::vector<bool>& linkStringsChooseCondition,
                                              std::vector<QPoint>& linkTopPoints, std::vector<QPoint>& linkBottomPoints,
                                              const std::vector<double>& maxValueInVariables,
                                              const std::vector<double>& minValueInVariables,
                                              const QColor& choosedColor, const QColor& unChoosedColor) {
    vector<int> choosedLinkStringIndexs;
    if (variableNum == 1) {
        for (int linkStrIndex = 0; linkStrIndex < linkStrings.size(); linkStrIndex++) {
            auto& linkStr = linkStrings[linkStrIndex];
            if (ShoultBeFilted(variableNum, linkStr)) continue;
            if (linkStringsChooseCondition[linkStrIndex]) {
                choosedLinkStringIndexs.push_back(linkStrIndex);
                continue;
            }
            DrawLink(linkTopPoints.front().x(), linkTopPoints.back().x(), linkTopPoints.front().y(),
                     linkBottomPoints.front().y(), linkStr.front(), linkStr.back(), maxValueInVariables.front(),
                     minValueInVariables.front(), maxValueInVariables.back(), minValueInVariables.back(),
                     unChoosedColor);
        }
        for (auto& linkStrIndex: choosedLinkStringIndexs) {
            auto& linkStr = linkStrings[linkStrIndex];
            DrawLink(linkTopPoints.front().x(), linkTopPoints.back().x(), linkTopPoints.front().y(),
                     linkBottomPoints.front().y(), linkStr.front(), linkStr.back(), maxValueInVariables.front(),
                     minValueInVariables.front(), maxValueInVariables.back(), minValueInVariables.back(), choosedColor);
        }
        return;
    }
    for (int linkStrIndex = 0; linkStrIndex < linkStrings.size(); linkStrIndex++) {
        auto& linkStr = linkStrings[linkStrIndex];
        if (ShoultBeFilted(variableNum, linkStr)) continue;
        if (linkStringsChooseCondition[linkStrIndex]) {
            choosedLinkStringIndexs.push_back(linkStrIndex);
            continue;
        }
        for (int i = 0; i < variableNum - 1; i++) {
            DrawLink(linkTopPoints[i].x(), linkTopPoints[i + 1].x(), linkTopPoints[i].y(), linkBottomPoints[i].y(),
                     linkStr[i], linkStr[i + 1], maxValueInVariables[i], minValueInVariables[i],
                     maxValueInVariables[i + 1], minValueInVariables[i + 1], unChoosedColor);
        }
    }
    for (auto& linkStrIndex: choosedLinkStringIndexs) {
        auto& linkStr = linkStrings[linkStrIndex];
        for (int i = 0; i < variableNum - 1; i++) {
            DrawLink(linkTopPoints[i].x(), linkTopPoints[i + 1].x(), linkTopPoints[i].y(), linkBottomPoints[i].y(),
                     linkStr[i], linkStr[i + 1], maxValueInVariables[i], minValueInVariables[i],
                     maxValueInVariables[i + 1], minValueInVariables[i + 1], choosedColor);
        }
    }
}

void igQtParallelCoordinatesWidget::DrawStrs(int variableNum, const std::vector<std::string>& variableName,
                                             const std::vector<double>& maxValueInVariables,
                                             const std::vector<double>& minValueInVariables,
                                             std::vector<QRect>& variableMaxFontPoints,
                                             std::vector<QRect>& variableMinFontPoints,
                                             std::vector<QRect>& variableNameFontPoints) {
    QPainter painter(this);
    QPen pen;
    pen.setWidth(10);
    QFont font;
    font.setPointSize(8);
    painter.setPen(pen);
    painter.setFont(font);
    for (int i = 0; i < variableName.size(); i++) {
        painter.drawText(variableNameFontPoints[i], Qt::AlignCenter, QString(variableName[i].c_str()));
    }
    for (int i = 0; i < maxValueInVariables.size(); i++) {
        painter.drawText(variableMaxFontPoints[i], Qt::AlignCenter, QString::number(maxValueInVariables[i]));
    }
    for (int i = 0; i < minValueInVariables.size(); i++) {
        painter.drawText(variableMinFontPoints[i], Qt::AlignCenter, QString::number(minValueInVariables[i]));
    }
}

void igQtParallelCoordinatesWidget::ChoosedAlphaSliderChanged(int value) {
    if (m_ChoosedAlpha == value) return;
    m_ChoosedAlpha = value;
    this->update();
    if (ui->choosedAlphaSpinBox->value() != value) ui->choosedAlphaSpinBox->setValue(value);
}

void igQtParallelCoordinatesWidget::UnChoosedAlphaSliderChanged(int value) {
    if (m_UnChoosedAlpha == value) return;
    m_UnChoosedAlpha = value;
    this->update();
    if (ui->unChoosedAlphaSpinBox->value() != value) ui->unChoosedAlphaSpinBox->setValue(value);
}

void igQtParallelCoordinatesWidget::ChoosedAlphaSpinBoxChanged(int value) {
    if (m_ChoosedAlpha == value) return;
    m_ChoosedAlpha = value;
    this->update();
    if (ui->choosedAlphaSlider->value() != value) ui->choosedAlphaSlider->setValue(value);
}

void igQtParallelCoordinatesWidget::UnChoosedAlphaSpinBoxChanged(int value) {
    if (m_UnChoosedAlpha == value) return;
    m_UnChoosedAlpha = value;
    this->update();
    if (ui->unChoosedAlphaSlider->value() != value) ui->unChoosedAlphaSlider->setValue(value);
}

bool igQtParallelCoordinatesWidget::GetDrawFramePoints(int variableNum, std::vector<QRect>& variableMaxFontPoints,
                                                       std::vector<QRect>& variableMinFontPoints,
                                                       std::vector<QRect>& variableNameFontPoints,
                                                       std::vector<QPoint>& linkTopPoints,
                                                       std::vector<QPoint>& linkBottomPoints) {
    constexpr int leftSpace = 5, rightSpace = 5, topSpace = 5, bottomSpace = 5;
    constexpr int stringSize = 10;
    constexpr int eachInterval = 2;
    QPoint startPoint(leftSpace, topSpace), endPoint(ui->ParallelCoordinatesDrawView->size().width() - rightSpace,
                                                     ui->ParallelCoordinatesDrawView->size().height() - bottomSpace);
    if (endPoint.x() <= startPoint.x() || endPoint.y() - stringSize * 3 - eachInterval * 3 <= startPoint.y())
        return false;
    int useableWidth = endPoint.x() - startPoint.x();
    int useableHeight = endPoint.y() - startPoint.y();
    int maxFontStartTop = startPoint.y();
    int maxFontEndBottom = maxFontStartTop + stringSize;
    int variableStartTop = maxFontEndBottom + eachInterval;
    int nameFontEndBottom = endPoint.y();
    int nameFontStartTop = nameFontEndBottom - stringSize;
    int minFontEndBottom = nameFontStartTop - eachInterval;
    int minFontStartTop = minFontEndBottom - stringSize;
    int variableEndBottom = minFontStartTop - eachInterval;
    if (variableNum < 1) return false;
    if (variableNum == 1) {
        int variableInterval = useableWidth;
        linkTopPoints.push_back(QPoint(startPoint.x(), variableStartTop));
        linkTopPoints.push_back(QPoint(endPoint.x(), variableStartTop));
        linkBottomPoints.push_back(QPoint(startPoint.x(), variableEndBottom));
        linkBottomPoints.push_back(QPoint(endPoint.x(), variableEndBottom));
        variableMaxFontPoints.push_back(QRect(
                QPoint(0, maxFontStartTop), QPoint(ui->ParallelCoordinatesDrawView->size().width(), maxFontEndBottom)));
        variableMinFontPoints.push_back(QRect(
                QPoint(0, minFontStartTop), QPoint(ui->ParallelCoordinatesDrawView->size().width(), minFontEndBottom)));
        variableNameFontPoints.push_back(
                QRect(QPoint(0, nameFontStartTop),
                      QPoint(ui->ParallelCoordinatesDrawView->size().width(), nameFontEndBottom)));
        return true;
    }
    //Interval of midline in each variable
    int variableInterval = useableWidth / variableNum;
    for (int i = 0; i < variableNum; i++) {
        int halfInterval = variableInterval / 2;
        int nowMidLine = startPoint.x() + i * variableInterval + halfInterval;
        linkTopPoints.push_back(QPoint(nowMidLine, variableStartTop));
        linkBottomPoints.push_back(QPoint(nowMidLine, variableEndBottom));
        variableMaxFontPoints.push_back(QRect(QPoint(nowMidLine - halfInterval, maxFontStartTop),
                                              QPoint(nowMidLine + halfInterval, maxFontEndBottom)));
        variableMinFontPoints.push_back(QRect(QPoint(nowMidLine - halfInterval, minFontStartTop),
                                              QPoint(nowMidLine + halfInterval, minFontEndBottom)));
        variableNameFontPoints.push_back(QRect(QPoint(nowMidLine - halfInterval, nameFontStartTop),
                                               QPoint(nowMidLine + halfInterval, nameFontEndBottom)));
    }
    return true;
}
