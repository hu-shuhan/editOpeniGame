#include <IQWidgets/igQtVariableDensityWidget.h>
#include "ui_igQtVariableDensityWidget.h"
#include <QElapsedTimer>
#include <QEvent>
#include <QColor>
#include <QLinearGradient>
#include <QTransform>
#include <QList>
#include <QCheckBox>
#include <iGameThreadPool.h>
#include <iGameTimer.h>
#include <BuildAdjacencyRelation/iGameBuildAdjacencyRelationFilter.h>
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
    return 0.0f;
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
    _AT_;
    m_Model = model;
    m_Mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(m_Model->GetDataObject());
    if (m_Mesh == nullptr) {
        this->setEnabled(false);
        return;
    }
    this->setEnabled(true);
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

void igQtVariableDensityWidget::RangeChooseButtonClicked(bool checked) {
    static auto PreVisitFunc = [](iGame::Model::Pointer model) {
        if (model == nullptr) return;
        auto dataObj = model->GetDataObject();
        if (dataObj == nullptr) return;
        auto type = dataObj->GetDataObjectType();
        switch (type) {
            case IG_SURFACE_MESH:
            case IG_STRUCTURED_MESH:
            case IG_VOLUME_MESH: {
                auto buildAdjacencyRelationFilter = BuildAdjacencyRelationFilter::New();
                buildAdjacencyRelationFilter->SetInput(dataObj);
                buildAdjacencyRelationFilter->Execute();
            } break;
            case IG_UNSTRUCTURED_MESH: {
                auto mesh = DynamicCast<UnstructuredMesh>(dataObj);
                if (mesh == nullptr) return;
                auto selection = mesh->GetSelection();
                if (selection == nullptr) return;
                auto& cellFaceExtracter = selection->GetCellFaceExtracter();
                cellFaceExtracter.PreVisit(mesh);
            } break;
            default:
                return;
        }
    };
    PreVisitFunc(m_Model);
    m_RangeChooseOn = checked;
}

void igQtVariableDensityWidget::mousePressEvent(QMouseEvent* event) {
    QWidget::mousePressEvent(event);
    StartRangeChoose(event->pos());
}

void igQtVariableDensityWidget::mouseReleaseEvent(QMouseEvent* event) {
    QWidget::mouseReleaseEvent(event);
    EndRangeChoose();
}

void igQtVariableDensityWidget::paintEvent(QPaintEvent* QPE) {
    QWidget::paintEvent(QPE);
}

void igQtVariableDensityWidget::mouseMoveEvent(QMouseEvent* event) {
    handleMouseMove(event->pos());
    QWidget::mouseMoveEvent(event);
}

bool igQtVariableDensityWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched != ui->drawWidget) return QWidget::eventFilter(watched, event);

    if (event->type() == QEvent::Paint) {
        QPainter painter(ui->drawWidget);
        painter.setRenderHint(QPainter::Antialiasing, true);
        _PaintPlotOnDrawWidget(painter);
        return true;
    }
    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        StartRangeChoose(ui->drawWidget->mapTo(this, mouseEvent->pos()));
        return false;
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        EndRangeChoose();
        return false;
    }
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint parentPos = ui->drawWidget->mapTo(this, mouseEvent->pos());
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
    // 与路径图/变量相关性等深色面板一致；原 colorBar 自适应逻辑被早退屏蔽，此处统一深色底
    m_BackgroundColor = {0x2b, 0x2b, 0x2b};
}

void igQtVariableDensityWidget::_PaintPlotOnDrawWidget(QPainter& painter) {
    const QRect plotRect = ui->drawWidget->rect();
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) {
        painter.fillRect(plotRect, QColor(0x2b, 0x2b, 0x2b));
        return;
    }
    QRect smallDrawFrame = InsetRectByBoundaryRatio(plotRect, boundaryRatio);
    _DrawBackground(painter, plotRect);
    _DrawCoordinateRect(painter, smallDrawFrame);
    _DrawImages(painter, smallDrawFrame);
    _DrawCenterLine(painter, smallDrawFrame);
    if (m_RangeChooseOn && m_RangeChoosing) {
        QPoint a = ui->drawWidget->mapFrom(this, m_RangeChooseStartPoint);
        QPoint b = ui->drawWidget->mapFrom(this, m_RangeChooseEndPoint);
        painter.setPen(QPen(QColorConstants::DarkMagenta, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRect(a, b).normalized());
    }
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

void igQtVariableDensityWidget::_DrawCoordinateRect(QPainter& painter, const QRect& range) {
    painter.setPen(QPen(QColor(0xa8, 0xa8, 0xa8), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(range);
}

void igQtVariableDensityWidget::_DrawCenterLine(QPainter& painter, const QRect& range) {
    painter.setPen(QPen(QColor(0xa8, 0xa8, 0xa8), 1));
    painter.setBrush(Qt::NoBrush);
    if (m_ImageShowDirection == ImageShowDirection::Vertical)
        painter.drawLine(QPoint(range.center().x(), range.bottom()), QPoint(range.center().x(), range.top()));
    else if (m_ImageShowDirection == ImageShowDirection::Horizontal)
        painter.drawLine(QPoint(range.left(), range.center().y()), QPoint(range.right(), range.center().y()));
}

void igQtVariableDensityWidget::_DrawBackground(QPainter& painter, const QRect& range) {
    QBrush brush;
    brush.setColor(QColor(get<0>(m_BackgroundColor), get<1>(m_BackgroundColor), get<2>(m_BackgroundColor)));
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.setPen(Qt::NoPen);
    painter.drawRect(range);
}

void igQtVariableDensityWidget::_DrawImages(QPainter& painter, const QRect& range) {
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