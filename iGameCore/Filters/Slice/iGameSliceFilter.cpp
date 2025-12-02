#include "iGameSliceFilter.h"

#include "iGameThreadPool.h"
#include "iGamePointSet.h"
#include "iGameFlatArray.h"
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
    
    // Compute point-to-plane signed distances and set as scalar data
    // All mesh types (UnstructuredMesh, SurfaceMesh, VolumeMesh, etc.) inherit from PointSet
    auto PointSet = DynamicCast<iGame::PointSet>(input);
    if (PointSet == nullptr) return false;
    auto Points = PointSet->GetPoints();
    auto PointNum = PointSet->GetNumberOfPoints();
    if (PointNum == 0) return false;
    DoubleArray::Pointer ScalarData = DoubleArray::New();
    ScalarData->Resize(PointNum);
    double* scalarData = ScalarData->RawPointer();
    Point p{0, 0, 0};
    for (int i = 0; i < PointNum; i++) {
        p = Points->GetPoint(i);
        scalarData[i] = m_PlaneNormal[0] * (p[0] - m_PlaneOrigin[0]) + 
                        m_PlaneNormal[1] * (p[1] - m_PlaneOrigin[1]) + 
                        m_PlaneNormal[2] * (p[2] - m_PlaneOrigin[2]);
    }
    m_Contourer->SetIsoScalarData(ScalarData, 0.0, 0);

    bool result = m_Contourer->Execute();
    this->SetOutput(m_Contourer->GetOutput());
    return result;

}

IGAME_NAMESPACE_END