#include "iGameUnstructuredMeshClip.h"

IGAME_NAMESPACE_BEGIN

UnstructuredMeshClip::UnstructuredMeshClip()
{
	m_CutPlane[0] = 0.0;
	m_CutPlane[1] = 0.0;
	m_CutPlane[2] = 0.0;
	m_CutPlane[3] = 1.0;


}
UnstructuredMeshClip::~UnstructuredMeshClip()
{

}

bool UnstructuredMeshClip::Execute()
{
	m_UnstructuredMesh = DynamicCast<UnstructuredMesh>(this->GetInput(0));
	if (!m_UnstructuredMesh)return false;

	CellArray::Pointer m_OutConn=CellArray::New();
	UnsignedIntArray::Pointer m_OutType= UnsignedIntArray::New();


	auto Points = m_UnstructuredMesh->GetPoints();
	auto PointNum = m_UnstructuredMesh->GetNumberOfPoints();
	igIndex PointId = 0;
	FloatArray::Pointer PointClipArray = FloatArray::New();
	PointClipArray->Resize(PointNum);
	float* PointClipValue = PointClipArray->RawPointer();
	Point p;
	for (PointId = 0; PointId < PointNum; PointId++) {
		p = Points->GetPoint(PointId);
		PointClipValue[PointId] = this->GetCutValue(p[0], p[1], p[2]);
	}
	igIndex CellId = 0;
	IGsize CellNum = m_UnstructuredMesh->GetNumberOfCells();
	igIndex vhs[IGAME_CELL_MAX_SIZE];
	igIndex vcnt = 0,i=0,j=0;
	float CellClipValue[IGAME_CELL_MAX_SIZE];
	for (CellId = 0; CellId < CellNum; CellId++) {

		vcnt = m_UnstructuredMesh->GetCellPointIds(CellId, vhs);

		for (i = 0; i < vcnt; i++) {
			CellClipValue[i]= PointClipValue[vhs[i]];
		}

	}


}
IGAME_NAMESPACE_END