#include "iGameTensorWidgetBase.h"
#include "iGameScene.h"
IGAME_NAMESPACE_BEGIN
iGameTensorWidgetBase::iGameTensorWidgetBase() {
    this->Modified();

    this->m_TensorManager = iGameTensorRepresentation::New();
    this->m_TensorManager->SetSliceNum(5);
    this->m_DrawGlyphPoints = Points::New();
    this->m_DrawGlyphPointOrders = UnsignedIntArray::New();
    this->m_DrawGlyphColors = FloatArray::New();
    this->m_DrawGlyphColors->SetDimension(3);
}
iGameTensorWidgetBase::~iGameTensorWidgetBase() {}
void iGameTensorWidgetBase::SetPoints(Points::Pointer points) {
    this->m_Points = points;
}
void iGameTensorWidgetBase::SetTensorAttributes(
        ArrayObject::Pointer attributes) {
    this->m_TensorAttributes = attributes;
}
void iGameTensorWidgetBase::ShowTensorField() {
    UpdateGlyphDrawPositionData();
    UpdateGlyphDrawIndexData();
    UpdateGlyphDrawColor();
}
void iGameTensorWidgetBase::UpdateGlyphDrawPositionData() {
    int PointNum = this->m_Points ? this->m_Points->GetNumberOfPoints() : 0;
    Point p;
    if (PointNum == 0 || !this->m_TensorAttributes ||
        this->m_TensorAttributes->GetNumberOfValues() != (9 * PointNum)) {
        return;
    }
    int GlyphPointNum = m_TensorManager->GetNumberOfDrawPoints();
    m_DrawGlyphPoints->Resize(PointNum * GlyphPointNum);
    auto GlyphPoints = m_DrawGlyphPoints->RawPointer();
    double t[9];
    for (int i = 0; i < PointNum; i++) {
        p = this->m_Points->GetPoint(i);
        m_TensorManager->SetPosition(p);
        for (int j = 0; j < 9; j++) {
            t[j] = this->m_TensorAttributes->GetValue(9 * i + j);
        }
        m_TensorManager->SetTensor(t);
        auto DrawPoints = m_TensorManager->GetDrawPoints()->RawPointer();
        IGsize st = i * 3 * GlyphPointNum;
        std::copy(DrawPoints, DrawPoints + 3 * GlyphPointNum, GlyphPoints + st);
    }
}
void iGameTensorWidgetBase::UpdateGlyphDrawIndexData() {
    int PointNum = this->m_Points ? this->m_Points->GetNumberOfPoints() : 0;
    Point p;
    if (PointNum == 0 || !this->m_TensorAttributes ||
        this->m_TensorAttributes->GetNumberOfValues() != (9 * PointNum)) {
        return;
    }
    auto GlyphPointIndexOrders = m_TensorManager->GetDrawPointIndexOrders();
    int GlyphPointNum = m_TensorManager->GetNumberOfDrawPoints();
    m_DrawGlyphPointOrders->Resize(PointNum *
                                   GlyphPointIndexOrders->GetNumberOfValues());
    double t[9];
    for (int i = 0; i < PointNum; i++) {
        IGsize st = i * GlyphPointIndexOrders->GetNumberOfValues();
        IGsize offset = i * GlyphPointNum;
        for (int j = 0; j < GlyphPointIndexOrders->GetNumberOfValues(); j++) {
            //这里不太好用copy，不能保证两个类型相同
            m_DrawGlyphPointOrders->SetValue(
                    st + j, offset + GlyphPointIndexOrders->GetValue(j));
            //m_DrawGlyphPointOrders->AddValue(st + GlyphPointIndexOrders->GetValue(j));
        }
    }
}
void iGameTensorWidgetBase::UpdateGlyphDrawColor() {
    int PointNum = this->m_Points ? this->m_Points->GetNumberOfPoints() : 0;
    if (PointNum == 0 || this->m_PositionColors == nullptr ||
        this->m_PositionColors->GetNumberOfElements() != PointNum) {
        return;
    }
    int GlyphPointNum = m_TensorManager->GetNumberOfDrawPoints();
    float rgb[16] = {.0};
    m_DrawGlyphColors = FloatArray::New();
    m_DrawGlyphColors->SetDimension(3);
    m_DrawGlyphColors->Resize(PointNum * GlyphPointNum);
    auto GlyphColors = m_DrawGlyphColors->RawPointer();
    IGsize offset = 0;
    for (int i = 0; i < PointNum; i++) {
        m_PositionColors->GetElement(i, rgb);
        //std::cout << rgb[0] << ' ' << rgb[1] << ' ' << rgb[2] << '\n';
        for (int j = 0; j < GlyphPointNum; j++) {
            std::copy(rgb, rgb + 3, GlyphColors + offset);
            offset += 3;
            //m_DrawGlyphColors->AddValue(rgb[0]);
            //m_DrawGlyphColors->AddValue(rgb[1]);
            //m_DrawGlyphColors->AddValue(rgb[2]);
        }
    }
}
void iGameTensorWidgetBase::UpdateGlyphScale(double s) {
    this->m_TensorManager->SetScale(s);
    UpdateGlyphDrawPositionData();

    this->ConvertToDrawableData();
}

void iGameTensorWidgetBase::SetPositionColors(FloatArray::Pointer colors) {
    this->m_PositionColors = colors;
    UpdateGlyphDrawColor();

    this->ConvertToDrawableData();
}

DoubleArray::Pointer iGameTensorWidgetBase::GenerateVectorField() {
    int PointNum = this->m_Points ? this->m_Points->GetNumberOfPoints() : 0;
    if (PointNum == 0 || !this->m_TensorAttributes ||
        this->m_TensorAttributes->GetNumberOfValues() != (9 * PointNum)) {
        return nullptr;
    }
    double vector[3];
    m_EigenVector = DoubleArray::New();
    m_EigenVector->SetDimension(3);
    m_EigenVector->SetName(this->m_TensorAttributes->GetName() +
                           "_PrimaryFeature");
    m_EigenVector->Resize(PointNum);
    double t[9];
    for (int i = 0; i < PointNum; i++) {
        for (int j = 0; j < 9; j++) {
            t[j] = this->m_TensorAttributes->GetValue(9 * i + j);
        }
        m_TensorManager->InitTensorEigenData(t);
        m_TensorManager->GetEigenVector(0, vector);
        m_EigenVector->SetElement(i, vector);
    }
    return m_EigenVector;
}

void iGameTensorWidgetBase::ConvertToDrawableData() {
    m_Positions = m_DrawGlyphPoints->ConvertToArray();
    m_Positions->Modified();

    m_TriangleIndices = m_DrawGlyphPointOrders;
    m_TriangleIndices->Modified();

    if (m_DrawGlyphColors->GetNumberOfValues() != 0) {
        m_Colors = m_DrawGlyphColors;
        m_Colors->Modified();

        if (m_Colors != nullptr) { m_UseColor = true; }
    }
}
IGAME_NAMESPACE_END