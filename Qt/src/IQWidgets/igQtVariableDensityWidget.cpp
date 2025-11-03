#include <IQWidgets/igQtVariableDensityWidget.h>
#include "ui_igQtVariableDensityWidget.h"
#include <QElapsedTimer>
#include <QLinearGradient>
#include <QTransform>
#include <QList>
#include <QCheckBox>
#include <iGameThreadPool.h>

using namespace std;

static constexpr int defaultW = 2000, defaultH = 2000;
static constexpr double boundaryRatio = 0.05;
static constexpr double edBoundryRatio = 1.0 - boundaryRatio;
static constexpr int COPY_NUM{500};
static constexpr int HEIGHT_COPY_NUM_TIME{1};

static inline QColor GetQColorFromTuple(const tuple<int, int, int>& rgb, int alpha) {
    return QColor(get<0>(rgb), get<1>(rgb), get<2>(rgb), alpha);
}

static QImage flipAlongAntiDiagonal(const QImage& original) {
    int w = original.width();
    int h = original.height();

    QTransform transform(0, -1,       // m11, m12
                         -1, 0,       // m21, m22
                         h - 1, w - 1 // dx, dy
    );

    return original.transformed(transform);
}

static QRect InsetRectByBoundaryRatio(const QRect& rect, double boundaryRatio) {
    int width = rect.width();
    int height = rect.height();

    double insetX = static_cast<double>(width) * boundaryRatio;
    double insetY = static_cast<double>(height) * boundaryRatio;

    int insetXInt = static_cast<int>(std::round(insetX));
    int insetYInt = static_cast<int>(std::round(insetY));

    int newLeft = rect.left() + insetXInt;
    int newTop = rect.top() + insetYInt;

    int newWidth = std::max(0, width - 2 * insetXInt);
    int newHeight = std::max(0, height - 2 * insetYInt);

    return QRect(newLeft, newTop, newWidth, newHeight);
}

static int DistanceSquared(const std::tuple<int, int, int>& color1, const std::tuple<int, int, int>& color2) {
    int r1 = std::get<0>(color1);
    int g1 = std::get<1>(color1);
    int b1 = std::get<2>(color1);
    int r2 = std::get<0>(color2);
    int g2 = std::get<1>(color2);
    int b2 = std::get<2>(color2);
    int dr = r1 - r2;
    int dg = g1 - g2;
    int db = b1 - b2;
    return dr * dr + dg * dg + db * db;
}

static int MinDistanceSquared(const std::tuple<int, int, int>& bg,
                              const std::vector<std::tuple<int, int, int>>& colors) {
    int minDistSq = std::numeric_limits<int>::max();
    for (const auto& obj: colors) {
        int distSq = DistanceSquared(bg, obj);
        if (distSq < minDistSq) {
            minDistSq = distSq;
            if (minDistSq == 0) break;
        }
    }
    return minDistSq;
}

static std::tuple<int, int, int> CalculateBackgroundColor(const std::vector<std::tuple<int, int, int>>& colors) {
    if (colors.empty()) { return std::make_tuple(255, 255, 255); }

    const int step1 = 16;
    int bestMinDistSq = -1;
    std::tuple<int, int, int> bestBg;

    for (int r = 0; r <= 255; r += step1) {
        for (int g = 0; g <= 255; g += step1) {
            for (int b = 0; b <= 255; b += step1) {
                auto bg = std::make_tuple(r, g, b);
                int minDistSq = MinDistanceSquared(bg, colors);
                if (minDistSq > bestMinDistSq) {
                    bestMinDistSq = minDistSq;
                    bestBg = bg;
                }
            }
        }
    }

    int r0 = std::get<0>(bestBg);
    int g0 = std::get<1>(bestBg);
    int b0 = std::get<2>(bestBg);
    int r_start = std::max(0, r0 - step1);
    int r_end = std::min(255, r0 + step1);
    int g_start = std::max(0, g0 - step1);
    int g_end = std::min(255, g0 + step1);
    int b_start = std::max(0, b0 - step1);
    int b_end = std::min(255, b0 + step1);

    bestMinDistSq = -1;
    for (int r = r_start; r <= r_end; ++r) {
        for (int g = g_start; g <= g_end; ++g) {
            for (int b = b_start; b <= b_end; ++b) {
                auto bg = std::make_tuple(r, g, b);
                int minDistSq = MinDistanceSquared(bg, colors);
                if (minDistSq > bestMinDistSq) {
                    bestMinDistSq = minDistSq;
                    bestBg = bg;
                }
            }
        }
    }

    return bestBg;
}

static std::tuple<int, int, int> CalculateBackgroundColor(FloatArray::Pointer colorBar) {
    std::vector<std::tuple<int, int, int>> colors;
    for (int i = 0; i < colorBar->GetNumberOfElements(); i++) {
        colors.push_back({colorBar->GetElementValue(i, 0) * 255, colorBar->GetElementValue(i, 1) * 255,
                          colorBar->GetElementValue(i, 2) * 255});
    }
    return VariableDensityData::ChangeSaturation(CalculateBackgroundColor(colors), 85);
}

static double CalculateValueByPos(int pos, int minPos, int maxPos, double minValue, double maxValue) {
    if (minPos == maxPos) return (minValue + maxValue) / 2;
    if (minValue == maxValue) return minValue;
    return (pos - minPos) * (maxValue - minValue) / (maxPos - minPos) + minValue;
}

static double CalculateValueByPos(igQtVariableDensityWidget::ImageShowDirection showDirection, int x, int y,
                                  const QRect& frame, double minValue, double maxValue) {
    if (showDirection == igQtVariableDensityWidget::ImageShowDirection::Vertical) {
        return CalculateValueByPos(y, frame.bottom(), frame.top(), minValue, maxValue);
    } else if (showDirection == igQtVariableDensityWidget::ImageShowDirection::Horizontal) {
        return CalculateValueByPos(x, frame.left(), frame.right(), minValue, maxValue);
    }
}

igQtVariableDensityWidget_VariableChooseButton::igQtVariableDensityWidget_VariableChooseButton(QWidget* parent)
    : QRadioButton(parent) {}

igQtVariableDensityWidget::igQtVariableDensityWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::igQtVariableDensityWidget)
{
    ui->setupUi(this);
    m_BoxNum = COPY_NUM;
    ui->boxNumSlider->setValue(m_BoxNum);
    ui->boxNumSpinBox->setValue(m_BoxNum);
    connect(ui->choosedAlphaSlider, &QSlider::valueChanged, this,
            &igQtVariableDensityWidget::ChoosedAlphaSliderChanged);
    connect(ui->choosedAlphaSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtVariableDensityWidget::ChoosedAlphaSpinBoxChanged);
    connect(ui->unChoosedAlphaSlider, &QSlider::valueChanged, this,
            &igQtVariableDensityWidget::UnChoosedAlphaSliderChanged);
    connect(ui->unChoosedAlphaSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtVariableDensityWidget::UnChoosedAlphaSpinBoxChanged);
    connect(ui->choosedLightSlider, &QSlider::valueChanged, this,
            &igQtVariableDensityWidget::ChoosedLightSliderChanged);
    connect(ui->choosedLightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtVariableDensityWidget::ChoosedLightSpinBoxChanged);
    connect(ui->unChoosedLightSlider, &QSlider::valueChanged, this,
            &igQtVariableDensityWidget::UnChoosedLightSliderChanged);
    connect(ui->unChoosedLightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtVariableDensityWidget::UnChoosedLightSpinBoxChanged);
    connect(ui->boxNumSlider, &QSlider::valueChanged, this, &igQtVariableDensityWidget::BoxNumSliderChanged);
    connect(ui->boxNumSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtVariableDensityWidget::BoxNumSpinBoxChanged);
    connect(ui->dataTypeChoose, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtVariableDensityWidget::DataChooseChanged);
    connect(ui->flipDirection, &QPushButton::clicked, this, &igQtVariableDensityWidget::FlipDirectionClicked);
    connect(ui->refreshData, &QPushButton::clicked, this, &igQtVariableDensityWidget::RefreshData);
    connect(ui->rangeChoose, &QCheckBox::clicked, this, &igQtVariableDensityWidget::RangeChooseButtonClicked);
    setMouseTracking(true);
    ui->drawWidget->installEventFilter(this);
    ui->drawWidget->setMouseTracking(true);
}

igQtVariableDensityWidget::~igQtVariableDensityWidget()
{
    delete ui; }

void igQtVariableDensityWidget::SetModel(Model::Pointer model) {
    m_Model = model;
    m_Mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(m_Model->GetDataObject());
    SetSelectionCallback();
    SetClearSelectionCallback();
    GenerateVariableDensityDatas();
    SetDataTypeChoose();
    SetUiData();
    ClearVariableChoose();
    GenerateVariableChoose();
    GenerateBackgroundColor();
}

void igQtVariableDensityWidget::RangeChooseObj(const QRect& chooseRange, const QRect& frameRange,
                                               std::vector<igIndex>& ids, IGenum& type) {
    //init
    type = {};

    //set
    QRect overLapRange = frameRange.intersected(chooseRange);
    if (overLapRange.right() <= overLapRange.left() || overLapRange.bottom() <= overLapRange.top()) return;
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    type = Data->GetDataType();

    //range
    bool firstChoose{}, secondChoose{};
    double firstMinValue{}, firstMaxValue{}, secondMinValue{}, secondMaxValue{};
    switch (m_ImageShowDirection) {
        case igQtVariableDensityWidget::Vertical:
            firstChoose = (0 <= m_CurrentShowVariable.first) &&
                          (m_CurrentShowVariable.first < Data->GetVariableNum()) &&
                          (overLapRange.left() < frameRange.center().x());
            secondChoose = (0 <= m_CurrentShowVariable.second) &&
                           (m_CurrentShowVariable.second < Data->GetVariableNum()) &&
                           (frameRange.center().x() < overLapRange.right());
            if (firstChoose) {
                firstMinValue = CalculateValueByPos(overLapRange.bottom(), frameRange.bottom(), frameRange.top(),
                                                    Data->GetMinValueInVariables()[m_CurrentShowVariable.first],
                                                    Data->GetMaxValueInVariables()[m_CurrentShowVariable.first]);
                firstMaxValue = CalculateValueByPos(overLapRange.top(), frameRange.bottom(), frameRange.top(),
                                                    Data->GetMinValueInVariables()[m_CurrentShowVariable.first],
                                                    Data->GetMaxValueInVariables()[m_CurrentShowVariable.first]);
            }
            if (secondChoose) {
                secondMinValue = CalculateValueByPos(overLapRange.bottom(), frameRange.bottom(), frameRange.top(),
                                                     Data->GetMinValueInVariables()[m_CurrentShowVariable.second],
                                                     Data->GetMaxValueInVariables()[m_CurrentShowVariable.second]);
                secondMaxValue = CalculateValueByPos(overLapRange.top(), frameRange.bottom(), frameRange.top(),
                                                     Data->GetMinValueInVariables()[m_CurrentShowVariable.second],
                                                     Data->GetMaxValueInVariables()[m_CurrentShowVariable.second]);
            }
            break;
        case igQtVariableDensityWidget::Horizontal:
            firstChoose = (0 <= m_CurrentShowVariable.first) &&
                          (m_CurrentShowVariable.first < Data->GetVariableNum()) &&
                          (overLapRange.top() < frameRange.center().y());
            secondChoose = (0 <= m_CurrentShowVariable.second) &&
                           (m_CurrentShowVariable.second < Data->GetVariableNum()) &&
                           (frameRange.center().y() < overLapRange.bottom());
            if (firstChoose) {
                firstMinValue = CalculateValueByPos(overLapRange.left(), frameRange.left(), frameRange.right(),
                                                    Data->GetMinValueInVariables()[m_CurrentShowVariable.first],
                                                    Data->GetMaxValueInVariables()[m_CurrentShowVariable.first]);
                firstMaxValue = CalculateValueByPos(overLapRange.right(), frameRange.left(), frameRange.right(),
                                                    Data->GetMinValueInVariables()[m_CurrentShowVariable.first],
                                                    Data->GetMaxValueInVariables()[m_CurrentShowVariable.first]);
            }
            if (secondChoose) {
                secondMinValue = CalculateValueByPos(overLapRange.left(), frameRange.left(), frameRange.right(),
                                                     Data->GetMinValueInVariables()[m_CurrentShowVariable.second],
                                                     Data->GetMaxValueInVariables()[m_CurrentShowVariable.second]);
                secondMaxValue = CalculateValueByPos(overLapRange.right(), frameRange.left(), frameRange.right(),
                                                     Data->GetMinValueInVariables()[m_CurrentShowVariable.second],
                                                     Data->GetMaxValueInVariables()[m_CurrentShowVariable.second]);
            }
            break;
        default:
            break;
    }

    //choose
    if (!firstChoose && !secondChoose) return;
    std::vector<igIndex> firstIds, secondIds;
    if (firstChoose)
        firstIds = Data->FiltInRangeIds(m_CurrentShowVariable.first, firstMinValue, firstMaxValue);
    if (secondChoose)
        secondIds = Data->FiltInRangeIds(m_CurrentShowVariable.second, secondMinValue, secondMaxValue);
    std::sort(firstIds.begin(), firstIds.end());
    std::sort(secondIds.begin(), secondIds.end());
    std::merge(firstIds.begin(), firstIds.end(), secondIds.begin(), secondIds.end(), std::back_inserter(ids));
    auto last = std::unique(ids.begin(), ids.end());
    ids.erase(last, ids.end());
}

void igQtVariableDensityWidget::EndRangeChoose() {
    if (!m_RangeChoosing) return;
    m_RangeChoosing = false;
    if (!m_RangeChooseOn) return;
    QRect chooseRect(m_RangeChooseStartPoint, m_RangeChooseEndPoint);
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    std::vector<igIndex> ids;
    IGenum type{};
    RangeChooseObj(chooseRect, smallDrawFrame, ids, type);
    m_Model->GetSelection()->SelectionCallBackEvent(type, ids, Selection::Operate::Add);
    update();
}

void igQtVariableDensityWidget::StartRangeChoose(const QPoint& pos) {
    if (!m_RangeChooseOn) return;
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    if (!bigDrawFrame.contains(pos)) return;
    m_RangeChoosing = true;
    m_RangeChooseStartPoint = pos;
    m_RangeChooseEndPoint = pos;
}

void igQtVariableDensityWidget::MoveRangeChooseEndPoint(const QPoint& pos) {
    if (!m_RangeChooseOn) return;
    if (!m_RangeChoosing) return;
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    int x = max(bigDrawFrame.left(), min(pos.x(), bigDrawFrame.right()));
    int y = max(bigDrawFrame.top(), min(pos.y(), bigDrawFrame.bottom()));
    m_RangeChooseEndPoint = {x, y};
    update();
}

void igQtVariableDensityWidget::DrawRangeChooseRect() {
    if (!m_RangeChooseOn) return;
    if (!m_RangeChoosing) return;
    QPainter painter(this);
    painter.setPen(QPen(QColorConstants::DarkMagenta, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRect(m_RangeChooseStartPoint, m_RangeChooseEndPoint));
}

void igQtVariableDensityWidget::RangeChooseButtonClicked(bool checked) { m_RangeChooseOn = checked; }

void igQtVariableDensityWidget::mousePressEvent(QMouseEvent* event) {
    QWidget::mousePressEvent(event);
    StartRangeChoose(event->pos());
}

void igQtVariableDensityWidget::mouseReleaseEvent(QMouseEvent* event) {
    QWidget::mouseReleaseEvent(event);
    EndRangeChoose();
}

void igQtVariableDensityWidget::paintEvent(QPaintEvent* QPE) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    Draw();
    DrawRangeChooseRect();
}

void igQtVariableDensityWidget::mouseMoveEvent(QMouseEvent* event) {
    handleMouseMove(event->pos());
    QWidget::mouseMoveEvent(event);
}

bool igQtVariableDensityWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        QPoint parentPos = static_cast<QWidget*>(watched)->mapTo(this, mouseEvent->pos());
        handleMouseMove(parentPos);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void igQtVariableDensityWidget::handleMouseMove(const QPoint& pos) {
    static QElapsedTimer timer;
    if (!timer.isValid() || timer.elapsed() >= 50) {
        ClearVariablePosLabel();
        SetVariablePosLabel(pos.x(), pos.y());
        MoveRangeChooseEndPoint(pos);
        timer.start();
    }
}

void igQtVariableDensityWidget::GenerateVariableDensityDatas() {
    m_VariableDensityDatas.clear();
    m_CurrentModelDataIndex = -1;
    auto pointData = _GenerateVariableDensityDatas(IG_POINT);
    if (pointData.IsNotNull()) { m_VariableDensityDatas.push_back(pointData); }
    auto cellData = _GenerateVariableDensityDatas(IG_CELL);
    if (cellData.IsNotNull()) { m_VariableDensityDatas.push_back(cellData); }
    if (m_VariableDensityDatas.size() != 0) m_CurrentModelDataIndex = 0;
}

void igQtVariableDensityWidget::SetUiData() {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    ui->choosedAlphaSlider->setValue(Data->GetChoosedAlpha());
    ui->choosedAlphaSpinBox->setValue(Data->GetChoosedAlpha());
    ui->unChoosedAlphaSlider->setValue(Data->GetUnChoosedAlpha());
    ui->unChoosedAlphaSpinBox->setValue(Data->GetUnChoosedAlpha());
    ui->choosedLightSlider->setValue(Data->GetChoosedLight());
    ui->choosedLightSpinBox->setValue(Data->GetChoosedLight());
    ui->unChoosedLightSlider->setValue(Data->GetUnChoosedLight());
    ui->unChoosedLightSpinBox->setValue(Data->GetUnChoosedLight());
}

void igQtVariableDensityWidget::SetDataTypeChoose() {
    ui->dataTypeChoose->clear();
    if (m_VariableDensityDatas.size() == 0) {
        ui->dataTypeChoose->hide();
        return;
    }
    ui->dataTypeChoose->show();
    for (auto& Data: m_VariableDensityDatas) { ui->dataTypeChoose->addItem(Data->GetDataTypeName().c_str()); }
}

void igQtVariableDensityWidget::ClearVariableChoose() {
    auto firstButtonList = m_VariableFirstChooseButtons.buttons();
    auto secondButtonList = m_VariableSecondChooseButtons.buttons();
    for (auto& variableButton: firstButtonList) {
        m_VariableFirstChooseButtons.removeButton(variableButton);
        variableButton->deleteLater();
    }
    for (auto& variableButton: secondButtonList) {
        m_VariableSecondChooseButtons.removeButton(variableButton);
        variableButton->deleteLater();
    }
    m_CurrentShowVariable = {-1, -1};
    /*for (auto& variableChooseButton: m_VariableFirstChooseButtons) { variableChooseButton->deleteLater(); }
    for (auto& variableChooseButton: m_VariableSecondChooseButtons) { variableChooseButton->deleteLater(); }
    m_VariableFirstChooseButtons.clear();
    m_VariableSecondChooseButtons.clear();
    m_CurrentShowVariable = {-1, -1};*/
}

void igQtVariableDensityWidget::GenerateVariableChoose() {
    m_CurrentShowVariable = {-1, -1};
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    for (int variableIndex = 0; variableIndex < Data->GetVariableNum(); variableIndex++) {
        auto button_1 = new igQtVariableDensityWidget_VariableChooseButton(this);
        button_1->m_VariableIndex = variableIndex;
        button_1->setText("");
        button_1->setMinimumWidth(17);
        button_1->setMinimumHeight(17);
        connect(button_1, &igQtVariableDensityWidget_VariableChooseButton::clicked, this,
                &igQtVariableDensityWidget::VariableFirstChooseButtonClicked);
        m_VariableFirstChooseButtons.addButton(button_1);
        ui->variable->addWidget(button_1, variableIndex, 0);
        //ui->variable_1->addWidget(button_1);
        auto button_2 = new igQtVariableDensityWidget_VariableChooseButton(this);
        button_2->m_VariableIndex = variableIndex;
        button_2->setText(Data->GetVariableName()[variableIndex].c_str());
        button_2->setMinimumHeight(17);
        connect(button_2, &igQtVariableDensityWidget_VariableChooseButton::clicked, this,
                &igQtVariableDensityWidget::VariableSecondChooseButtonClicked);
        m_VariableSecondChooseButtons.addButton(button_2);
        ui->variable->addWidget(button_2, variableIndex, 1);
        //ui->variable_2->addWidget(button_2);
    }
}

void igQtVariableDensityWidget::ClearImage() {
    m_FirstDensityImage = QImage();
    m_FirstChoosedDensityImage = QImage();
    m_FirstDensityImage_T = QImage();
    m_FirstChoosedDensityImage_T = QImage();
    m_SecondDensityImage = QImage();
    m_SecondChoosedDensityImage = QImage();
    m_SecondDensityImage_T = QImage();
    m_SecondChoosedDensityImage_T = QImage();
}

void igQtVariableDensityWidget::GenerateFirstDensityImage() {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (m_CurrentShowVariable.first < 0 || Data->GetVariableNum() <= m_CurrentShowVariable.first) return;
    int maxDensity = *max_element(Data->GetDensity()[m_CurrentShowVariable.first].begin(),
                                  Data->GetDensity()[m_CurrentShowVariable.first].end());
    auto image = _DrawDensityImage(m_CurrentShowVariable.first, Data->GetDensity(), maxDensity, Data->GetDensityColor(),
                                   Data->GetUnChoosedAlpha());
    m_FirstDensityImage = image.mirrored(true, false);
    m_FirstDensityImage_T = m_FirstDensityImage.transformed(QTransform().rotate(90));
}

void igQtVariableDensityWidget::GenerateFirstChoosedDensityImage() {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (m_CurrentShowVariable.first < 0 || Data->GetVariableNum() <= m_CurrentShowVariable.first) return;
    int maxDensity = *max_element(Data->GetDensity()[m_CurrentShowVariable.first].begin(),
                                  Data->GetDensity()[m_CurrentShowVariable.first].end());
    auto image = _DrawDensityImage(m_CurrentShowVariable.first, Data->GetChoosedDensity(), maxDensity,
                                   Data->GetChoosedDensityColor(), Data->GetChoosedAlpha());
    m_FirstChoosedDensityImage = image.mirrored(true,false);
    m_FirstChoosedDensityImage_T = m_FirstChoosedDensityImage.transformed(QTransform().rotate(90));
}

void igQtVariableDensityWidget::GenerateSecondDensityImage() {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (m_CurrentShowVariable.second < 0 || Data->GetVariableNum() <= m_CurrentShowVariable.second) return;
    int maxDensity = *max_element(Data->GetDensity()[m_CurrentShowVariable.second].begin(),
                                  Data->GetDensity()[m_CurrentShowVariable.second].end());
    auto image = _DrawDensityImage(m_CurrentShowVariable.second, Data->GetDensity(), maxDensity,
                                   Data->GetDensityColor(), Data->GetUnChoosedAlpha());
    m_SecondDensityImage = image;
    m_SecondDensityImage_T = m_SecondDensityImage.transformed(QTransform().rotate(90));
}

void igQtVariableDensityWidget::GenerateSecondChoosedDensityImage() {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (m_CurrentShowVariable.second < 0 || Data->GetVariableNum() <= m_CurrentShowVariable.second) return;
    int maxDensity = *max_element(Data->GetDensity()[m_CurrentShowVariable.second].begin(),
                                  Data->GetDensity()[m_CurrentShowVariable.second].end());
    auto image = _DrawDensityImage(m_CurrentShowVariable.second, Data->GetChoosedDensity(), maxDensity,
                                   Data->GetChoosedDensityColor(), Data->GetChoosedAlpha());
    m_SecondChoosedDensityImage = image;
    m_SecondChoosedDensityImage_T = m_SecondChoosedDensityImage.transformed(QTransform().rotate(90));
}

void igQtVariableDensityWidget::GenerateBackgroundColor() {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) {
        m_BackgroundColor = {255, 255, 255};
        return;
    }
    auto colorMap = m_Mesh->GetColorMapper();
    auto colorBar = colorMap->GetColorBar();
    m_BackgroundColor = CalculateBackgroundColor(colorBar);
}

void igQtVariableDensityWidget::Draw() {
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    _DrawBackground(bigDrawFrame);
    _DrawCoordinateRect(smallDrawFrame);
    _DrawImages(smallDrawFrame);
    _DrawCenterLine(smallDrawFrame);
}

void igQtVariableDensityWidget::SetDensityColor() {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    auto colorMap = m_Mesh->GetColorMapper();
    Data->SetDensityColor(
            VariableDensityData::GenerateDensityColor(Data->GetCopyNum(), Data->GetUnChoosedLight(), colorMap));
}

void igQtVariableDensityWidget::SetChoosedDensityColor() {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    auto colorMap = m_Mesh->GetColorMapper();
    Data->SetChoosedDensityColor(
            VariableDensityData::GenerateDensityColor(Data->GetCopyNum(), Data->GetChoosedLight(), colorMap));
}

void igQtVariableDensityWidget::SetVariableNameLabel() {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (!(m_CurrentShowVariable.first < 0 || Data->GetVariableNum() <= m_CurrentShowVariable.first)) {
        ui->variableFirstName->setText(Data->GetVariableName()[m_CurrentShowVariable.first].c_str());
    }
    if (!(m_CurrentShowVariable.second < 0 || Data->GetVariableNum() <= m_CurrentShowVariable.second)) {
        ui->variableSecondName->setText(Data->GetVariableName()[m_CurrentShowVariable.second].c_str());
    }
}

void igQtVariableDensityWidget::ClearVariableNameLabel() {
    ui->variableFirstName->setText("");
    ui->variableSecondName->setText("");
}

void igQtVariableDensityWidget::SetVariablePosLabel(int x, int y) {
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    {
        double value{};
        int densityNum{};
        int choosedDensityNum{};
        bool result = _GetVariablePosMsg(m_CurrentShowVariable.first, x, y, smallDrawFrame, value, densityNum,
                                         choosedDensityNum);
        if (result) {
            ui->variableFirstPos->setNum(value);
            ui->variableFirstDensityNum->setNum(densityNum);
            ui->variableFirstChoosedDensityNum->setNum(choosedDensityNum);
        }
    }
    {
        double value{};
        int densityNum{};
        int choosedDensityNum{};
        bool result = _GetVariablePosMsg(m_CurrentShowVariable.second, x, y, smallDrawFrame, value, densityNum,
                                         choosedDensityNum);
        if (result) {
            ui->variableSecondPos->setNum(value);
            ui->variableSecondDensityNum->setNum(densityNum);
            ui->variableSecondChoosedDensityNum->setNum(choosedDensityNum);
        }
    }
}

void igQtVariableDensityWidget::ClearVariablePosLabel() {
    ui->variableFirstPos->setText("");
    ui->variableFirstDensityNum->setText("");
    ui->variableFirstChoosedDensityNum->setText("");
    ui->variableSecondPos->setText("");
    ui->variableSecondDensityNum->setText("");
    ui->variableSecondChoosedDensityNum->setText("");
}

void igQtVariableDensityWidget::UpdateChoosedData(IGenum itemType, const std::vector<igIndex>& ids,
                                                  Selection::Operate ope) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    for (auto& Data: m_VariableDensityDatas) {
        if (Data->GetDataType() != itemType) continue;
        switch (ope) {
            case Selection::Add:
                for (auto& id: ids) { Data->AddChoosedObjectId(id); }
                break;
            case Selection::Remove:
                for (auto& id: ids) { Data->RemoveChoosedObjectId(id); }
                break;
            default:
                break;
        }
        auto choosedDensity = VariableDensityData::GenerateDensity(
                Data->GetVariableNum(), Data->GetCopyNum(), Data->GetMaxValueInVariables(),
                Data->GetMinValueInVariables(), attrs, Data->GetDataType(), Data->GetChoosedObjectIds());
        Data->SetChoosedDensity(choosedDensity);
    }
}

void igQtVariableDensityWidget::ClearChoosedData() {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    for (auto& Data: m_VariableDensityDatas) {
        Data->ClearChoosedObjectIds();
        auto choosedDensity = VariableDensityData::GenerateDensity(
                Data->GetVariableNum(), Data->GetCopyNum(), Data->GetMaxValueInVariables(),
                Data->GetMinValueInVariables(), attrs, Data->GetDataType(), Data->GetChoosedObjectIds());
        Data->SetChoosedDensity(choosedDensity);
    }
}

VariableDensityData::Pointer igQtVariableDensityWidget::_GenerateVariableDensityDatas(IGenum dataType) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    auto colorMap = m_Mesh->GetColorMapper();
    auto& selectedItems = m_Model->GetSelection()->GetSelectedItems(dataType);
    int objNum{};
    if (dataType == IG_POINT) objNum = m_Mesh->GetNumberOfPoints();
    else
        objNum = m_Mesh->GetNumberOfCells();
    return VariableDensityData::New(attrs, dataType, selectedItems, objNum, m_BoxNum, colorMap);
}

QImage igQtVariableDensityWidget::_DrawDensityImage(
        int variableIndex, const std::vector<std::vector<int>>& density, int maxDensity,
        const std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>& densityColor, int alpha) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return {};
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (variableIndex < 0 || Data->GetVariableNum() <= variableIndex) return {};
    int copyNum = Data->GetCopyNum();
    int w = max(1000, defaultW / max(copyNum / 1000, 1));
    int h = copyNum * HEIGHT_COPY_NUM_TIME;
    QRect drawFrame;
    _CalculateDrawFrame(w, h, drawFrame);
    QImage re = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    re.fill(Qt::transparent);
    std::shared_ptr<QPainter> painter = make_shared<QPainter>(&re);
    _DrawDensityImage(variableIndex, density, maxDensity, densityColor, alpha, drawFrame, painter);
    return re;
}

void igQtVariableDensityWidget::_DrawDensityImage(
        int variableIndex, const std::vector<std::vector<int>>& density, int maxDensity,
        const std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>& densityColor, int alpha,
        const QRect& drawFrame, std::shared_ptr<QPainter> painter) {
    for (int copyIndex = 0; copyIndex < densityColor.size(); copyIndex++) {
        _DrawDensityRect(density[variableIndex][copyIndex], maxDensity, copyIndex, densityColor.size(),
                         densityColor[copyIndex], alpha, drawFrame, painter);
    }
}

void igQtVariableDensityWidget::_CalculateDrawFrame(int w, int h, QRect& drawFrame) {
    drawFrame = InsetRectByBoundaryRatio(QRect(0, 0, w, h), 0);
}

void igQtVariableDensityWidget::_CalculatePaintDrawFrame(QRect& bigDrawFrame, QRect& smallDrawFrame) {
    QRect drawWidgetRect = ui->drawWidget->rect();
    drawWidgetRect.moveTo(ui->drawWidget->mapTo(this, QPoint(0, 0)));
    bigDrawFrame = drawWidgetRect;
    smallDrawFrame = InsetRectByBoundaryRatio(drawWidgetRect, boundaryRatio);
}

void igQtVariableDensityWidget::_CalculateFrameCenterCut(const QRect& frame, QRect& leftFrame, QRect& rightFrame,
                                                         QRect& topFrame, QRect& bottomFrame) {
    int centerX = frame.center().x();
    int centerY = frame.center().y();
    leftFrame = QRect(QPoint(frame.left(), frame.top()), QPoint(centerX, frame.bottom()));
    rightFrame = QRect(QPoint(centerX, frame.top()), QPoint(frame.right(), frame.bottom()));
    topFrame = QRect(QPoint(frame.left(), frame.top()), QPoint(frame.right(), centerY));
    bottomFrame = QRect(QPoint(frame.left(), centerY), QPoint(frame.right(), frame.bottom()));
}

void igQtVariableDensityWidget::_DrawCoordinateRect(const QRect& range) {
    QPainter painter(this);
    painter.setPen(QPen(QColorConstants::White, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(range);
}

void igQtVariableDensityWidget::_DrawCenterLine(const QRect& range) {
    QPainter painter(this);
    painter.setPen(QPen(QColorConstants::White, 1));
    painter.setBrush(Qt::NoBrush);
    if (m_ImageShowDirection == ImageShowDirection::Vertical)
        painter.drawLine(QPoint(range.center().x(), range.bottom()), QPoint(range.center().x(), range.top()));
    else if (m_ImageShowDirection == ImageShowDirection::Horizontal)
        painter.drawLine(QPoint(range.left(), range.center().y()), QPoint(range.right(), range.center().y()));
}

void igQtVariableDensityWidget::_DrawBackground(const QRect& range) {
    QPainter painter(this);
    QBrush brush;
    brush.setColor(QColor(get<0>(m_BackgroundColor), get<1>(m_BackgroundColor), get<2>(m_BackgroundColor)));
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.drawRect(range);
}

void igQtVariableDensityWidget::_DrawImages(const QRect& range) {
    QPainter painter(this);
    QRect leftRange, rightRange, topRange, bottomRange;
    _CalculateFrameCenterCut(range, leftRange, rightRange, topRange, bottomRange);
    if (m_ImageShowDirection == ImageShowDirection::Vertical) {
        //painter.drawImage(range, m_FirstDensityImage);
        painter.drawImage(leftRange, m_FirstDensityImage);
        painter.drawImage(rightRange, m_SecondDensityImage);
        painter.drawImage(leftRange, m_FirstChoosedDensityImage);
        painter.drawImage(rightRange, m_SecondChoosedDensityImage);
    } else if (m_ImageShowDirection == ImageShowDirection::Horizontal) {
        painter.drawImage(topRange, m_FirstDensityImage_T);
        painter.drawImage(bottomRange, m_SecondDensityImage_T);
        painter.drawImage(topRange, m_FirstChoosedDensityImage_T);
        painter.drawImage(bottomRange, m_SecondChoosedDensityImage_T);
    }
}

void igQtVariableDensityWidget::_DrawDensityRect(
        int density, int maxDensity, int copyIndex, int copyNum,
        const std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>& color, int alpha, const QRect& drawFrame,
        std::shared_ptr<QPainter> painter) {
    int w = maxDensity == 0 ? 0 : ((double) density / (double) maxDensity) * (long long) drawFrame.width();
    int halfLeftSpace = (drawFrame.width() - w) / 2;
    int h = drawFrame.height() / copyNum;
    //int left = drawFrame.left() + halfLeftSpace;
    //int right = drawFrame.right() - halfLeftSpace;
    int left = drawFrame.left();
    int right = drawFrame.left() + w;
    int top = drawFrame.top() + (copyNum - copyIndex) * h - h;
    int bottom = drawFrame.top() + (copyNum - copyIndex) * h;
    int center = (drawFrame.left() + drawFrame.right()) / 2;
    QLinearGradient ling(QPoint(center, bottom), QPoint(center, top));
    ling.setColorAt(0, GetQColorFromTuple(color.first, alpha));
    ling.setColorAt(1, GetQColorFromTuple(color.second, alpha));
    ling.setSpread(QGradient::PadSpread);
    QBrush brush(ling);
    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(QRect(left, top, w, h));
}

bool igQtVariableDensityWidget::_GetVariablePosMsg(int variableIndex, int x, int y, QRect& frame, double& value,
                                                   int& densityNum, int& choosedDensityNum) {
    if (!frame.contains(x, y)) return false;
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return false;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (variableIndex < 0 || Data->GetVariableNum() <= variableIndex) return false;
    if (m_ImageShowDirection == ImageShowDirection::Vertical)
        value = CalculateValueByPos(y, frame.bottom(), frame.top(), Data->GetMinValueInVariables()[variableIndex],
                                    Data->GetMaxValueInVariables()[variableIndex]);
    else if (m_ImageShowDirection == ImageShowDirection::Horizontal)
        value = CalculateValueByPos(x, frame.left(), frame.right(), Data->GetMinValueInVariables()[variableIndex],
                                    Data->GetMaxValueInVariables()[variableIndex]);
    int copyIndex = VariableDensityData::CalculateCopyIndexByValue(Data->GetCopyNum(), value,
                                                                   Data->GetMaxValueInVariables()[variableIndex],
                                                                   Data->GetMinValueInVariables()[variableIndex]);
    densityNum = Data->GetDensity()[variableIndex][copyIndex];
    choosedDensityNum = Data->GetChoosedDensity()[variableIndex][copyIndex];
    return true;
}

void igQtVariableDensityWidget::_UpdateDataCopyNum() {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    auto colorMap = m_Mesh->GetColorMapper();
    for (auto& Data: m_VariableDensityDatas) {
        int objNum{};
        if (Data->GetDataType() == IG_POINT) objNum = m_Mesh->GetNumberOfPoints();
        else
            objNum = m_Mesh->GetNumberOfCells();
        Data->SetCopyNum(m_BoxNum);
        auto density = VariableDensityData::GenerateDensity(
                Data->GetVariableNum(), m_BoxNum, Data->GetMaxValueInVariables(), Data->GetMinValueInVariables(), attrs,
                Data->GetDataType(), objNum);
        Data->SetDensity(density);
        auto choosedDensity = VariableDensityData::GenerateDensity(
                Data->GetVariableNum(), m_BoxNum, Data->GetMaxValueInVariables(), Data->GetMinValueInVariables(), attrs,
                Data->GetDataType(), Data->GetChoosedObjectIds());
        Data->SetChoosedDensity(choosedDensity);
        Data->SetDensityColor(VariableDensityData::GenerateDensityColor(m_BoxNum, Data->GetUnChoosedLight(), colorMap));
        Data->SetChoosedDensityColor(
                VariableDensityData::GenerateDensityColor(m_BoxNum, Data->GetChoosedLight(), colorMap));
    }
}

void igQtVariableDensityWidget::SetSelectionCallback() {
    auto selection = m_Model->GetSelection();
    selection->SetSelectionCallBackEvent(&igQtVariableDensityWidget::SelectionCallbackEvent, this,
                                         std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void igQtVariableDensityWidget::SetClearSelectionCallback() {
    auto selection = m_Model->GetSelection();
    selection->SetClearSelectionCallBackEvent(&igQtVariableDensityWidget::ClearSelectionCallback, this);
}

void igQtVariableDensityWidget::SelectionCallbackEvent(IGenum itemType, const std::vector<igIndex>& ids,
                                                       Selection::Operate ope) {
    UpdateChoosedData(itemType, ids, ope);
    GenerateFirstChoosedDensityImage();
    GenerateSecondChoosedDensityImage();
    update();
}

void igQtVariableDensityWidget::ClearSelectionCallback() {
    ClearChoosedData();
    GenerateFirstChoosedDensityImage();
    GenerateSecondChoosedDensityImage();
    update();
}

void igQtVariableDensityWidget::ChoosedAlphaSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedAlpha() == value) return;
    Data->SetChoosedAlpha(value);
    GenerateFirstChoosedDensityImage();
    GenerateSecondChoosedDensityImage();
    update();
    if (ui->choosedAlphaSpinBox->value() != value) ui->choosedAlphaSpinBox->setValue(value);
}

void igQtVariableDensityWidget::UnChoosedAlphaSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedAlpha() == value) return;
    Data->SetUnChoosedAlpha(value);
    GenerateFirstDensityImage();
    GenerateSecondDensityImage();
    this->update();
    if (ui->unChoosedAlphaSpinBox->value() != value) ui->unChoosedAlphaSpinBox->setValue(value);
}

void igQtVariableDensityWidget::ChoosedAlphaSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedAlpha() == value) return;
    Data->SetChoosedAlpha(value);
    GenerateFirstChoosedDensityImage();
    GenerateSecondChoosedDensityImage();
    this->update();
    if (ui->choosedAlphaSlider->value() != value) ui->choosedAlphaSlider->setValue(value);
}

void igQtVariableDensityWidget::UnChoosedAlphaSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedAlpha() == value) return;
    Data->SetUnChoosedAlpha(value);
    GenerateFirstDensityImage();
    GenerateSecondDensityImage();
    this->update();
    if (ui->unChoosedAlphaSlider->value() != value) ui->unChoosedAlphaSlider->setValue(value);
}

void igQtVariableDensityWidget::ChoosedLightSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    SetChoosedDensityColor();
    GenerateFirstChoosedDensityImage();
    GenerateSecondChoosedDensityImage();
    this->update();
    if (ui->choosedLightSpinBox->value() != value) ui->choosedLightSpinBox->setValue(value);
}

void igQtVariableDensityWidget::UnChoosedLightSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    SetDensityColor();
    GenerateFirstDensityImage();
    GenerateSecondDensityImage();
    this->update();
    if (ui->unChoosedLightSpinBox->value() != value) ui->unChoosedLightSpinBox->setValue(value);
}

void igQtVariableDensityWidget::ChoosedLightSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    SetChoosedDensityColor();
    GenerateFirstChoosedDensityImage();
    GenerateSecondChoosedDensityImage();
    this->update();
    if (ui->choosedLightSlider->value() != value) ui->choosedLightSlider->setValue(value);
}

void igQtVariableDensityWidget::UnChoosedLightSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    SetDensityColor();
    GenerateFirstDensityImage();
    GenerateSecondDensityImage();
    this->update();
    if (ui->unChoosedLightSlider->value() != value) ui->unChoosedLightSlider->setValue(value);
}

void igQtVariableDensityWidget::BoxNumSliderChanged(int value) {
    if (m_BoxNum == value) return;
    m_BoxNum = value;
    _UpdateDataCopyNum();
    GenerateFirstDensityImage();
    GenerateFirstChoosedDensityImage();
    GenerateSecondDensityImage();
    GenerateSecondChoosedDensityImage();
    this->update();
    if (ui->boxNumSpinBox->value() != value) ui->boxNumSpinBox->setValue(value);
}

void igQtVariableDensityWidget::BoxNumSpinBoxChanged(int value) {
    if (m_BoxNum == value) return;
    m_BoxNum = value;
    _UpdateDataCopyNum();
    GenerateFirstDensityImage();
    GenerateFirstChoosedDensityImage();
    GenerateSecondDensityImage();
    GenerateSecondChoosedDensityImage();
    this->update();
    if (ui->boxNumSlider->value() != value) ui->boxNumSlider->setValue(value);
}

void igQtVariableDensityWidget::VariableFirstChooseButtonClicked(bool checked) {
    ClearVariableNameLabel();
    if (!checked) { return; }
    igQtVariableDensityWidget_VariableChooseButton* theSender =
            qobject_cast<igQtVariableDensityWidget_VariableChooseButton*>(sender());
    int& variableIndex = theSender->m_VariableIndex;
    m_CurrentShowVariable.first = variableIndex;
    GenerateFirstDensityImage();
    GenerateFirstChoosedDensityImage();
    SetVariableNameLabel();
    update();
}

void igQtVariableDensityWidget::VariableSecondChooseButtonClicked(bool checked) {
    ClearVariableNameLabel();
    if (!checked) { return; }
    igQtVariableDensityWidget_VariableChooseButton* theSender =
            qobject_cast<igQtVariableDensityWidget_VariableChooseButton*>(sender());
    int& variableIndex = theSender->m_VariableIndex;
    m_CurrentShowVariable.second = variableIndex;
    GenerateSecondDensityImage();
    GenerateSecondChoosedDensityImage();
    SetVariableNameLabel();
    update();
}

void igQtVariableDensityWidget::DataChooseChanged(int choosedIndex) {
    if (m_CurrentModelDataIndex == choosedIndex) return;
    m_CurrentModelDataIndex = choosedIndex;
    SetUiData();
    ClearVariableChoose();
    GenerateVariableChoose();
    ClearImage();
    update();
}

void igQtVariableDensityWidget::FlipDirectionClicked() {
    if (m_ImageShowDirection == ImageShowDirection::Vertical) m_ImageShowDirection = ImageShowDirection::Horizontal;
    else if (m_ImageShowDirection == ImageShowDirection::Horizontal)
        m_ImageShowDirection = ImageShowDirection::Vertical;
    update();
}

void igQtVariableDensityWidget::RefreshData() {
    emit SIGNAL_RefreshDataClicked();
    //GenerateVariableDensityDatas();
    //SetDataTypeChoose();
    //SetUiData();
    //ClearVariableChoose();
    //GenerateVariableChoose();
    //GenerateBackgroundColor();
}