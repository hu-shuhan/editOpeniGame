#include <IQWidgets/igQtVariableDensityWidget.h>
#include "ui_igQtVariableDensityWidget.h"
#include <QElapsedTimer>
#include <QLinearGradient>

using namespace std;

static constexpr int defaultW = 2000, defaultH = 2000;
static constexpr double boundaryRatio = 0.05;
static constexpr double edBoundryRatio = 1.0 - boundaryRatio;
static constexpr int COPY_NUM{200};
static constexpr int HEIGHT_COPY_NUM_TIME{20};

static inline QColor GetQColorFromTuple(const tuple<int, int, int>& rgb, int alpha) {
    return QColor(get<0>(rgb), get<1>(rgb), get<2>(rgb), alpha);
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
    return CalculateBackgroundColor(colors);
}

static double CalculateValueByPos(int pos, int minPos, int maxPos, double minValue, double maxValue) {
    if (minPos == maxPos) return (minValue + maxValue) / 2;
    if (minValue == maxValue) return minValue;
    return (pos - minPos) * (maxValue - minValue) / (maxPos - minPos) + minValue;
}

igQtVariableDensityWidget_VariableChooseButton::igQtVariableDensityWidget_VariableChooseButton(QWidget* parent) {}

igQtVariableDensityWidget::igQtVariableDensityWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::igQtVariableDensityWidget)
{
    ui->setupUi(this);
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
    connect(ui->dataTypeChoose, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtVariableDensityWidget::DataChooseChanged);
    connect(ui->refreshData, &QPushButton::clicked, this, &igQtVariableDensityWidget::RefreshData);
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

void igQtVariableDensityWidget::paintEvent(QPaintEvent* QPE) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    Draw();
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
    for (auto& variableChooseButton: m_VariableChooseButtons) { variableChooseButton->deleteLater(); }
    m_VariableChooseButtons.clear();
    m_CurrentShowVariable = -1;
}

void igQtVariableDensityWidget::GenerateVariableChoose() {
    m_CurrentShowVariable = -1;
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    for (int variableIndex = 0; variableIndex < Data->GetVariableNum(); variableIndex++) {
        auto button = new igQtVariableDensityWidget_VariableChooseButton(this);
        button->m_VariableIndex = variableIndex;
        button->setText(Data->GetVariableName()[variableIndex].c_str());
        connect(button, &igQtVariableDensityWidget_VariableChooseButton::clicked, this,
                &igQtVariableDensityWidget::VariableChooseButtonClicked);
        m_VariableChooseButtons.push_back(button);
        ui->variable->addWidget(button);
    }
}

void igQtVariableDensityWidget::ClearImage() {
    m_DensityImage = QImage();
    m_ChoosedDensityImage = QImage();
}

void igQtVariableDensityWidget::GenerateDensityImage() { m_DensityImage = _DrawDensityImage(); }

void igQtVariableDensityWidget::GenerateChoosedDensityImage() { m_ChoosedDensityImage = _DrawChoosedDensityImage(); }

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
    _DrawCoordinate(smallDrawFrame);
    _DrawImages(smallDrawFrame);
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
    if (m_CurrentShowVariable < 0 || Data->GetVariableNum() <= m_CurrentShowVariable) return;
    ui->variableName->setText(Data->GetVariableName()[m_CurrentShowVariable].c_str());
}

void igQtVariableDensityWidget::ClearVariableNameLabel() { ui->variableName->setText(""); }

void igQtVariableDensityWidget::SetVariablePosLabel(int x, int y) {
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    if (!smallDrawFrame.contains(x, y)) return;
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (m_CurrentShowVariable < 0 || Data->GetVariableNum() <= m_CurrentShowVariable) return;
    double value = CalculateValueByPos(y, smallDrawFrame.bottom(), smallDrawFrame.top(),
                                       Data->GetMinValueInVariables()[m_CurrentShowVariable],
                                       Data->GetMaxValueInVariables()[m_CurrentShowVariable]);
    ui->variablePos->setNum(value);
    int copyIndex = VariableDensityData::CalculateCopyIndexByValue(
            Data->GetCopyNum(), value, Data->GetMaxValueInVariables()[m_CurrentShowVariable],
            Data->GetMinValueInVariables()[m_CurrentShowVariable]);
    int densityNum = Data->GetDensity()[m_CurrentShowVariable][copyIndex];
    ui->variableDensityNum->setNum(densityNum);
    int choosedDensityNum = Data->GetChoosedDensity()[m_CurrentShowVariable][copyIndex];
    ui->variableChoosedDensityNum->setNum(choosedDensityNum);
}

void igQtVariableDensityWidget::ClearVariablePosLabel() {
    ui->variablePos->setText("");
    ui->variableDensityNum->setText("");
    ui->variableChoosedDensityNum->setText("");
}

void igQtVariableDensityWidget::UpdateChoosedData(const std::vector<Selection::Event>& _events) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    for (auto& Data: m_VariableDensityDatas) {
        for (auto& e: _events) {
            switch (e.type) {
                case Selection::Event::Type::PickPoint:
                    if (Data->GetDataType() != IG_POINT) break;
                    if (e.operate == Selection::Event::Operate::Add) Data->AddChoosedObjectIndex(e.pickId);
                    else if (e.operate == Selection::Event::Operate::Remove)
                        Data->RemoveChoosedObjectIndex(e.pickId);
                    break;
                case Selection::Event::Type::PickFace:
                    if (Data->GetDataType() != IG_CELL) break;
                    if (e.operate == Selection::Event::Operate::Add) Data->AddChoosedObjectIndex(e.pickId);
                    else if (e.operate == Selection::Event::Operate::Remove)
                        Data->RemoveChoosedObjectIndex(e.pickId);
                    break;
                default:
                    break;
            }
        }
        auto choosedDensity = VariableDensityData::GenerateDensity(
                Data->GetVariableNum(), Data->GetCopyNum(), Data->GetMaxValueInVariables(),
                Data->GetMinValueInVariables(), attrs, Data->GetDataType(), Data->GetChoosedObjectIndexs());
        Data->SetChoosedDensity(choosedDensity);
    }
}

void igQtVariableDensityWidget::ClearChoosedData() {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    for (auto& Data: m_VariableDensityDatas) {
        Data->ClearChoosedObjectIndex();
        auto choosedDensity = VariableDensityData::GenerateDensity(
                Data->GetVariableNum(), Data->GetCopyNum(), Data->GetMaxValueInVariables(),
                Data->GetMinValueInVariables(), attrs, Data->GetDataType(), Data->GetChoosedObjectIndexs());
        Data->SetChoosedDensity(choosedDensity);
    }
}

VariableDensityData::Pointer igQtVariableDensityWidget::_GenerateVariableDensityDatas(IGenum dataType) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    auto colorMap = m_Mesh->GetColorMapper();
    auto& selectedItems = m_Model->GetSelection()->GetSelectedItems();
    int objNum{};
    if (dataType == IG_POINT) objNum = m_Mesh->GetNumberOfPoints();
    else
        objNum = m_Mesh->GetNumberOfCells();
    auto variableNames = VariableDensityData::GenerateVariableNames(attrs, dataType);
    int variableNum = variableNames.size();
    if (variableNum == 0) return VariableDensityData::Pointer();
    auto Data = VariableDensityData::New();
    Data->SetCopyNum(COPY_NUM);
    Data->SetVariableNum(variableNum);
    Data->SetVariableName(variableNames);
    auto choosedObjIds = VariableDensityData::GenerateChoosedObjectIndexs(selectedItems, dataType);
    Data->SetChoosedObjectIndexs(choosedObjIds);
    auto [minValue, maxValue] = VariableDensityData::GenerateMinMaxData(attrs, dataType);
    Data->SetMinValueInVariables(minValue);
    Data->SetMaxValueInVariables(maxValue);
    auto density =
            VariableDensityData::GenerateDensity(variableNum, COPY_NUM, maxValue, minValue, attrs, dataType, objNum);
    Data->SetDensity(density);
    auto choosedDensity = VariableDensityData::GenerateDensity(variableNum, COPY_NUM, maxValue, minValue, attrs,
                                                               dataType, choosedObjIds);
    Data->SetChoosedDensity(choosedDensity);
    Data->SetDensityColor(VariableDensityData::GenerateDensityColor(COPY_NUM, Data->GetUnChoosedLight(), colorMap));
    Data->SetChoosedDensityColor(
            VariableDensityData::GenerateDensityColor(COPY_NUM, Data->GetChoosedLight(), colorMap));
    Data->SetDataType(dataType);
    Data->SetDataTypeName(VariableDensityData::GenerateDataTypeName(dataType));
    return Data;
}

QImage igQtVariableDensityWidget::_DrawDensityImage() {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return {};
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (m_CurrentShowVariable < 0 || Data->GetVariableNum() <= m_CurrentShowVariable) return {};
    int copyNum = Data->GetCopyNum();
    int w = max(1000, defaultW / max(copyNum / 1000, 1));
    int h = copyNum * HEIGHT_COPY_NUM_TIME;
    //int h = max(1000, defaultH / max(copyNum / 1000, 1));
    QRect drawFrame;
    _CalculateDrawFrame(w, h, drawFrame);
    QImage re = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    re.fill(Qt::transparent);
    std::shared_ptr<QPainter> painter = make_shared<QPainter>(&re);
    //painter->setRenderHint(QPainter::Antialiasing, true);
    //painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
    _DrawDensityImage(m_CurrentShowVariable, drawFrame, painter);
    return re;
}

QImage igQtVariableDensityWidget::_DrawChoosedDensityImage() {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return {};
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (m_CurrentShowVariable < 0 || Data->GetVariableNum() <= m_CurrentShowVariable) return {};
    int copyNum = Data->GetCopyNum();
    int w = max(1000, defaultW / max(copyNum / 1000, 1));
    int h = copyNum * HEIGHT_COPY_NUM_TIME;
    QRect drawFrame;
    _CalculateDrawFrame(w, h, drawFrame);
    QImage re = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    re.fill(Qt::transparent);
    std::shared_ptr<QPainter> painter = make_shared<QPainter>(&re);
    //painter->setRenderHint(QPainter::Antialiasing, true);
    //painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
    _DrawChoosedDensityImage(m_CurrentShowVariable, drawFrame, painter);
    return re;
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

void igQtVariableDensityWidget::_DrawDensityImage(int variableIndex, const QRect& drawFrame,
                                                  std::shared_ptr<QPainter> painter) {
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    auto& density = Data->GetDensity();
    auto& densityColor = Data->GetDensityColor();
    auto maxDensity = *std::max_element(density[variableIndex].begin(), density[variableIndex].end());
    for (int copyIndex = 0; copyIndex < Data->GetCopyNum(); copyIndex++) {
        _DrawDensityRect(density[variableIndex][copyIndex], maxDensity, copyIndex, Data->GetCopyNum(),
                         densityColor[copyIndex], Data->GetUnChoosedAlpha(), drawFrame, painter);
    }
}

void igQtVariableDensityWidget::_DrawChoosedDensityImage(int variableIndex, const QRect& drawFrame,
                                                         std::shared_ptr<QPainter> painter) {
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    auto& density = Data->GetChoosedDensity();
    auto& densityColor = Data->GetChoosedDensityColor();
    auto maxDensity =
            *std::max_element(Data->GetDensity()[variableIndex].begin(), Data->GetDensity()[variableIndex].end());
    for (int copyIndex = 0; copyIndex < Data->GetCopyNum(); copyIndex++) {
        _DrawDensityRect(density[variableIndex][copyIndex], maxDensity, copyIndex, Data->GetCopyNum(),
                         densityColor[copyIndex], Data->GetChoosedAlpha(), drawFrame, painter);
    }
}

void igQtVariableDensityWidget::_DrawCoordinate(const QRect& range) {
    QPainter painter(this);
    painter.setPen(QPen(QColorConstants::Black, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(range);
    painter.drawLine(QPoint(range.center().x(), range.bottom()), QPoint(range.center().x(), range.top()));
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
    painter.drawImage(range, m_DensityImage);
    painter.drawImage(range, m_ChoosedDensityImage);
}

void igQtVariableDensityWidget::_DrawDensityRect(
        int density, int maxDensity, int copyIndex, int copyNum,
        const std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>& color, int alpha, const QRect& drawFrame,
        std::shared_ptr<QPainter> painter) {
    int w = maxDensity == 0 ? 0 : ((double) density / (double) maxDensity) * (long long) drawFrame.width();
    int halfLeftSpace = (drawFrame.width() - w) / 2;
    int h = drawFrame.height() / copyNum;
    int left = drawFrame.left() + halfLeftSpace;
    int right = drawFrame.right() - halfLeftSpace;
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

void igQtVariableDensityWidget::SetSelectionCallback() {
    auto selection = m_Model->GetSelection();
    selection->SetSelectionCallBackEvent(&igQtVariableDensityWidget::SelectionCallbackEvent, this,
                                         std::placeholders::_1);
}

void igQtVariableDensityWidget::SetClearSelectionCallback() {
    auto selection = m_Model->GetSelection();
    selection->SetClearSelectionCallBackEvent(&igQtVariableDensityWidget::ClearSelectionCallback, this);
}

void igQtVariableDensityWidget::SelectionCallbackEvent(const std::vector<Selection::Event>& _events) {
    UpdateChoosedData(_events);
    GenerateChoosedDensityImage();
    update();
}

void igQtVariableDensityWidget::ClearSelectionCallback() {
    ClearChoosedData();
    GenerateChoosedDensityImage();
    update();
}

void igQtVariableDensityWidget::ChoosedAlphaSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedAlpha() == value) return;
    Data->SetChoosedAlpha(value);
    GenerateChoosedDensityImage();
    update();
    if (ui->choosedAlphaSpinBox->value() != value) ui->choosedAlphaSpinBox->setValue(value);
}

void igQtVariableDensityWidget::UnChoosedAlphaSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedAlpha() == value) return;
    Data->SetUnChoosedAlpha(value);
    GenerateDensityImage();
    this->update();
    if (ui->unChoosedAlphaSpinBox->value() != value) ui->unChoosedAlphaSpinBox->setValue(value);
}

void igQtVariableDensityWidget::ChoosedAlphaSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedAlpha() == value) return;
    Data->SetChoosedAlpha(value);
    GenerateChoosedDensityImage();
    this->update();
    if (ui->choosedAlphaSlider->value() != value) ui->choosedAlphaSlider->setValue(value);
}

void igQtVariableDensityWidget::UnChoosedAlphaSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedAlpha() == value) return;
    Data->SetUnChoosedAlpha(value);
    GenerateDensityImage();
    this->update();
    if (ui->unChoosedAlphaSlider->value() != value) ui->unChoosedAlphaSlider->setValue(value);
}

void igQtVariableDensityWidget::ChoosedLightSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    SetChoosedDensityColor();
    GenerateChoosedDensityImage();
    this->update();
    if (ui->choosedLightSpinBox->value() != value) ui->choosedLightSpinBox->setValue(value);
}

void igQtVariableDensityWidget::UnChoosedLightSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    SetDensityColor();
    GenerateDensityImage();
    this->update();
    if (ui->unChoosedLightSpinBox->value() != value) ui->unChoosedLightSpinBox->setValue(value);
}

void igQtVariableDensityWidget::ChoosedLightSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    SetChoosedDensityColor();
    GenerateChoosedDensityImage();
    this->update();
    if (ui->choosedLightSlider->value() != value) ui->choosedLightSlider->setValue(value);
}

void igQtVariableDensityWidget::UnChoosedLightSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_VariableDensityDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_VariableDensityDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    SetDensityColor();
    GenerateDensityImage();
    this->update();
    if (ui->unChoosedLightSlider->value() != value) ui->unChoosedLightSlider->setValue(value);
}

void igQtVariableDensityWidget::VariableChooseButtonClicked(bool checked) {
    ClearVariableNameLabel();
    if (!checked) { return; }
    igQtVariableDensityWidget_VariableChooseButton* theSender =
            qobject_cast<igQtVariableDensityWidget_VariableChooseButton*>(sender());
    int& variableIndex = theSender->m_VariableIndex;
    m_CurrentShowVariable = variableIndex;
    GenerateDensityImage();
    GenerateChoosedDensityImage();
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

void igQtVariableDensityWidget::RefreshData() {
    GenerateVariableDensityDatas();
    SetDataTypeChoose();
    SetUiData();
    ClearVariableChoose();
    GenerateVariableChoose();
    GenerateBackgroundColor();
}