#include "iGameSliceFilter.h"

#include "iGameThreadPool.h"
IGAME_NAMESPACE_BEGIN
SliceFilter::SliceFilter() {

    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
    m_Contourer = iGame::ContourFilter::New();
    m_PlaneOrigin[0] = 0;
    m_PlaneOrigin[1] = 0;
    m_PlaneOrigin[2] = 0;
    m_PlaneNormal[0] = 1;
    m_PlaneNormal[1] = 0;
    m_PlaneNormal[2] = 0;
}


SliceFilter::~SliceFilter() {}

void SliceFilter::SetPlane(double ox, double oy, double oz, double nx, double ny, double nz)
{
    m_PlaneOrigin[0] = ox;
    m_PlaneOrigin[1] = oy;
    m_PlaneOrigin[2] = oz;
    m_PlaneNormal[0] = nx;
    m_PlaneNormal[1] = ny;
    m_PlaneNormal[2] = nz;
}

bool SliceFilter::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) { return false; }
    auto input = m_Inputs->GetElement(0);
    if (!input) { return false; }

    m_Contourer->SetInput(input);
    m_Contourer->SetPlane(m_PlaneOrigin, m_PlaneNormal);

    bool result = m_Contourer->Execute();
    this->SetOutput(m_Contourer->GetOutput());
    return result;

}

IGAME_NAMESPACE_END