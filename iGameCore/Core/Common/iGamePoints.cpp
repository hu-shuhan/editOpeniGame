#include "iGamePoints.h"

IGAME_NAMESPACE_BEGIN
void Points::Initialize() { m_Buffer->Initialize(); }

void Points::Reserve(const IGsize _NewPointsNum) {
    m_Buffer->Reserve(_NewPointsNum);
}

void Points::Resize(const IGsize _NewPointsNum) {
    m_Buffer->Resize(_NewPointsNum);
}

void Points::Reset() { m_Buffer->Reset(); }

void Points::Squeeze() { m_Buffer->Squeeze(); }

void Points::SetNumberOfPoints(const IGsize _NewPointsNum) {
    this->Resize(_NewPointsNum);
}

IGsize Points::GetNumberOfPoints() const noexcept {
    return m_Buffer->GetNumberOfElements();
}

void Points::GetPoint(const IGsize ptId, Vector3d& p) {
    m_Buffer->GetElement(ptId, p.pointer());
}
void Points::GetPoint(const IGsize ptId, Vector3f& p) {
    m_Buffer->GetElement(ptId, p.pointer());
}
Point& Points::GetPoint(const IGsize ptId) {
    float* ptr = m_Buffer->RawPointer() + ptId * 3;
    Point* p = reinterpret_cast<Point*>(ptr);
    return *p;
}
const Point& Points::GetPoint(const IGsize ptId) const {
    float* ptr = m_Buffer->RawPointer() + ptId * 3;
    Point* p = reinterpret_cast<Point*>(ptr);
    return *p;
}

void Points::SetPoint(const IGsize ptId, const Vector3d& p) {
    Modified();
    Vector3f pp(p);
    m_Buffer->SetElement(ptId, pp);
}
void Points::SetPoint(const IGsize ptId, const Vector3f& p) {
    Modified();
    m_Buffer->SetElement(ptId, p.pointer());
}
void Points::SetPoint(const IGsize ptId, float x, float y, float z) {
    Modified();
    Vector3f p(x, y, z);
    m_Buffer->SetElement(ptId, p);
}

IGsize Points::AddPoint(const Vector3d& p) {
    Modified();
    Vector3f pp(p);
    return m_Buffer->AddElement(pp);
}
IGsize Points::AddPoint(const Vector3f& p) {
    Modified();
    return m_Buffer->AddElement(p);
}
IGsize Points::AddPoint(float x, float y, float z) {
    Modified();
    Vector3f p(x, y, z);
    return m_Buffer->AddElement(p);
}
IGsize Points::AddPoint(float p[3]) {
    Modified();
    return m_Buffer->AddElement(p);
}
IGsize Points::AddPoint(double p[3]) {
    Modified();
    float pp[3]{(float) p[0], (float) p[1], (float) p[2]};
    return m_Buffer->AddElement(pp);
}

FloatArray::Pointer Points::ConvertToArray() { return m_Buffer; }

float* Points::RawPointer() { return m_Buffer->RawPointer(); }

Points::Points() {
    m_Buffer = FloatArray::New();
    m_Buffer->SetDimension(3);
}
IGAME_NAMESPACE_END

/*
 #include "iGamePoints.h"

IGAME_NAMESPACE_BEGIN
void Points::Initialize() { m_Buffer->Initialize(); }

void Points::Reserve(const IGsize _NewPointsNum) {
   IGsize currentCapacity = m_Buffer->GetCapacity();
   IGsize newCapacity = _NewPointsNum * m_Buffer->GetDimension();

   if (newCapacity != currentCapacity) {
       m_Buffer->Reserve(_NewPointsNum);
       this->Modified();
   }
}

void Points::Resize(const IGsize _NewPointsNum) {
   IGsize currentSize = m_Buffer->GetNumberOfElements();
   IGsize newSize = _NewPointsNum;

   if (newSize != currentSize) {
       m_Buffer->Resize(_NewPointsNum);
       this->Modified();
   }
}

void Points::Reset() {
   m_Buffer->Reset();
   this->Modified();
}

void Points::Squeeze() {
   IGsize currentCapacity = m_Buffer->GetCapacity();
   IGsize newCapacity =
           m_Buffer->GetNumberOfElements() * m_Buffer->GetDimension();

   if (newCapacity != currentCapacity) { m_Buffer->Squeeze(); }
}

void Points::SetNumberOfPoints(const IGsize _NewPointsNum) {
   this->Resize(_NewPointsNum);
}

IGsize Points::GetNumberOfPoints() const noexcept {
   return m_Buffer->GetNumberOfElements();
}

void Points::GetPoint(const IGsize ptId, Vector3d& p) {
   m_Buffer->GetElement(ptId, p.pointer());
}
void Points::GetPoint(const IGsize ptId, Vector3f& p) {
   m_Buffer->GetElement(ptId, p.pointer());
}
Point& Points::GetPoint(const IGsize ptId) {
   float* ptr = m_Buffer->RawPointer() + ptId * 3;
   Point* p = reinterpret_cast<Point*>(ptr);
   return *p;
}
const Point& Points::GetPoint(const IGsize ptId) const {
   float* ptr = m_Buffer->RawPointer() + ptId * 3;
   Point* p = reinterpret_cast<Point*>(ptr);
   return *p;
}

void Points::SetPoint(const IGsize ptId, const Vector3d& p) {
   Vector3f pp(p);
   m_Buffer->SetElement(ptId, pp);
   this->Modified();
}
void Points::SetPoint(const IGsize ptId, const Vector3f& p) {
   m_Buffer->SetElement(ptId, p.pointer());
   this->Modified();
}
void Points::SetPoint(const IGsize ptId, float x, float y, float z) {
   Vector3f p(x, y, z);
   m_Buffer->SetElement(ptId, p);
   this->Modified();
}

IGsize Points::AddPoint(const Vector3d& p) {
   Modified();
   Vector3f pp(p);
   this->Modified();
   return m_Buffer->AddElement(pp);
}
IGsize Points::AddPoint(const Vector3f& p) {
   this->Modified();
   return m_Buffer->AddElement(p);
}
IGsize Points::AddPoint(float x, float y, float z) {
   this->Modified();
   Vector3f p(x, y, z);
   return m_Buffer->AddElement(p);
}
IGsize Points::AddPoint(float p[3]) {
   this->Modified();
   return m_Buffer->AddElement(p);
}
IGsize Points::AddPoint(double p[3]) {
   this->Modified();
   float pp[3]{(float) p[0], (float) p[1], (float) p[2]};
   return m_Buffer->AddElement(pp);
}

FloatArray::Pointer Points::ConvertToArray() { return m_Buffer; }

float* Points::RawPointer() { return m_Buffer->RawPointer(); }

Points::Points() {
   m_Buffer = FloatArray::New();
   m_Buffer->SetDimension(3);
   this->Modified();
}
IGAME_NAMESPACE_END
*/
