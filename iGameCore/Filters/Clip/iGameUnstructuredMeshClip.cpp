#include "iGameUnstructuredMeshClip.h"
#include "iGameCellClip.h"
IGAME_NAMESPACE_BEGIN

UnstructuredMeshClip::UnstructuredMeshClip()
{
	this->SetNumberOfInputs(1);
	this->SetNumberOfOutputs(1);
	m_CutPlane[0] = 0.0;
	m_CutPlane[1] = 0.0;
	m_CutPlane[2] = 0.0;
	m_CutPlane[3] = 1.0;

	m_Normal[0] = 0.0;
	m_Normal[1] = 0.0;
	m_Normal[2] = 0.0;
	m_Origin[0] = 0.0;
	m_Origin[1] = 0.0;
	m_Origin[2] = 0.0;
}
UnstructuredMeshClip::~UnstructuredMeshClip()
{

}

bool UnstructuredMeshClip::Execute()
{
	m_UnstructuredMesh = DynamicCast<UnstructuredMesh>(this->GetInput(0));
	if (!m_UnstructuredMesh)return false;
	AttributeSet::Pointer inData = m_UnstructuredMesh->GetAttributeSet();



	AttributeSet::Pointer outData = AttributeSet::New();





	CellArray::Pointer m_OutConn = CellArray::New();
	UnsignedIntArray::Pointer m_OutType = UnsignedIntArray::New();
	Points::Pointer m_OutPoints = Points::New();
	UnstructuredMesh::Pointer m_OutMesh = UnstructuredMesh::New();
	std::vector<CellClip::InterpolateEdge>OriginEdge;
	std::vector<igIndex> originCell;
	auto Points = m_UnstructuredMesh->GetPoints();
	auto PointNum = m_UnstructuredMesh->GetNumberOfPoints();
	//m_OutPoints->Resize(PointNum);
	//std::copy(Points->RawPointer(), Points->RawPointer() + PointNum * 3, m_OutPoints->RawPointer());
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
	igIndex vcnt = 0, i = 0, j = 0, k = 0;
	float CellClipValue[IGAME_CELL_MAX_SIZE];
	Cell::Pointer cell;
	for (CellId = 0; CellId < CellNum; CellId++) {
		cell = m_UnstructuredMesh->GetCell(CellId);
		vcnt = m_UnstructuredMesh->GetCellPointIds(CellId, vhs);
		for (i = 0; i < vcnt; i++) {
			CellClipValue[i] = PointClipValue[vhs[i]];
		}
		switch (cell->GetCellType())
		{
		case IG_TETRA:
			CellClip::Clip(DynamicCast<Tetra>(cell), CellClipValue, m_OutPoints, m_OutConn, m_OutType, nullptr, nullptr, CellId, OriginEdge, originCell);
			break;
		case IG_QUADRATIC_TETRA:
			CellClip::Clip(DynamicCast<QuadraticTetra>(cell), CellClipValue, m_OutPoints, m_OutConn, m_OutType, nullptr, nullptr, CellId, OriginEdge, originCell);
			break;
		default:
			if (Cell::GetCellDimension(cell->GetCellType()) == 3) {
				CellClip::Clip(DynamicCast<Volume>(cell), CellClipValue, m_OutPoints, m_OutConn, m_OutType, nullptr, nullptr, CellId, OriginEdge, originCell);
			}
			break;
		}
	}

	auto outCellNum = m_OutConn->GetNumberOfCells();
	auto outPointNum = m_OutPoints->GetNumberOfPoints();
	auto inAllAttr = inData->GetAllAttributes();
	double values[IGAME_CELL_MAX_SIZE] = { 0 };
	double values_1[IGAME_CELL_MAX_SIZE] = { 0 };
	double values_2[IGAME_CELL_MAX_SIZE] = { 0 };
	for (i = 0; i < inAllAttr->GetNumberOfElements(); i++) {
		auto attr = inAllAttr->GetElement(i);
		auto inArray = attr.pointer;
		auto outArray = FloatArray::New();
		if (attr.attachmentType == IG_CELL) {
			outArray->Resize(outCellNum);
			for (j = 0; j < outCellNum; j++) {
				inArray->GetElement(originCell[j], values);
				outArray->SetElement(j, values);
			}
			outData->AddAttribute(attr.type, attr.attachmentType, outArray);
		}
		else if (attr.attachmentType == IG_POINT) {
			outArray->Resize(outPointNum);
			for (j = 0; j < outPointNum; j++) {
				inArray->GetElement(OriginEdge[j].vh1, values_1);
				if (OriginEdge[j].vh2 == -1) {
					outArray->SetElement(j, values_1);
				}
				else {
					inArray->GetElement(OriginEdge[j].vh2, values_2);
					auto dimension = inArray->GetDimension();
					for (k = 0; k < dimension; k++) {
						values[k] = values_1[k] + OriginEdge[j].t * (values_2[k] - values_1[k]);
					}
					outArray->SetElement(j, values);
				}

			}
			outData->AddAttribute(attr.type, attr.attachmentType, outArray);
		}
	}

	//std::cout<<m_OutConn->GetNumberOfCells()<<'\n';
	m_OutMesh->SetCells(m_OutConn, m_OutType);
	m_OutMesh->SetPoints(m_OutPoints);
	m_OutMesh->SetAttributeSet(outData);
	this->SetOutput(0, m_OutMesh);
	return true;
}
IGAME_NAMESPACE_END