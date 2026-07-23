#include "IQWidgets/ColorManager/igQtColorRangeSlider.h"
#include <QDebug>
#include "iGameScalarsToColors.h"
#include "iGameSceneManager.h"
//#include <iGameManager.h>
//#include <iGameColorMap.h>
//#include <iGameModelColorManager.h>
namespace
{

    const int SliderBarHeight = 15;
    const int HandleSideLength = SliderBarHeight+1;
    const int LeftRightMargin = 10;
    const int TopMargin = 10;
}

igQtColorRangeSlider::igQtColorRangeSlider(QWidget* aParent)
    : QWidget(aParent)
{
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    this->colorBarHeight = SliderBarHeight;
    setMouseTracking(true);
    InitColorRangeSlider();
}
bool igQtColorRangeSlider::InitColorRangeSlider() {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    m_ColorMapper = nullptr;
    iGame::ColorMap::Pointer src;
    if (scene) {
        auto model = scene->GetCurrentModel();
        if (model && model->GetDataObject()) {
            src = model->GetDataObject()->GetColorMapper();
            m_ColorMapper = src;
        }
    }
    if (src) {
        UpdateSliderWithColorBar(src);
        return true;
    }
    // 无模型时仍给可编辑默认色条，避免空指针绘制崩溃
    m_TmpColorMapper = iGame::ColorMap::New();
    m_TmpColorMapper->InitColorBarWithBlueWhiteRedType();
    PressedHandle = -1;
    update();
    return false;
}
void igQtColorRangeSlider::UpdateSliderWithColorBar(iGame::ColorMap::Pointer colorMap) {
    m_ColorMapper = colorMap;
    this->m_TmpColorMapper = iGame::ColorMap::New();
    if (!colorMap || !colorMap->GetColorBar() || !colorMap->GetColorRange()) {
        m_TmpColorMapper->InitColorBarWithBlueWhiteRedType();
        PressedHandle = -1;
        return;
    }
    auto tmpColorBar = iGame::FloatArray::New();
    tmpColorBar->SetDimension(3);
    auto tmpColorRange = iGame::FloatArray::New();
    int n = colorMap->GetColorBarSize() + 1;
    tmpColorBar->Reserve(n);
    tmpColorRange->Reserve(n);
    float rgb[16];
    for (int i = 0; i < n; i++) {
        colorMap->GetColorBar()->GetElement(i, rgb);
        tmpColorBar->AddElement(rgb);
    }
    for (int i = 0; i < n; i++) {
        tmpColorRange->AddValue(colorMap->GetColorRange()->GetValue(i));
    }
    m_TmpColorMapper->SetColorMap(tmpColorBar, tmpColorRange);
    PressedHandle = -1;
}
void igQtColorRangeSlider::updateSliderDrawInfo() {

    // 计算colors
    this->colorBarLength = this->width() - 2 * LeftRightMargin;
    if (colorBarLength <= 0) {
        drawColors.clear();
        return;
    }
    if (!m_TmpColorMapper) {
        m_TmpColorMapper = iGame::ColorMap::New();
        m_TmpColorMapper->InitColorBarWithBlueWhiteRedType();
    }
    drawColors.resize(colorBarLength);
    float scale = 1.0 / colorBarLength;
    QColor color;
    float rgb[16];
    for (int i = 0; i < colorBarLength; i++) {
        float val = i * scale;
        this->m_TmpColorMapper->MapColor(val, rgb);
        color.setRgbF(rgb[0], rgb[1], rgb[2]);
        drawColors[i] = color;
    }
}

void igQtColorRangeSlider::paintEvent(QPaintEvent* aEvent)
{
    Q_UNUSED(aEvent);
    updateSliderDrawInfo();
    if (!m_TmpColorMapper || drawColors.isEmpty()) return;
    QPainter painter(this);
    for (int i = 0; i < colorBarLength; i++) {
        QRect rect(LeftRightMargin + i, TopMargin, 1, colorBarHeight);
        painter.fillRect(rect, drawColors.at(i));
    }
    QPen pen;
    painter.setRenderHint(QPainter::Antialiasing);
    QBrush handleBrush(QColor(0xFA, 0xFA, 0xFA));
    painter.setBrush(handleBrush);
    auto colorRanges = this->m_TmpColorMapper->GetColorRange();
    if (!colorRanges) return;
    for (int i = 0; i < colorRanges->GetNumberOfElements(); i++) {
        if (i == PressedHandle) {
            pen.setColor(QColor(155, 0, 0));
            pen.setWidth(4);
            painter.setPen(pen);
        }
        else {
            pen.setColor(Qt::darkGray);
            pen.setWidth(static_cast<int>(0.5));
            painter.setPen(pen);
        }
        QRectF HandleRect = getHandleRectWithFloatValue(colorRanges->GetValue(i));
        painter.drawRoundedRect(HandleRect, 4, 4);
    }
}

QRectF igQtColorRangeSlider::getHandleRectWithFloatValue(float aValue) const
{
    return getHandleRect(aValue * colorBarLength - HandleSideLength / 2 + LeftRightMargin);
}

QRectF igQtColorRangeSlider::getHandleRect(int aValue) const
{
    return QRect(aValue, TopMargin, HandleSideLength, HandleSideLength);
}

void igQtColorRangeSlider::mousePressEvent(QMouseEvent* aEvent)
{
    if (!m_TmpColorMapper || !m_TmpColorMapper->GetColorRange() || !m_TmpColorMapper->GetColorBar()) return;
    this->isPressed = true;
    int tmp = this->PressedHandle;
    this->PressedHandle = -1;
    auto colorRanges = this->m_TmpColorMapper->GetColorRange();
    auto colorBars = this->m_TmpColorMapper->GetColorBar();
    if (aEvent->buttons() & Qt::LeftButton) {
        int  posValue;
        posValue = aEvent->pos().x();
        for (int i = 0; i < colorRanges->GetNumberOfElements(); i++) {
            if (getHandleRectWithFloatValue(colorRanges->GetValue(i)).contains(aEvent->pos())) {
                if (PressedHandle != -1) {
                    if (abs(getHandleRectWithFloatValue(colorRanges->GetValue(i)).center().x() - posValue)
                        < abs(getHandleRectWithFloatValue(colorRanges->GetValue(PressedHandle)).center().x() - posValue)) {
                        PressedHandle = i;
                    }
                }
                else {
                    PressedHandle = i;
                }
            }
        }
        //std::cout << PressedHandle << '\n';
        if (PressedHandle != -1) {
            mDelta = posValue - getHandleRectWithFloatValue(colorRanges->GetValue(PressedHandle)).center().x();
        }
        if (tmp != PressedHandle) {
            update();
        }

    }
    else if (aEvent->buttons() & Qt::RightButton) {
        int  posValue;
        posValue = aEvent->pos().x();
        float rangeValue = (posValue - LeftRightMargin) * 1.0 / colorBarLength;
        if (rangeValue <= 0.001 || rangeValue >= 0.999)return;
        int idx = 0;
        for (int i = 1; i < colorRanges->GetNumberOfElements(); i++) {
            if (colorRanges->GetValue(i - 1) < rangeValue && colorRanges->GetValue(i) > rangeValue) {
                idx = i; break;
            }
        }
        float rgb[3] = { 0 };
        m_TmpColorMapper->MapColor(rangeValue, rgb);
        m_TmpColorMapper->InsetIndexColorBar(idx, rangeValue,rgb);
        PressedHandle = idx;
        update();
    }
    else if (aEvent->buttons() & Qt::MiddleButton) {
        int  posValue;
        posValue = aEvent->pos().x();
        for (int i = 0; i < colorRanges->GetNumberOfElements(); i++) {
            if (getHandleRectWithFloatValue(colorRanges->GetValue(i)).contains(aEvent->pos())) {
                if (PressedHandle != -1) {
                    if (abs(getHandleRectWithFloatValue(colorRanges->GetValue(i)).center().x() - posValue)
                        < abs(getHandleRectWithFloatValue(colorRanges->GetValue(PressedHandle)).center().x() - posValue)) {
                        PressedHandle = i;
                    }
                }
                else {
                    PressedHandle = i;
                }
            }
        }
        if (PressedHandle != -1) {
            m_TmpColorMapper->DeleteIndexColor(PressedHandle);
            PressedHandle = -1;
            Q_EMIT rangeChanges();
            update();
        }
    }

}

void igQtColorRangeSlider::mouseMoveEvent(QMouseEvent* aEvent)
{
    if (!m_TmpColorMapper || !m_TmpColorMapper->GetColorRange()) return;
    auto colorRanges = this->m_TmpColorMapper->GetColorRange();
    if (aEvent->buttons() & Qt::LeftButton && isPressed)
    {
        if (this->isPressed == false || PressedHandle == -1 || PressedHandle == 0 || PressedHandle == m_TmpColorMapper->GetColorBarSize())return;
        int posValue = aEvent->pos().x();
        double minPos = PressedHandle == 0 ? 0.0 : colorRanges->GetValue(PressedHandle - 1);
        double maxPos = PressedHandle == colorRanges->GetNumberOfElements() - 1 ? 1.0 : colorRanges->GetValue(PressedHandle + 1);
        colorRanges->SetValue(PressedHandle, qBound(minPos, (posValue - mDelta - LeftRightMargin) * 1.0 / colorBarLength, maxPos));
        Q_EMIT rangeChanges();
        update();
    }
}

void igQtColorRangeSlider::mouseReleaseEvent(QMouseEvent* aEvent)
{
    Q_UNUSED(aEvent);
    this->isPressed = false;
    if (this->PressedHandle != -1 && m_TmpColorMapper && m_TmpColorMapper->GetColorBar()) {
        QColor c;
        float rgb[3];
        this->m_TmpColorMapper->GetColorBar()->GetElement(PressedHandle, rgb);
        c.setRgbF(rgb[0], rgb[1], rgb[2]);
        Q_EMIT MouseReleased(c);
    }

}
void igQtColorRangeSlider::updateColorInIndex(QColor c)
{
    if (!m_TmpColorMapper) return;
    if (PressedHandle != -1) {
        this->m_TmpColorMapper->SetIndexColor(PressedHandle, c.red() / 255.0, c.green() / 255.0, c.blue() / 255.0);
    }
    update();
    Q_EMIT rangeChanges();
}

bool igQtColorRangeSlider::updataManagerColorBarWithMyCorlorBar()
{
    if (!m_ColorMapper || !m_TmpColorMapper || !m_TmpColorMapper->GetColorBar() ||
        !m_TmpColorMapper->GetColorRange()) {
        return false;
    }
    // 重新绑定当前模型 mapper，避免场景切换后写到旧对象
    if (auto scene = iGame::SceneManager::Instance()->GetCurrentScene()) {
        if (auto model = scene->GetCurrentModel()) {
            if (auto obj = model->GetDataObject()) {
                if (auto mapper = obj->GetColorMapper()) { m_ColorMapper = mapper; }
            }
        }
    }
    if (!m_ColorMapper) return false;

    int num = this->m_TmpColorMapper->GetColorBar()->GetNumberOfElements();
    auto tmpColorBar = iGame::FloatArray::New();
    tmpColorBar->SetDimension(3);
    auto tmpColorRange = iGame::FloatArray::New();
    float rgb[3];
    for (int i = 0; i < num; i++) {
        m_TmpColorMapper->GetColorBar()->GetElement(i, rgb);
        tmpColorBar->AddElement(rgb);
        tmpColorRange->AddValue(m_TmpColorMapper->GetColorRange()->GetValue(i));
    }
    m_ColorMapper->SetColorMap(tmpColorBar, tmpColorRange);
    m_ColorMapper->Modified();
    return true;
}
void igQtColorRangeSlider::changeColorBarWithDefaultMode(int mode)
{
    if (!m_TmpColorMapper) { m_TmpColorMapper = iGame::ColorMap::New(); }
    switch (mode)
    {
    case 0:
        this->InitColorRangeSlider();
        break;
    case 1:
        this->m_TmpColorMapper->InitColorBarWithGrayScaleType();//灰度
        break;
    case 2:
        this->m_TmpColorMapper->InitColorBarWithBlueWhiteRedType();//蓝红
        break;
    case 3:
        this->m_TmpColorMapper->InitColorBarWithBlueCyanGreenYellowRedType();//蓝绿黄红
        break;
    case 4:
        this->m_TmpColorMapper->InitColorBarWithBlueCyanGreenYellowRedMagentaType();//蓝绿黄红紫
        break;
    default:
        break;
    }
    PressedHandle = -1;
    update();
    Q_EMIT rangeChanges();
}

QSize igQtColorRangeSlider::minimumSizeHint() const
{
    return QSize(HandleSideLength * 2 + LeftRightMargin * 2, HandleSideLength);
}


