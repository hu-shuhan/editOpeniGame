#include "ui_igQtVariableCorrelationWidget.h"
#include <IQWidgets/igQtVariableCorrelationWidget.h>
#include <QElapsedTimer>
#include <QPoint>
#include <algorithm>
#include <cmath>
#include <thread>
#include <QCheckBox>
#include <iGameThreadPool.h>
using namespace std;

static constexpr int defaultW = 2000, defaultH = 2000;
static constexpr double boundaryRatio = 0.05;
static constexpr double edBoundryRatio = 1.0 - boundaryRatio;

static inline QColor GetQColorFromTuple(const tuple<int, int, int>& rgb, int alpha) {
    return QColor(get<0>(rgb), get<1>(rgb), get<2>(rgb), alpha);
}

static QRect InsetRectByBoundaryRatio(const QRect& rect, double boundaryRatio) {
    //int width = rect.width();
    //int height = rect.height();

    //double insetX = static_cast<double>(width) * boundaryRatio;
    //double insetY = static_cast<double>(height) * boundaryRatio;

    //double inset = min(insetX, insetY);

    //int insetInt = static_cast<int>(std::round(inset));

    //int newLeft = rect.left() + insetInt;
    //int newTop = rect.top() + insetInt;

    //int newWidth = std::max(0, width - 2 * insetInt);
    //int newHeight = std::max(0, height - 2 * insetInt);

    //return QRect(newLeft, newTop, newWidth, newHeight);

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
    return VariableCorrelationData::ChangeSaturation(CalculateBackgroundColor(colors), 85);
}

static double CalculateValueByPos(int pos, int minPos, int maxPos, double minValue, double maxValue) {
    if (minPos == maxPos) return (minValue + maxValue) / 2;
    if (minValue == maxValue) return minValue;
    return (pos - minPos) * (maxValue - minValue) / (maxPos - minPos) + minValue;
}

igQtVariableCorrelationWidget_VariableChooseButton::igQtVariableCorrelationWidget_VariableChooseButton(QWidget* parent)
    : QRadioButton(parent) {}

igQtVariableCorrelationWidget_VariableCorrelationLabel::igQtVariableCorrelationWidget_VariableCorrelationLabel(
        QWidget* parent)
    : QLabel(parent) {}

igQtVariableCorrelationWidget::igQtVariableCorrelationWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::igQtVariableCorrelationWidget) {
    ui->setupUi(this);
    connect(ui->choosedAlphaSlider, &QSlider::valueChanged, this,
            &igQtVariableCorrelationWidget::ChoosedAlphaSliderChanged);
    connect(ui->choosedAlphaSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtVariableCorrelationWidget::ChoosedAlphaSpinBoxChanged);
    connect(ui->unChoosedAlphaSlider, &QSlider::valueChanged, this,
            &igQtVariableCorrelationWidget::UnChoosedAlphaSliderChanged);
    connect(ui->unChoosedAlphaSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtVariableCorrelationWidget::UnChoosedAlphaSpinBoxChanged);
    connect(ui->choosedLightSlider, &QSlider::valueChanged, this,
            &igQtVariableCorrelationWidget::ChoosedLightSliderChanged);
    connect(ui->choosedLightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtVariableCorrelationWidget::ChoosedLightSpinBoxChanged);
    connect(ui->unChoosedLightSlider, &QSlider::valueChanged, this,
            &igQtVariableCorrelationWidget::UnChoosedLightSliderChanged);
    connect(ui->unChoosedLightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtVariableCorrelationWidget::UnChoosedLightSpinBoxChanged);
    connect(ui->dataTypeChoose, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtVariableCorrelationWidget::DataChooseChanged);
    connect(ui->refreshData, &QPushButton::clicked, this, &igQtVariableCorrelationWidget::RefreshData);
    connect(this, &igQtVariableCorrelationWidget::SIGNAL_WaitImageLoading, this,
            &igQtVariableCorrelationWidget::WaitImageLoading);
    connect(this, &igQtVariableCorrelationWidget::SIGNAL_CompleteImageLoading, this,
            &igQtVariableCorrelationWidget::CompleteImageLoading);
    connect(ui->rangeChoose, &QCheckBox::clicked, this, &igQtVariableCorrelationWidget::RangeChooseButtonClicked);
    setMouseTracking(true);
    ui->drawWidget->installEventFilter(this);
    ui->drawWidget->setMouseTracking(true);
}

igQtVariableCorrelationWidget::~igQtVariableCorrelationWidget() { delete ui; }

void igQtVariableCorrelationWidget::SetModel(Model::Pointer model) {
    m_Model = model;
    m_Mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(m_Model->GetDataObject());
    SetSelectionCallback();
    SetClearSelectionCallback();
    GenerateVariableCorrelationDatas();
    SetDataTypeChoose();
    SetUiData();
    ClearMainVariableChoose();
    GenerateMainVariableChoose();
    ClearSubVariableChoose();
    GenerateBackgroundColor();
}

void igQtVariableCorrelationWidget::RangeChooseObj(const QRect& chooseRange, const QRect& frameRange,
                                                   std::vector<igIndex>& ids, IGenum& type) {
    //init
    type = {};

    //set
    QRect overLapRange = frameRange.intersected(chooseRange);
    if (overLapRange.right() <= overLapRange.left() || overLapRange.bottom() <= overLapRange.top()) return;
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    type = Data->GetDataType();
    if (m_CurrentShowVariable.first < 0 || Data->GetVariableNum() <= m_CurrentShowVariable.first ||
        m_CurrentShowVariable.second < 0 || Data->GetVariableNum() <= m_CurrentShowVariable.second)
        return;

    //range
    double mainVariableMinValue = CalculateValueByPos(overLapRange.bottom(), frameRange.bottom(), frameRange.top(),
                                                      Data->GetMinValueInVariables()[m_CurrentShowVariable.first],
                                                      Data->GetMaxValueInVariables()[m_CurrentShowVariable.first]);
    double mainVariableMaxValue = CalculateValueByPos(overLapRange.top(), frameRange.bottom(), frameRange.top(),
                                                      Data->GetMinValueInVariables()[m_CurrentShowVariable.first],
                                                      Data->GetMaxValueInVariables()[m_CurrentShowVariable.first]);
    double subVariableMinValue = CalculateValueByPos(overLapRange.left(), frameRange.left(), frameRange.right(),
                                                     Data->GetMinValueInVariables()[m_CurrentShowVariable.second],
                                                     Data->GetMaxValueInVariables()[m_CurrentShowVariable.second]);
    double subVariableMaxValue = CalculateValueByPos(overLapRange.right(), frameRange.left(), frameRange.right(),
                                                     Data->GetMinValueInVariables()[m_CurrentShowVariable.second],
                                                     Data->GetMaxValueInVariables()[m_CurrentShowVariable.second]);

    //choose
    ids = Data->FiltInRangeIds(m_CurrentShowVariable.first, m_CurrentShowVariable.second, mainVariableMinValue,
                               mainVariableMaxValue, subVariableMinValue, subVariableMaxValue);
}

void igQtVariableCorrelationWidget::EndRangeChoose() {
    if (!m_RangeChoosing) return;
    m_RangeChoosing = false;
    if (!m_RangeChooseOn) return;
    QRect chooseRect(m_RangeChooseStartPoint, m_RangeChooseEndPoint);
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    std::vector<igIndex> ids;
    IGenum type{};
    RangeChooseObj(chooseRect, smallDrawFrame, ids, type);
    auto events = Selection::GenerateEvents(ids, type, Selection::Event::Add, m_Mesh, m_Model->GetPainter3D().get());
    m_Model->GetSelection()->SelectionCallBackEvent(events);
    update();
}

void igQtVariableCorrelationWidget::StartRangeChoose(const QPoint& pos) {
    if (!m_RangeChooseOn) return;
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    if (!bigDrawFrame.contains(pos)) return;
    m_RangeChoosing = true;
    m_RangeChooseStartPoint = pos;
    m_RangeChooseEndPoint = pos;
}

void igQtVariableCorrelationWidget::MoveRangeChooseEndPoint(const QPoint& pos) {
    if (!m_RangeChooseOn) return;
    if (!m_RangeChoosing) return;
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    int x = max(bigDrawFrame.left(), min(pos.x(), bigDrawFrame.right()));
    int y = max(bigDrawFrame.top(), min(pos.y(), bigDrawFrame.bottom()));
    m_RangeChooseEndPoint = {x, y};
    update();
}

void igQtVariableCorrelationWidget::DrawRangeChooseRect() {
    if (!m_RangeChooseOn) return;
    if (!m_RangeChoosing) return;
    QPainter painter(this);
    painter.setPen(QPen(QColorConstants::DarkMagenta, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRect(m_RangeChooseStartPoint, m_RangeChooseEndPoint));
}

void igQtVariableCorrelationWidget::RangeChooseButtonClicked(bool checked) { m_RangeChooseOn = checked; }

void igQtVariableCorrelationWidget::mousePressEvent(QMouseEvent* event) {
    QWidget::mousePressEvent(event);
    StartRangeChoose(event->pos());
}

void igQtVariableCorrelationWidget::mouseReleaseEvent(QMouseEvent* event) {
    QWidget::mouseReleaseEvent(event);
    EndRangeChoose();
}

void igQtVariableCorrelationWidget::paintEvent(QPaintEvent* QPE) {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    Draw();
    DrawRangeChooseRect();
}

void igQtVariableCorrelationWidget::mouseMoveEvent(QMouseEvent* event) {
    handleMouseMove(event->pos());
    QWidget::mouseMoveEvent(event);
}

bool igQtVariableCorrelationWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        QPoint parentPos = static_cast<QWidget*>(watched)->mapTo(this, mouseEvent->pos());
        handleMouseMove(parentPos);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void igQtVariableCorrelationWidget::handleMouseMove(const QPoint& pos) {
    //ui->mainVariablePos->setNum(pos.x());
    //ui->subVariablePos->setNum(pos.y());
    //return;
    static QElapsedTimer timer;
    if (!timer.isValid() || timer.elapsed() >= 50) {
        ClearMainSubPosLabel();
        SetMainSubPosLabel(pos.x(), pos.y());
        MoveRangeChooseEndPoint(pos);
        timer.start();
    }
}

void igQtVariableCorrelationWidget::GenerateVariableCorrelationDatas() {
    m_VariableCorrelationDatas.clear();
    m_CurrentModelDataIndex = -1;
    auto pointData = _GenerateVariableCorrelationDatas(IG_POINT);
    if (pointData.IsNotNull()) { m_VariableCorrelationDatas.push_back(pointData); }
    auto cellData = _GenerateVariableCorrelationDatas(IG_CELL);
    if (cellData.IsNotNull()) { m_VariableCorrelationDatas.push_back(cellData); }
    if (m_VariableCorrelationDatas.size() != 0) m_CurrentModelDataIndex = 0;
}

void igQtVariableCorrelationWidget::SetUiData() {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    ui->choosedAlphaSlider->setValue(Data->GetChoosedAlpha());
    ui->choosedAlphaSpinBox->setValue(Data->GetChoosedAlpha());
    ui->unChoosedAlphaSlider->setValue(Data->GetUnChoosedAlpha());
    ui->unChoosedAlphaSpinBox->setValue(Data->GetUnChoosedAlpha());
    ui->choosedLightSlider->setValue(Data->GetChoosedLight());
    ui->choosedLightSpinBox->setValue(Data->GetChoosedLight());
    ui->unChoosedLightSlider->setValue(Data->GetUnChoosedLight());
    ui->unChoosedLightSpinBox->setValue(Data->GetUnChoosedLight());
}

void igQtVariableCorrelationWidget::SetDataTypeChoose() {
    ui->dataTypeChoose->clear();
    if (m_VariableCorrelationDatas.size() == 0) {
        ui->dataTypeChoose->hide();
        return;
    }
    ui->dataTypeChoose->show();
    for (auto& Data: m_VariableCorrelationDatas) { ui->dataTypeChoose->addItem(Data->GetDataTypeName().c_str()); }
}

void igQtVariableCorrelationWidget::ClearMainVariableChoose() {
    for (auto& mainVariableChooseButton: m_MainVariableChooseButtons) { mainVariableChooseButton->deleteLater(); }
    m_MainVariableChooseButtons.clear();
    {
        lock_guard lg(m_CurrentShowVariableMutex);
        m_CurrentShowVariable = {-1, -1};
    }
}

void igQtVariableCorrelationWidget::GenerateMainVariableChoose() {
    {
        lock_guard lg(m_CurrentShowVariableMutex);
        m_CurrentShowVariable = {-1, -1};
    }
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    for (int variableIndex = 0; variableIndex < Data->GetVariableNum(); variableIndex++) {
        auto button = new igQtVariableCorrelationWidget_VariableChooseButton(this);
        button->m_VariableIndex = variableIndex;
        button->setText(Data->GetVariableName()[variableIndex].c_str());
        connect(button, &igQtVariableCorrelationWidget_VariableChooseButton::clicked, this,
                &igQtVariableCorrelationWidget::MainVariableChooseButtonClicked);
        m_MainVariableChooseButtons.push_back(button);
        ui->mainVariable->addWidget(button);
    }
}

void igQtVariableCorrelationWidget::ClearSubVariableChoose() {
    for (auto& variableCorLable: m_VariableCorLabels) { variableCorLable->deleteLater(); }
    for (auto& choosedVariableCorLabel: m_ChoosedVariableCorLabels) { choosedVariableCorLabel->deleteLater(); }
    for (auto& subVariableChooseButton: m_SubVariableChooseButtons) { subVariableChooseButton->deleteLater(); }
    m_VariableCorLabels.clear();
    m_ChoosedVariableCorLabels.clear();
    m_SubVariableChooseButtons.clear();
    {
        lock_guard lg(m_CurrentShowVariableMutex);
        m_CurrentShowVariable.second = -1;
    }
}

void igQtVariableCorrelationWidget::GenerateSubVariableChoose(int mainVariableIndex) {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    {
        lock_guard lg(m_CurrentShowVariableMutex);
        if (m_CurrentShowVariable.first < 0 || Data->GetVariableNum() <= m_CurrentShowVariable.first) return;
    }
    vector<int> subVariableIndexSort;
    for (int variableIndex = 0; variableIndex < Data->GetVariableNum(); variableIndex++) {
        if (variableIndex == mainVariableIndex) continue;
        subVariableIndexSort.push_back(variableIndex);
    }
    auto& variableCor = Data->GetVariableCorrelation()[mainVariableIndex];
    auto& choosedVariableCor = Data->GetChoosedVariableCorrelation()[mainVariableIndex];
    std::sort(subVariableIndexSort.begin(), subVariableIndexSort.end(), [&](int variableIndexA, int variableIndexB) {
        return variableCor[variableIndexA] > variableCor[variableIndexB];
    });
    for (auto& variableIndex: subVariableIndexSort) {
        //cor
        auto corLabel = new igQtVariableCorrelationWidget_VariableCorrelationLabel(this);
        corLabel->m_VariableIndex = variableIndex;
        corLabel->m_OtherVariableIndex = mainVariableIndex;
        corLabel->m_LabelType = igQtVariableCorrelationWidget_VariableCorrelationLabel::LabelType::UnChoosedCor;
        corLabel->setText(QString::number(variableCor[variableIndex], 'f', 3));
        corLabel->setAlignment(Qt::AlignCenter);
        m_VariableCorLabels.push_back(corLabel);
        ui->unChoosedDataCor->addWidget(corLabel);
        //choosed cor
        auto choosedCorLabel = new igQtVariableCorrelationWidget_VariableCorrelationLabel(this);
        choosedCorLabel->m_VariableIndex = variableIndex;
        choosedCorLabel->m_OtherVariableIndex = mainVariableIndex;
        choosedCorLabel->m_LabelType = igQtVariableCorrelationWidget_VariableCorrelationLabel::LabelType::ChoosedCor;
        choosedCorLabel->setText(QString::number(choosedVariableCor[variableIndex], 'f', 3));
        choosedCorLabel->setAlignment(Qt::AlignCenter);
        m_ChoosedVariableCorLabels.push_back(choosedCorLabel);
        ui->choosedDataCor->addWidget(choosedCorLabel);
        //button
        auto button = new igQtVariableCorrelationWidget_VariableChooseButton(this);
        button->m_VariableIndex = variableIndex;
        button->setText(Data->GetVariableName()[variableIndex].c_str());
        connect(button, &igQtVariableCorrelationWidget_VariableChooseButton::clicked, this,
                &igQtVariableCorrelationWidget::SubVariableChooseButtonClicked);
        m_SubVariableChooseButtons.push_back(button);
        ui->subVariable->addWidget(button);
    }
}

void igQtVariableCorrelationWidget::ClearImage() {
    {
        std::lock_guard lg(m_CorImageMutex);
        m_CorImage = QImage();
    }
    m_ChoosedCorImage = QImage();
}

void igQtVariableCorrelationWidget::GenerateCorImage() {
    std::thread drawImageThread([&]() {
        emit SIGNAL_WaitImageLoading();
        auto image = _DrawCorImage();
        {
            std::lock_guard lg(m_CorImageMutex);
            m_CorImage = image;
        }
        emit SIGNAL_CompleteImageLoading();
    });
    drawImageThread.detach();
}

void igQtVariableCorrelationWidget::GenerateChoosedCorImage() { m_ChoosedCorImage = _DrawChoosedCorImage(); }

void igQtVariableCorrelationWidget::GenerateBackgroundColor() {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) {
        m_BackgroundColor = {255, 255, 255};
        return;
    }
    auto colorMap = m_Mesh->GetColorMapper();
    auto colorBar = colorMap->GetColorBar();
    m_BackgroundColor = CalculateBackgroundColor(colorBar);
}

void igQtVariableCorrelationWidget::Draw() {
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    _DrawBackground(bigDrawFrame);
    _DrawCoordinate(smallDrawFrame);
    _DrawImages(bigDrawFrame);
}

void igQtVariableCorrelationWidget::SetVariableCorrelationDataColor(int variableIndex) {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() < m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    if (variableIndex < 0 || Data->GetVariableNum() <= variableIndex) return;
    auto colorMap = m_Mesh->GetColorMapper();
    Data->SetObjectColor(VariableCorrelationData::GenerateObjectColors(
            variableIndex, Data->GetKeyObjectIds(), Data, Data->GetMaxValueInVariables(),
            Data->GetMinValueInVariables(), Data->GetUnChoosedLight(), colorMap));
}

void igQtVariableCorrelationWidget::SetChoosedVariableCorrelationDataColor(int variableIndex) {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() < m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    if (variableIndex < 0 || Data->GetVariableNum() <= variableIndex) return;
    auto colorMap = m_Mesh->GetColorMapper();
    Data->SetChoosedObjectColor(VariableCorrelationData::GenerateObjectColors(
            variableIndex, Data->GetChoosedObjectIds(), Data, Data->GetMaxValueInVariables(),
            Data->GetMinValueInVariables(), Data->GetChoosedLight(), colorMap));
}

void igQtVariableCorrelationWidget::SetMainSubNameLabel() {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() < m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    pair<int, int> showVariable{};
    {
        std::lock_guard lg(m_CurrentShowVariableMutex);
        showVariable = m_CurrentShowVariable;
    }
    if (showVariable.first < 0 || Data->GetVariableNum() <= showVariable.first || showVariable.second < 0 ||
        Data->GetVariableNum() <= showVariable.second)
        return;
    ui->mainVariableName->setText(Data->GetVariableName()[showVariable.first].c_str());
    ui->subVariableName->setText(Data->GetVariableName()[showVariable.second].c_str());
}

void igQtVariableCorrelationWidget::ClearMainSubNameLabel() {
    ui->mainVariableName->setText("");
    ui->subVariableName->setText("");
}

void igQtVariableCorrelationWidget::SetMainSubPosLabel(int x, int y) {
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    if (!smallDrawFrame.contains(x, y)) return;
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    pair<int, int> showVariable{};
    {
        std::lock_guard lg(m_CurrentShowVariableMutex);
        showVariable = m_CurrentShowVariable;
    }
    if (showVariable.first < 0 || Data->GetVariableNum() <= showVariable.first || showVariable.second < 0 ||
        Data->GetVariableNum() <= showVariable.second)
        return;
    auto& [mainVariableIndex, subVariableIndex] = showVariable;
    double mainValue = CalculateValueByPos(y, smallDrawFrame.bottom(), smallDrawFrame.top(),
                                           Data->GetMinValueInVariables()[mainVariableIndex],
                                           Data->GetMaxValueInVariables()[mainVariableIndex]);
    double subValue = CalculateValueByPos(x, smallDrawFrame.left(), smallDrawFrame.right(),
                                          Data->GetMinValueInVariables()[subVariableIndex],
                                          Data->GetMaxValueInVariables()[mainVariableIndex]);
    ui->mainVariablePos->setNum(mainValue);
    ui->subVariablePos->setNum(subValue);
}

void igQtVariableCorrelationWidget::ClearMainSubPosLabel() {
    ui->mainVariablePos->setText("");
    ui->subVariablePos->setText("");
}

void igQtVariableCorrelationWidget::UpdateChoosedData(const std::vector<Selection::Event>& _events) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    for (auto& Data: m_VariableCorrelationDatas) {
        for (auto& e: _events) {
            switch (e.type) {
                case Selection::Event::Type::PickPoint:
                    if (Data->GetDataType() != IG_POINT) break;
                    if (e.operate == Selection::Event::Operate::Add) Data->AddChoosedObjectId(e.pickId);
                    else if (e.operate == Selection::Event::Operate::Remove)
                        Data->RemoveChoosedObjectId(e.pickId);
                    break;
                case Selection::Event::Type::PickFace:
                    if (Data->GetDataType() != IG_CELL) break;
                    if (e.operate == Selection::Event::Operate::Add) Data->AddChoosedObjectId(e.pickId);
                    else if (e.operate == Selection::Event::Operate::Remove)
                        Data->RemoveChoosedObjectId(e.pickId);
                    break;
                default:
                    break;
            }
        }
        Data->SetChoosedObjectDrawSorts(VariableCorrelationData::GenerateObjectDrawSorts(
                Data->GetVariableNum(), Data->GetChoosedObjectIds(), Data));
    }
}

void igQtVariableCorrelationWidget::ClearChoosedData() {
    for (auto& Data: m_VariableCorrelationDatas) {
        Data->ClearChoosedObjectIds();
        Data->SetChoosedObjectDrawSorts(VariableCorrelationData::GenerateObjectDrawSorts(
                Data->GetVariableNum(), Data->GetChoosedObjectIds(), Data));
    }
}

void igQtVariableCorrelationWidget::UpdateChoosedCorrelation() {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    Data->SetChoosedVariableCorrelation(VariableCorrelationData::CalculateVariableCorrelation(
            Data->GetVariableNum(), Data->GetChoosedObjectIds(), Data));
    auto& choosedVariableCorrelation = Data->GetChoosedVariableCorrelation();
    for (auto& choosedVariableCorLabel: m_ChoosedVariableCorLabels) {
        auto correlation = choosedVariableCorrelation[choosedVariableCorLabel->m_OtherVariableIndex]
                                                     [choosedVariableCorLabel->m_VariableIndex];
        choosedVariableCorLabel->setText(QString::number(correlation, 'f', 3));
    }
}

VariableCorrelationData::Pointer igQtVariableCorrelationWidget::_GenerateVariableCorrelationDatas(IGenum dataType) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    auto& selectedItems = m_Model->GetSelection()->GetSelectedItems();
    int objNum{};
    if (dataType == IG_POINT) objNum = m_Mesh->GetNumberOfPoints();
    else
        objNum = m_Mesh->GetNumberOfCells();
    return VariableCorrelationData::New(attrs, dataType, selectedItems, objNum);
}

QImage igQtVariableCorrelationWidget::_DrawCorImage() {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return {};
    pair<int, int> drawVariableIndex{};
    {
        std::lock_guard lg(m_CurrentShowVariableMutex);
        if (m_CurrentShowVariable.first == -1 || m_CurrentShowVariable.second == -1) return {};
        drawVariableIndex = m_CurrentShowVariable;
    }
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    int objNum = Data->GetKeyObjectIds().size();
    int w = max(1000, defaultW / max(objNum / 1000, 1));
    int h = max(1000, defaultH / max(objNum / 1000, 1));
    QRect drawFrame;
    _CalculateDrawFrame(w, h, drawFrame);
    QImage re = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    re.fill(Qt::transparent);
    std::shared_ptr<QPainter> painter = make_shared<QPainter>(&re);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
    _DrawCorImage(drawVariableIndex.first, drawVariableIndex.second, drawFrame, painter);
    return re;
}

QImage igQtVariableCorrelationWidget::_DrawChoosedCorImage() {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return {};
    pair<int, int> drawVariableIndex{};
    {
        std::lock_guard lg(m_CurrentShowVariableMutex);
        if (m_CurrentShowVariable.first == -1 || m_CurrentShowVariable.second == -1) return {};
        drawVariableIndex = m_CurrentShowVariable;
    }
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    int objNum = Data->GetChoosedObjectIds().size();
    int w = max(1000, defaultW / max(objNum / 1000, 1));
    int h = max(1000, defaultH / max(objNum / 1000, 1));
    QRect drawFrame;
    _CalculateDrawFrame(w, h, drawFrame);
    QImage re = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    re.fill(Qt::transparent);
    std::shared_ptr<QPainter> painter = make_shared<QPainter>(&re);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
    _DrawChoosedCorImage(drawVariableIndex.first, drawVariableIndex.second, drawFrame, painter);
    return re;
}

void igQtVariableCorrelationWidget::_CalculateDrawFrame(int w, int h, QRect& drawFrame) {
    drawFrame = InsetRectByBoundaryRatio(QRect(0, 0, w, h), boundaryRatio);
}

void igQtVariableCorrelationWidget::_CalculatePaintDrawFrame(QRect& bigDrawFrame, QRect& smallDrawFrame) {
    QRect drawWidgetRect = ui->drawWidget->rect();
    drawWidgetRect.moveTo(ui->drawWidget->mapTo(this, QPoint(0, 0)));
    bigDrawFrame = drawWidgetRect;
    smallDrawFrame = InsetRectByBoundaryRatio(drawWidgetRect, boundaryRatio);
}

void igQtVariableCorrelationWidget::_DrawCorImage(int mainVariableIndex, int subVariableIndex, const QRect& drawFrame,
                                                  std::shared_ptr<QPainter> painter) {
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    auto& objDrawSort = Data->GetObjectDrawSorts()[mainVariableIndex];
    auto& variableMaxData = Data->GetMaxValueInVariables();
    auto& variableMinData = Data->GetMinValueInVariables();
    for (auto& objId: objDrawSort) {
        _DrawPoint(Data->GetObjectData(objId, mainVariableIndex), Data->GetObjectData(objId, subVariableIndex),
                   variableMaxData[mainVariableIndex], variableMinData[mainVariableIndex],
                   variableMaxData[subVariableIndex], variableMinData[subVariableIndex],
                   Data->GetObjectColor(false, objId), Data->GetUnChoosedAlpha(), drawFrame, painter, 5);
    }
}

void igQtVariableCorrelationWidget::_DrawChoosedCorImage(int mainVariableIndex, int subVariableIndex,
                                                         const QRect& drawFrame, std::shared_ptr<QPainter> painter) {
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    auto& objDrawSort = Data->GetChoosedObjDrawSorts()[mainVariableIndex];
    auto& variableMaxData = Data->GetMaxValueInVariables();
    auto& variableMinData = Data->GetMinValueInVariables();
    for (auto& objId: objDrawSort) {
        _DrawPoint(Data->GetObjectData(objId, mainVariableIndex), Data->GetObjectData(objId, subVariableIndex),
                   variableMaxData[mainVariableIndex], variableMinData[mainVariableIndex],
                   variableMaxData[subVariableIndex], variableMinData[subVariableIndex],
                   Data->GetObjectColor(true, objId), Data->GetChoosedAlpha(), drawFrame, painter, 10);
    }
}

void igQtVariableCorrelationWidget::_DrawCoordinate(const QRect& range) {
    QPainter painter(this);
    painter.setPen(QPen(QColorConstants::Black, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(range);
    painter.drawLine(range.bottomLeft(), range.topRight());
}

void igQtVariableCorrelationWidget::_DrawBackground(const QRect& range) {
    QPainter painter(this);
    QBrush brush;
    brush.setColor(QColor(get<0>(m_BackgroundColor), get<1>(m_BackgroundColor), get<2>(m_BackgroundColor)));
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.drawRect(range);
}

void igQtVariableCorrelationWidget::_DrawImages(const QRect& range) {
    QPainter painter(this);
    if (m_ImageLoading) {
        QPen pen;
        pen.setWidth(10);
        QFont font;
        font.setPointSize(8);
        painter.setPen(pen);
        painter.setFont(font);
        painter.drawText(range, Qt::AlignCenter, "Loading...");
    } else {
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true); // 设置平滑缩放
        {
            std::lock_guard lg(m_CorImageMutex);
            painter.drawImage(range, m_CorImage);
        }
        painter.drawImage(range, m_ChoosedCorImage);
    }
}

static inline int CalculateSite(double data, double maxData, double minData, int start, int end) {
    if (start == end) return start;
    if (minData == maxData) return (start + end) / 2;
    return ((double) start * (maxData - data) - (double) end * (minData - data)) / (maxData - minData);
}

static inline pair<int, int> CalculatePointSite(double mainVariableData, double subVariableData,
                                                double mainVariableMaxData, double mainVariableMinData,
                                                double subVariableMaxData, double subVariableMinData,
                                                const QRect& drawFrame) {
    int mainSite = CalculateSite(mainVariableData, mainVariableMaxData, mainVariableMinData, drawFrame.bottom(),
                                 drawFrame.top());
    int subSite =
            CalculateSite(subVariableData, subVariableMaxData, subVariableMinData, drawFrame.left(), drawFrame.right());
    return {subSite, mainSite};
}

void igQtVariableCorrelationWidget::_DrawPoint(double mainVariableData, double subVariableData,
                                               double mainVariableMaxData, double mainVariableMinData,
                                               double subVariableMaxData, double subVariableMinData,
                                               const std::tuple<int, int, int>& color, int alpha,
                                               const QRect& drawFrame, std::shared_ptr<QPainter> painter,
                                               int pointSize) {
    auto [x, y] = CalculatePointSite(mainVariableData, subVariableData, mainVariableMaxData, mainVariableMinData,
                                     subVariableMaxData, subVariableMinData, drawFrame);
    auto pen = QPen(QColor(GetQColorFromTuple(color, alpha)), pointSize);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    painter->drawPoint(x, y);
}

void igQtVariableCorrelationWidget::SetSelectionCallback() {
    auto selection = m_Model->GetSelection();
    selection->SetSelectionCallBackEvent(&igQtVariableCorrelationWidget::SelectionCallbackEvent, this,
                                         std::placeholders::_1);
}

void igQtVariableCorrelationWidget::SetClearSelectionCallback() {
    auto selection = m_Model->GetSelection();
    selection->SetClearSelectionCallBackEvent(&igQtVariableCorrelationWidget::ClearSelectionCallback, this);
}

void igQtVariableCorrelationWidget::SelectionCallbackEvent(const std::vector<Selection::Event>& _events) {
    UpdateChoosedData(_events);
    SetChoosedVariableCorrelationDataColor(m_CurrentShowVariable.first);
    UpdateChoosedCorrelation();
    GenerateChoosedCorImage();
    update();
}

void igQtVariableCorrelationWidget::ClearSelectionCallback() {
    ClearChoosedData();
    SetChoosedVariableCorrelationDataColor(m_CurrentShowVariable.first);
    UpdateChoosedCorrelation();
    GenerateChoosedCorImage();
    update();
}

void igQtVariableCorrelationWidget::ChoosedAlphaSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedAlpha() == value) return;
    Data->SetChoosedAlpha(value);
    GenerateChoosedCorImage();
    update();
    if (ui->choosedAlphaSpinBox->value() != value) ui->choosedAlphaSpinBox->setValue(value);
}

void igQtVariableCorrelationWidget::UnChoosedAlphaSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedAlpha() == value) return;
    Data->SetUnChoosedAlpha(value);
    GenerateCorImage();
    this->update();
    if (ui->unChoosedAlphaSpinBox->value() != value) ui->unChoosedAlphaSpinBox->setValue(value);
}

void igQtVariableCorrelationWidget::ChoosedAlphaSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedAlpha() == value) return;
    Data->SetChoosedAlpha(value);
    GenerateChoosedCorImage();
    this->update();
    if (ui->choosedAlphaSlider->value() != value) ui->choosedAlphaSlider->setValue(value);
}

void igQtVariableCorrelationWidget::UnChoosedAlphaSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedAlpha() == value) return;
    Data->SetUnChoosedAlpha(value);
    GenerateCorImage();
    this->update();
    if (ui->unChoosedAlphaSlider->value() != value) ui->unChoosedAlphaSlider->setValue(value);
}

void igQtVariableCorrelationWidget::ChoosedLightSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    Data->SetChoosedDefaultColor(VariableCorrelationData::GenerateDefaultColor(value));
    SetChoosedVariableCorrelationDataColor(m_CurrentShowVariable.first);
    GenerateChoosedCorImage();
    this->update();
    if (ui->choosedLightSpinBox->value() != value) ui->choosedLightSpinBox->setValue(value);
}

void igQtVariableCorrelationWidget::UnChoosedLightSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    Data->SetDefaultColor(VariableCorrelationData::GenerateDefaultColor(value));
    SetVariableCorrelationDataColor(m_CurrentShowVariable.first);
    GenerateCorImage();
    this->update();
    if (ui->unChoosedLightSpinBox->value() != value) ui->unChoosedLightSpinBox->setValue(value);
}

void igQtVariableCorrelationWidget::ChoosedLightSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    Data->SetChoosedDefaultColor(VariableCorrelationData::GenerateDefaultColor(value));
    SetChoosedVariableCorrelationDataColor(m_CurrentShowVariable.first);
    GenerateChoosedCorImage();
    this->update();
    if (ui->choosedLightSlider->value() != value) ui->choosedLightSlider->setValue(value);
}

void igQtVariableCorrelationWidget::UnChoosedLightSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableCorrelationDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableCorrelationDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    Data->SetDefaultColor(VariableCorrelationData::GenerateDefaultColor(value));
    SetVariableCorrelationDataColor(m_CurrentShowVariable.first);
    GenerateCorImage();
    this->update();
    if (ui->unChoosedLightSlider->value() != value) ui->unChoosedLightSlider->setValue(value);
}

void igQtVariableCorrelationWidget::MainVariableChooseButtonClicked(bool checked) {
    ClearSubVariableChoose();
    ClearMainSubNameLabel();
    if (!checked) { return; }
    igQtVariableCorrelationWidget_VariableChooseButton* theSender =
            qobject_cast<igQtVariableCorrelationWidget_VariableChooseButton*>(sender());
    int& mainVariableIndex = theSender->m_VariableIndex;
    SetVariableCorrelationDataColor(mainVariableIndex);
    SetChoosedVariableCorrelationDataColor(mainVariableIndex);
    {
        lock_guard lg(m_CurrentShowVariableMutex);
        m_CurrentShowVariable.first = mainVariableIndex;
    }
    GenerateSubVariableChoose(mainVariableIndex);
    ClearImage();
    update();
}

void igQtVariableCorrelationWidget::SubVariableChooseButtonClicked(bool checked) {
    if (!checked) {
        std::lock_guard lg(m_CurrentShowVariableMutex);
        m_CurrentShowVariable.second = -1;
    } else {
        igQtVariableCorrelationWidget_VariableChooseButton* theSender =
                qobject_cast<igQtVariableCorrelationWidget_VariableChooseButton*>(sender());
        lock_guard lg(m_CurrentShowVariableMutex);
        m_CurrentShowVariable.second = theSender->m_VariableIndex;
    }
    GenerateCorImage();
    GenerateChoosedCorImage();
    SetMainSubNameLabel();
    update();
}

void igQtVariableCorrelationWidget::DataChooseChanged(int choosedIndex) {
    if (m_CurrentModelDataIndex == choosedIndex) return;
    m_CurrentModelDataIndex = choosedIndex;
    SetUiData();
    ClearMainVariableChoose();
    GenerateMainVariableChoose();
    ClearSubVariableChoose();
    ClearImage();
    update();
}

void igQtVariableCorrelationWidget::RefreshData() {
    emit SIGNAL_RefreshDataClicked();
    //GenerateVariableCorrelationDatas();
    //SetDataTypeChoose();
    //SetUiData();
    //ClearMainVariableChoose();
    //GenerateMainVariableChoose();
    //ClearSubVariableChoose();
    //GenerateBackgroundColor();
}

void igQtVariableCorrelationWidget::WaitImageLoading() {
    setDisabled(true);
    m_ImageLoading = true;
    update();
}

void igQtVariableCorrelationWidget::CompleteImageLoading() {
    setDisabled(false);
    m_ImageLoading = false;
    update();
}
