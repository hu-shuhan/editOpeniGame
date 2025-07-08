#include <IQWidgets/igQtContextPreservingShowWidget.h>
#include "ui_ContextPreservingShowView.h"
#include <sstream>
#include <cmath>
#include <algorithm>
#include <climits>


const float EPSILON = 1e-6f;

static void rgbToHsb(float r, float g, float b, float& h, float& s, float& bVal) {
    float max_c = std::max({r, g, b});
    float min_c = std::min({r, g, b});
    float delta = max_c - min_c;

    bVal = max_c;

    if (max_c != 0.0f) {
        s = delta / max_c;
    } else {
        s = 0.0f;
        h = 0.0f;
        return;
    }

    if (delta == 0.0f) {
        h = 0.0f;
        return;
    }

    if (max_c == r) {
        h = 60.0f * std::fmod((g - b) / delta, 6.0f);
    } else if (max_c == g) {
        h = 60.0f * ((b - r) / delta + 2.0f);
    } else { // max_c == b
        h = 60.0f * ((r - g) / delta + 4.0f);
    }
}

static void hsbToRgb(float h, float s, float bVal, float& r, float& g, float& b) {
    float c = bVal * s;
    float x = c * (1.0f - std::fabs(std::fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = bVal - c;

    float r1, g1, b1;

    if (h >= 0.0f - EPSILON && h < 60.0f + EPSILON) {
        r1 = c + m;
        g1 = x + m;
        b1 = 0.0f + m;
    } else if (h >= 60.0f - EPSILON && h < 120.0f + EPSILON) {
        r1 = x + m;
        g1 = c + m;
        b1 = 0.0f + m;
    } else if (h >= 120.0f - EPSILON && h < 180.0f + EPSILON) {
        r1 = 0.0f + m;
        g1 = c + m;
        b1 = x + m;
    } else if (h >= 180.0f - EPSILON && h < 240.0f + EPSILON) {
        r1 = 0.0f + m;
        g1 = x + m;
        b1 = c + m;
    } else if (h >= 240.0f - EPSILON && h < 300.0f + EPSILON) {
        r1 = x + m;
        g1 = 0.0f + m;
        b1 = c + m;
    } else {
        r1 = c + m;
        g1 = 0.0f + m;
        b1 = x + m;
    }

    r = r1;
    g = g1;
    b = b1;
}

static void ChangeRgbBrightness(float& r, float& g, float& b, float targetBrightness) {
    float h, s, currentB;

    rgbToHsb(r, g, b, h, s, currentB);

    if (std::fabs(currentB - targetBrightness) < EPSILON) { return; }

    float newB = std::max(0.0f, std::min(1.0f, targetBrightness));

    hsbToRgb(h, s, newB, r, g, b);
}

igQtContextPreservingShowWidget::igQtContextPreservingShowWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContextPreservingShowView)
{
    ui->setupUi(this);
    connect(ui->chooesdData, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtContextPreservingShowWidget::ChoosedDataChanged);
    connect(ui->refreshModelData, &QPushButton::clicked, this, &igQtContextPreservingShowWidget::Slot_Refresh);
    ui->choosedLightSlider->setValue(m_MaxBrightness);
    ui->choosedLightSpinBox->setValue(m_MaxBrightness);
    ui->unChoosedLightSlider->setValue(m_MinBrightness);
    ui->unChoosedLightSpinBox->setValue(m_MinBrightness);
    connect(ui->choosedLightSlider, &QSlider::valueChanged, this,
            &igQtContextPreservingShowWidget::Slot_ChooesdLightSliderChanged);
    connect(ui->choosedLightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtContextPreservingShowWidget::Slot_ChoosedLightSpinBoxChanged);
    connect(ui->unChoosedLightSlider, &QSlider::valueChanged, this,
            &igQtContextPreservingShowWidget::Slot_UnChoosedLightSliderChanged);
    connect(ui->unChoosedLightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtContextPreservingShowWidget::Slot_UnChoosedLightSpinBoxChanged);
}

igQtContextPreservingShowWidget::~igQtContextPreservingShowWidget() { delete ui; }

void igQtContextPreservingShowWidget::SetContextPreserving(Model::Pointer model) {
    ClearOldDraws();
    m_Model = model;
    m_Mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(m_Model->GetDataObject());
    SetAttrs();
    SetSelectionCallBack();
    SetChoosedDataComboBox();
    UpdateDraw();
}

void igQtContextPreservingShowWidget::ClearOldDraws() {
    if (m_ObjDrawHandles.empty() || m_Model.IsNull()) return;
    auto painter = m_Model->GetPainter3D();
    for (auto& objHandles: m_ObjDrawHandles) {
        for (auto& handle: objHandles) { painter->Delete(handle); }
    }
    m_ObjDrawHandles.clear();
}

void igQtContextPreservingShowWidget::UpdateDraw() {
    ClearOldDraws();
    if (m_Model.IsNull() || m_Mesh.IsNull()) return;
    if (m_ShowingAttrIndex < 0 || m_Attrs.size() <= m_ShowingAttrIndex) m_ShowingAttrIndex = 0;
    if (m_Attrs.size() == 0) return;
    auto& attrIndex = m_Attrs[m_ShowingAttrIndex];
    if (attrIndex.first != -1) {
        auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
        if (attrs->Size() <= attrIndex.first) return;
        auto& attr = attrs->GetElement(attrIndex.first);
        DrawAttr(attr, attrIndex.second);
    }
    emit DrawUpdated();
}

void igQtContextPreservingShowWidget::DrawAttr(AttributeSet::Attribute& attr, int variableIndex) {
    switch (attr.attachmentType) {
        case IG_POINT:
            DrawPointAttr(attr, variableIndex);
            break;
        case IG_CELL:
            DrawCellAttr(attr, variableIndex);
            break;
        default:
            break;
    }
}

static bool IsSelectedItem(Model::Pointer model, IGenum itemType, igIndex itemIndex) {
    auto& selectedItems = model->GetSelection()->GetSelectedItems();
    switch (itemType) {
        case IG_POINT:
            if (selectedItems.count(Selection::Event::Type::PickPoint) == 0) return false;
            if (selectedItems.at(Selection::Event::Type::PickPoint).count(itemIndex) == 0) return false;
            return true;
            break;
        case IG_CELL:
            if (selectedItems.count(Selection::Event::Type::PickFace) == 0) return false;
            if (selectedItems.at(Selection::Event::Type::PickFace).count(itemIndex) == 0) return false;
            return true;
            break;
        default:
            return false;
            break;
    }
}

static double SelectedPointPerOfFace(Model::Pointer model, Cell* face) {
    int pointSize = face->GetNumberOfPoints();
    int choosedNum{};
    for (int pointIndex = 0; pointIndex < pointSize; pointIndex++) {
        auto pointId = face->GetPointId(pointIndex);
        if (IsSelectedItem(model, IG_POINT, pointId)) choosedNum++;
    }
    return (double) choosedNum / (double) pointSize;
}

static int CalBrightness(int minBrightNess, int maxBrightNess, double percent) {
    return (int) (percent * (double) (maxBrightNess - minBrightNess)) + minBrightNess;
}

static void ChangeBrightness(float& r, float& g, float& b, int brightness) {
    float _0_1_brightness = (double) brightness / 255.0;
    ChangeRgbBrightness(r, g, b, _0_1_brightness);
}

static double CalFacePointsAveValue(AttributeSet::Attribute& attr, Cell* cell, int variableIndex, int pointSize) {
    double sumNum{};
    if (variableIndex == -1) {
        int variableNum = attr.pointer->GetDimension();
        for (int pointIndex = 0; pointIndex < pointSize; pointIndex++) {
            auto pointId = cell->GetPointId(pointIndex);
            double pointValue{};
            for (int i = 0; i < variableNum; i++) {
                auto value = attr.pointer->GetElementValue(pointId, i);
                pointValue += value * value;
            }
            sumNum += std::sqrt(pointValue);
        }
        return sumNum / (double) pointSize;
    }
    /* variableIndex != -1 */
    for (int pointIndex = 0; pointIndex < pointSize; pointIndex++) {
        auto pointId = cell->GetPointId(pointIndex);
        sumNum += attr.pointer->GetElementValue(pointId, variableIndex);
    }
    return sumNum / (double) pointSize;
}

static double GetCellValue(AttributeSet::Attribute& attr, igIndex cellIndex, int variableIndex) {
    if (variableIndex == -1) {
        int variableNum = attr.pointer->GetDimension();
        double sumValue{};
        for (int i = 0; i < variableNum; i++) {
            auto value = attr.pointer->GetElementValue(cellIndex, i);
            sumValue += value * value;
        }
        return std::sqrt(sumValue);
    }
    return attr.pointer->GetElementValue(cellIndex, variableIndex);
}

static void DrawCellByPointAttr(Model::Pointer model, UnstructuredMesh::Pointer mesh, AttributeSet::Attribute& attr,
                                Cell* cell, int variableIndex, std::vector<IGuint>& drawHandles,
                                ScalarsToColors::Pointer colorMap, float shift, float scale, int maxBrightness,
                                int minBrightness) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 2) return;
        double selectedPointPercent = SelectedPointPerOfFace(model, cell);
        int brightness = CalBrightness(minBrightness, maxBrightness, selectedPointPercent);
        auto value = CalFacePointsAveValue(attr, cell, variableIndex, pointSize);
        float rgb[3]{};
        colorMap->GetColor(value, rgb, shift, scale);
        ChangeBrightness(rgb[0], rgb[1], rgb[2], brightness);
        auto painter = model->GetPainter3D();
        painter->SetPen(Color::Black);
        painter->SetPen(1);
        painter->SetBrush(rgb[0], rgb[1], rgb[2]);
        auto& p0 = cell->GetPoint(0);
        for (int i = 1; i < pointSize - 1; i++) {
            auto& p1 = cell->GetPoint(i);
            auto& p2 = cell->GetPoint(i + 1);
            auto drawHandle = painter->DrawTriangle(p0, p1, p2);
            drawHandles.push_back(drawHandle);
        }
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            DrawCellByPointAttr(model, mesh, attr, face, variableIndex, drawHandles, colorMap, shift, scale,
                                maxBrightness, minBrightness);
        }
    }
}

static void DrawCell(Model::Pointer model, UnstructuredMesh::Pointer mesh, AttributeSet::Attribute& attr, Cell* cell,
                     int variableIndex, std::vector<IGuint>& drawHandles, ScalarsToColors::Pointer colorMap,
                     float shift, float scale, int brightness,double value) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 2) return;
        float rgb[3]{};
        colorMap->GetColor(value, rgb, shift, scale);
        ChangeBrightness(rgb[0], rgb[1], rgb[2], brightness);
        auto painter = model->GetPainter3D();
        painter->SetPen(Color::Black);
        painter->SetPen(1);
        painter->SetBrush(rgb[0], rgb[1], rgb[2]);
        auto& p0 = cell->GetPoint(0);
        for (int i = 1; i < pointSize - 1; i++) {
            auto& p1 = cell->GetPoint(i);
            auto& p2 = cell->GetPoint(i + 1);
            auto drawHandle = painter->DrawTriangle(p0, p1, p2);
            drawHandles.push_back(drawHandle);
        }
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            DrawCell(model, mesh, attr, face, variableIndex, drawHandles, colorMap, shift, scale, brightness, value);
        }
    }
}

static void GetMaxMinValueInCellByPoint(AttributeSet::Attribute& attr, Cell* cell, int variableIndex,
                                           double& maxValue, double& minValue) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        if (pointSize <= 2) return;
        auto value = CalFacePointsAveValue(attr, cell, variableIndex, pointSize);
        maxValue = std::max(maxValue, value);
        minValue = std::min(minValue, value);
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            GetMaxMinValueInCellByPoint(attr, face, variableIndex, maxValue, minValue);
        }
    }
}

static void GetMaxMinValueInCell(AttributeSet::Attribute& attr, igIndex cellIndex, int variableIndex, double& maxValue,
                                 double& minValue) {
    auto value = GetCellValue(attr, cellIndex, variableIndex);
    maxValue = std::max(maxValue, value);
    minValue = std::min(minValue, value);
}

//Each face of the cell determines its value according to the value of the corresponding points.
void igQtContextPreservingShowWidget::DrawPointAttr(AttributeSet::Attribute& attr, int variableIndex) {
    double minValue{std::numeric_limits<float>::max()};
    double maxValue{-minValue};
    int cellNum = m_Mesh->GetNumberOfCells();
    for (int cellIndex = 0; cellIndex < cellNum; cellIndex++) {
        auto cell = m_Mesh->GetCell(cellIndex);
        GetMaxMinValueInCellByPoint(attr, cell, variableIndex, maxValue, minValue);
    }
    float shift = 0 - minValue;
    float scale = 1.0 / (maxValue - minValue);
    auto colorMapper = m_Mesh->GetColorMapper();
    for (int cellIndex = 0; cellIndex < cellNum; cellIndex++) {
        auto cell = m_Mesh->GetCell(cellIndex);
        std::vector<IGuint> handles;
        DrawCellByPointAttr(m_Model, m_Mesh, attr, cell, variableIndex, handles, colorMapper, shift, scale,
                            m_MaxBrightness, m_MinBrightness);
        m_ObjDrawHandles.push_back(handles);
    }
}

void igQtContextPreservingShowWidget::DrawCellAttr(AttributeSet::Attribute& attr, int variableIndex) {
    double minValue{std::numeric_limits<float>::max()};
    double maxValue{-minValue};
    int cellNum = m_Mesh->GetNumberOfCells();
    for (int cellIndex = 0; cellIndex < cellNum; cellIndex++) {
        GetMaxMinValueInCell(attr, cellIndex, variableIndex, maxValue, minValue);
    }
    float shift = 0 - minValue;
    float scale = 1.0 / (maxValue - minValue);
    auto colorMapper = m_Mesh->GetColorMapper();
    for (int cellIndex = 0; cellIndex < cellNum; cellIndex++) {
        auto cell = m_Mesh->GetCell(cellIndex);
        std::vector<IGuint> handles;
        int brightness{};
        if (IsSelectedItem(m_Model, IG_CELL, cellIndex)) brightness = m_MaxBrightness;
        else
            brightness = m_MinBrightness;
        double value = GetCellValue(attr, cellIndex, variableIndex);
        DrawCell(m_Model, m_Mesh, attr, cell, variableIndex, handles, colorMapper, shift, scale, brightness, value);
        m_ObjDrawHandles.push_back(handles);
    }
}

void igQtContextPreservingShowWidget::SetSelectionCallBack() {
    auto selection = m_Model->GetSelection();
    selection->SetSelectionCallBackEvent(&igQtContextPreservingShowWidget::SelectionCallbackEvent, this,
                                         std::placeholders::_1);
}

//Set m_Attrs And m_AttrsNames
void igQtContextPreservingShowWidget::SetAttrs() {
    if (m_Mesh.IsNull()) return;
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    m_Attrs.clear();
    m_AttrsNames.clear();
    m_Attrs.push_back({-1, -1});
    m_AttrsNames.push_back("none");
    for (int attrGroupIndex = 0; attrGroupIndex < attrs->Size(); attrGroupIndex++) {
        auto& attr = attrs->GetElement(attrGroupIndex);
        std::string suffix;
        if (attr.attachmentType == IG_POINT) suffix = "(POINT)";
        else if (attr.attachmentType == IG_CELL)
            suffix = "(CELL)";
        if (attr.pointer->GetDimension() == 1) {
            m_Attrs.push_back({attrGroupIndex, 0});
            m_AttrsNames.push_back(attr.pointer->GetName() + suffix);
            continue;
        }
        /* attr.pointer->GetDimension() != 1 */
        m_Attrs.push_back({attrGroupIndex, -1});
        m_AttrsNames.push_back(attr.pointer->GetName() + "_magnitude" + suffix);
        for (int j = 1; j <= attr.pointer->GetDimension(); j++) {
            std::stringstream ss;
            ss << attr.pointer->GetName() << "_" << j;
            m_Attrs.push_back({attrGroupIndex, j - 1});
            m_AttrsNames.push_back(ss.str() + suffix);
        }
    }
    m_ShowingAttrIndex = 0;
}

void igQtContextPreservingShowWidget::SetShowingAttrIndex(int index) {
    if (index < 0 || m_Attrs.size() <= index) {
        m_ShowingAttrIndex = 0;
        return;
    }
    m_ShowingAttrIndex = index;
}

void igQtContextPreservingShowWidget::SetChoosedDataComboBox() {
    ui->chooesdData->clear();
    for (auto& attrName: m_AttrsNames) { ui->chooesdData->addItem(attrName.c_str()); }
}

void igQtContextPreservingShowWidget::ChoosedDataChanged(int index) {
    SetShowingAttrIndex(index);
    UpdateDraw();
}

void igQtContextPreservingShowWidget::Slot_Refresh() { SetContextPreserving(m_Model); }

void igQtContextPreservingShowWidget::Slot_ChooesdLightSliderChanged(int light) {
    if (m_MaxBrightness == light) return;
    m_MaxBrightness = light;
    if (ui->choosedLightSpinBox->value() == m_MaxBrightness) return;
    ui->choosedLightSpinBox->setValue(m_MaxBrightness);
    UpdateDraw();
}

void igQtContextPreservingShowWidget::Slot_ChoosedLightSpinBoxChanged(int light) {
    if (m_MaxBrightness == light) return;
    m_MaxBrightness = light;
    if (ui->choosedLightSlider->value() == m_MaxBrightness) return;
    ui->choosedLightSlider->setValue(m_MaxBrightness);
    UpdateDraw();
}

void igQtContextPreservingShowWidget::Slot_UnChoosedLightSliderChanged(int light) {
    if (m_MinBrightness == light) return;
    m_MinBrightness = light;
    if (ui->unChoosedLightSpinBox->value() == m_MinBrightness) return;
    ui->unChoosedLightSpinBox->setValue(m_MinBrightness);
    UpdateDraw();
}

void igQtContextPreservingShowWidget::Slot_UnChoosedLightSpinBoxChanged(int light) {
    if (m_MinBrightness == light) return;
    m_MinBrightness = light;
    if (ui->unChoosedLightSlider->value() == m_MinBrightness) return;
    ui->unChoosedLightSlider->setValue(m_MinBrightness);
    UpdateDraw();
}

void igQtContextPreservingShowWidget::SelectionCallbackEvent(const std::vector<Selection::Event>& _events) {
    UpdateDraw();
}

void igQtContextPreservingShowWidget::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    emit Hided();
}
