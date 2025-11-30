#include "iGameScene.h"
#include "iGameTensorFilter.h"
#include "iGameThreadPool.h"
IGAME_NAMESPACE_BEGIN
iGameTensorFilter::iGameTensorFilter() {
    this->Modified();

    this->m_TensorManager = iGameTensorRepresentation::New();
    this->m_TensorManager->SetSliceNum(5);
    this->m_DrawGlyphPoints = Points::New();
    this->m_DrawGlyphPointOrders = IdArray::New();
    this->m_DrawGlyphAttributeSet = AttributeSet::New();
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}
iGameTensorFilter::~iGameTensorFilter() {}
void iGameTensorFilter::SetPoints(Points::Pointer points) { this->m_Points = points; }
void iGameTensorFilter::SetTensorAttributes(ArrayObject::Pointer attributes) { this->m_TensorAttributes = attributes; }


bool iGameTensorFilter::Execute() {
    auto input = m_Inputs->GetElement(0);
    if (input) {
        auto mesh = DynamicCast<iGame::PointSet>(input);
        if (mesh) { 
            this->SetPoints(mesh->GetPoints()); 
        }
        if (!m_TensorAttributes) {
            auto allAttributes = mesh->GetAttributeSet()->GetAllAttributes();
            if (!allAttributes) {
                std::cout << "No Tensor Data\n";
                return false;
            }
            iGame::ArrayObject::Pointer tensorData = nullptr;
            for (int i = 0; i < allAttributes->GetNumberOfElements(); i++) {
                auto attribute = allAttributes->GetElement(i);
                if (attribute.type == IG_TENSOR && attribute.attachmentType == IG_POINT) {
                    tensorData = attribute.pointer;
                }
            }
            m_TensorAttributes = tensorData;
        }
    }

    if (this->m_Points == nullptr) return false;
    if (this->m_TensorAttributes == nullptr) return false;
    UpdateGlyphDrawPositionData();
    UpdateGlyphDrawIndexData();
    UpdateGlyphDrawAttributeSet();
    UpdateTensorObject();
    return true;
}

void iGameTensorFilter::UpdateGlyphDrawPositionData() {
    int PointNum = this->m_Points ? this->m_Points->GetNumberOfPoints() : 0;
    Point p;
    if (PointNum == 0 || !this->m_TensorAttributes || this->m_TensorAttributes->GetNumberOfValues() != (9 * PointNum)) {
        return;
    }
    int GlyphPointNum = m_TensorManager->GetNumberOfDrawPoints();
    this->m_DrawGlyphPoints = Points::New();
    this->m_DrawGlyphPoints->Resize(PointNum * GlyphPointNum);
    auto GlyphPoints = m_DrawGlyphPoints->RawPointer();
    double t[9];
    for (int i = 0; i < PointNum; i++) {
        p = this->m_Points->GetPoint(i);
        m_TensorManager->SetPosition(p);
        for (int j = 0; j < 9; j++) { t[j] = this->m_TensorAttributes->GetValue(9 * i + j); }
        m_TensorManager->SetTensor(t);
        auto DrawPoints = m_TensorManager->GetDrawPoints()->RawPointer();
        IGsize st = i * 3 * GlyphPointNum;
        std::copy(DrawPoints, DrawPoints + 3 * GlyphPointNum, GlyphPoints + st);
    }
}
void iGameTensorFilter::UpdateGlyphDrawIndexData() {
    int PointNum = this->m_Points ? this->m_Points->GetNumberOfPoints() : 0;
    Point p;
    if (PointNum == 0 || !this->m_TensorAttributes || this->m_TensorAttributes->GetNumberOfValues() != (9 * PointNum)) {
        return;
    }
    auto GlyphPointIndexOrders = m_TensorManager->GetDrawPointIndexOrders();
    int GlyphPointNum = m_TensorManager->GetNumberOfDrawPoints();
    this->m_DrawGlyphPointOrders = IdArray::New();
    this->m_DrawGlyphPointOrders->Resize(PointNum * GlyphPointIndexOrders->GetNumberOfValues());
    
    auto func = [&](igIndex start, igIndex end) -> void {
        IGsize st = start * GlyphPointIndexOrders->GetNumberOfValues();
        for (int i = start; i < end; i++) {
            IGsize offset = i * GlyphPointNum;
            for (int j = 0; j < GlyphPointIndexOrders->GetNumberOfValues(); j++) {
                m_DrawGlyphPointOrders->SetId(st + j, offset + GlyphPointIndexOrders->GetValue(j));
            }
            st += GlyphPointIndexOrders->GetNumberOfValues();
        }
    };
    ThreadPool::parallelFor(0, PointNum, func);

    //for (int i = 0; i < PointNum; i++) {
    //    IGsize st = i * GlyphPointIndexOrders->GetNumberOfValues();
    //    IGsize offset = i * GlyphPointNum;
    //    for (int j = 0; j < GlyphPointIndexOrders->GetNumberOfValues(); j++) {
    //        //这里不太好用copy，不能保证两个类型相同
    //        //m_DrawGlyphPointOrders->SetValue(st + j, offset + GlyphPointIndexOrders->GetValue(j));
    //        m_DrawGlyphPointOrders->SetId(st + j, offset + GlyphPointIndexOrders->GetValue(j));
    //        //m_DrawGlyphPointOrders->AddValue(st + GlyphPointIndexOrders->GetValue(j));
    //    }
    //}
}
void iGameTensorFilter::UpdateGlyphDrawAttributeSet() {
    int PointNum = this->m_Points ? this->m_Points->GetNumberOfPoints() : 0;
    Point p;
    if (PointNum == 0 || !this->m_TensorAttributes || this->m_TensorAttributes->GetNumberOfValues() != (9 * PointNum)) {
        return;
    }
    int GlyphPointNum = m_TensorManager->GetNumberOfDrawPoints();
    igIndex newPNum = PointNum * GlyphPointNum;
    auto inAttributeSet = m_Inputs->GetElement(0)? m_Inputs->GetElement(0)->GetAttributeSet():nullptr;
    if (inAttributeSet == nullptr) {
        return ;
    }
    auto inPointsAttributes= inAttributeSet->GetAllPointAttributes();
    this->m_DrawGlyphAttributeSet = AttributeSet::New();
    for (int i = 0; i < inPointsAttributes->GetNumberOfElements(); i++) {
        auto inAttr = inPointsAttributes->GetElement(i);
        auto inData = inAttr.GetPointer();
        auto newData = DoubleArray::New();
        newData->SetDimension(inData->GetDimension());
        newData->Resize(newPNum);
        newData->SetName(inData->GetName());
        auto func = [&](igIndex start, igIndex end) -> void {
            double tmp[64];
            igIndex st = start* GlyphPointNum;
            for (igIndex i = start; i < end; i++) {
                    inData->GetElement(i, tmp);
                    for (int j = 0; j < GlyphPointNum; j++) {
                        newData->SetElement(st+j, tmp);
                    }
                    st+= GlyphPointNum;
            }
        };
        ThreadPool::parallelFor(0, PointNum, func);
        this->m_DrawGlyphAttributeSet->AddAttribute(inAttr.GetType(), IG_POINT,newData,inAttr.GetDataRange() );
    }
}
void iGameTensorFilter::SetGlyphScale(double s) {
    this->m_TensorManager->SetScale(s);
}

DoubleArray::Pointer iGameTensorFilter::GenerateVectorField() {
    int PointNum = this->m_Points ? this->m_Points->GetNumberOfPoints() : 0;
    if (PointNum == 0 || !this->m_TensorAttributes || this->m_TensorAttributes->GetNumberOfValues() != (9 * PointNum)) {
        return nullptr;
    }
    double vector[3];
    m_EigenVector = DoubleArray::New();
    m_EigenVector->SetDimension(3);
    m_EigenVector->SetName(this->m_TensorAttributes->GetName() + "_PrimaryFeature");
    m_EigenVector->Resize(PointNum);
    double t[9];
    for (int i = 0; i < PointNum; i++) {
        for (int j = 0; j < 9; j++) { t[j] = this->m_TensorAttributes->GetValue(9 * i + j); }
        m_TensorManager->InitTensorEigenData(t);
        m_TensorManager->GetEigenVector(0, vector);
        m_EigenVector->SetElement(i, vector);
    }
    return m_EigenVector;
}

void iGameTensorFilter::UpdateTensorObject() { 
    if (m_TensorObject == nullptr) {
        m_TensorObject = iGame::SurfaceMesh::New();
    }

    iGame::CellArray::Pointer Faces = iGame::CellArray::New();
    Faces->SetData(m_DrawGlyphPointOrders, 3);

    m_TensorObject->SetPoints(m_DrawGlyphPoints);
    m_TensorObject->SetFaces(Faces);
    m_TensorObject->SetAttributeSet(m_DrawGlyphAttributeSet);
    this->SetOutput(m_TensorObject);

}
IGAME_NAMESPACE_END