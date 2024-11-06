#include "iGameModelClip.h"
#include "iGameCellClip.h"
IGAME_NAMESPACE_BEGIN

ModelClip::ModelClip()
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
	this->m_Slice = false;
}
ModelClip::~ModelClip()
{

}
bool ModelClip::Execute()
{
	if (m_Inputs->GetNumberOfElements() == 0) {
		return false;
	}
	auto input = m_Inputs->GetElement(0);
	if (!input) {
		return false;
	}
	switch (input->GetDataObjectType())
	{
	case IG_NONE:
		return true;
	case IG_VOLUME_MESH:
		return this->ExecuteWithVolumeMesh(DynamicCast<VolumeMesh>(input));
	case IG_SURFACE_MESH:
		return this->ExecuteWithSurfaceMesh(DynamicCast<SurfaceMesh>(input));
	case IG_UNSTRUCTURED_MESH:
		return this->ExecuteWithUnstructuredMesh(DynamicCast<UnstructuredMesh>(input));
	case IG_STRUCTURED_MESH:
		return this->ExecuteWithVolumeMesh(DynamicCast<VolumeMesh>(input));
	default:
		break;
	}
	return true;
}



bool ModelClip::ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um)
{
	m_UnstructuredMesh = um;
	if (!m_UnstructuredMesh)return false;
	AttributeSet::Pointer inData = m_UnstructuredMesh->GetAttributeSet();
	AttributeSet::Pointer outData = AttributeSet::New();

	CellArray::Pointer OutConn = CellArray::New();
	UnsignedIntArray::Pointer OutType = UnsignedIntArray::New();
	Points::Pointer OutPoints = Points::New();
	UnstructuredMesh::Pointer OutMesh = UnstructuredMesh::New();
	std::vector<CellClip::InterpolateEdge>OriginEdge;
	std::vector<igIndex> originCell;
	auto Points = m_UnstructuredMesh->GetPoints();
	auto PointNum = m_UnstructuredMesh->GetNumberOfPoints();
	//OutPoints->Resize(PointNum);
	//std::copy(Points->RawPointer(), Points->RawPointer() + PointNum * 3, OutPoints->RawPointer());
	igIndex PointId = 0;
	FloatArray::Pointer PointClipArray = FloatArray::New();
	PointClipArray->Resize(PointNum);
	float* PointClipValue = PointClipArray->RawPointer();
	for (PointId = 0; PointId < PointNum; PointId++) {
		PointClipValue[PointId] = GetPointValue(PointId, Points);
	}
	igIndex CellId = 0;
	IGsize CellNum = m_UnstructuredMesh->GetNumberOfCells();
	igIndex vcnt = 0, i = 0, j = 0, k = 0;
	igIndex* vhs = nullptr;
	float CellClipValue[IGAME_CELL_MAX_SIZE];
	Cell::Pointer cell = nullptr;
	int allIn = 1, allOut = 1;
	for (CellId = 0; CellId < CellNum; CellId++) {
		cell = m_UnstructuredMesh->GetCell(CellId);
		vhs = cell->m_PointIds->RawPointer();
		vcnt = cell->GetNumberOfPoints();
		allIn = 1;
		allOut = 1;
		for (i = 0; i < vcnt; i++) {
			CellClipValue[i] = PointClipValue[vhs[i]];
			if (CellClipValue[i] < 0.0) {
				allOut = 0;
			}
			else if (CellClipValue[i] > 0.0) {
				allIn = 0;
			}
			else {
				allIn = 0;
				allOut = 0;
			}
		}
		if (allIn || allOut) {
			continue;
		}
		switch (cell->GetCellType())
		{
		case IG_TRIANGLE:
			CellClip::Clip(DynamicCast<Triangle>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, m_Slice);
			break;
		case IG_QUAD:
			CellClip::Clip(DynamicCast<Quad>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, m_Slice);
			break;
		case IG_POLYGON:
			CellClip::Clip(DynamicCast<Polygon>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, m_Slice);
			break;
		case IG_TETRA:
			CellClip::Clip(DynamicCast<Tetra>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, m_Slice);
			break;
		case IG_QUADRATIC_TETRA:
			CellClip::Clip(DynamicCast<QuadraticTetra>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, m_Slice);
			break;
		case IG_POLYHEDRON:
			CellClip::Clip(DynamicCast<Polyhedron>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, m_Slice);
			break;
		default:
			if (Cell::GetCellDimension(cell->GetCellType()) == 3) {
				CellClip::Clip(DynamicCast<Volume>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, PointClipValue, m_Slice);
			}
			break;
		}
	}

	auto outCellNum = OutConn->GetNumberOfCells();
	auto outPointNum = OutPoints->GetNumberOfPoints();
	auto inAllAttr = inData->GetAllAttributes();
	double values[IGAME_CELL_MAX_SIZE] = { 0 };
	double values_1[IGAME_CELL_MAX_SIZE] = { 0 };
	double values_2[IGAME_CELL_MAX_SIZE] = { 0 };
	for (i = 0; i < inAllAttr->GetNumberOfElements(); i++) {
		auto attr = inAllAttr->GetElement(i);
		auto inArray = attr.pointer;
		auto outArray = FloatArray::New();
		outArray->SetName(inArray->GetName());
		outArray->SetDimension(inArray->GetDimension());
		if (attr.attachmentType == IG_CELL) {
			outArray->Resize(outCellNum);
			for (j = 0; j < outCellNum; j++) {
				inArray->GetElement(originCell[j], values);
				outArray->SetElement(j, values);
			}
			outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
		}
		else if (attr.attachmentType == IG_POINT) {
			outArray->Resize(outPointNum);
			auto dimension = inArray->GetDimension();
			for (j = 0; j < outPointNum; j++) {
				inArray->GetElement(OriginEdge[j].vh1, values_1);
				if (OriginEdge[j].vh2 == -1) {
					outArray->SetElement(j, values_1);
				}
				else {
					inArray->GetElement(OriginEdge[j].vh2, values_2);
					for (k = 0; k < dimension; k++) {
						values[k] = values_1[k] + OriginEdge[j].t * (values_2[k] - values_1[k]);
					}
					outArray->SetElement(j, values);
				}
			}
			outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
		}
	}
	OutMesh->SetCells(OutConn, OutType);
	OutMesh->SetPoints(OutPoints);
	OutMesh->SetAttributeSet(outData);
	this->SetOutput(0, OutMesh);
	std::vector<igIndex>().swap(originCell);
	std::vector<CellClip::InterpolateEdge>().swap(OriginEdge);
	return true;
}


bool ModelClip::ExecuteWithVolumeMesh(VolumeMesh::Pointer vm)
{
	m_VolumeMesh = vm;
	if (!m_VolumeMesh)return false;
	AttributeSet::Pointer inData = m_VolumeMesh->GetAttributeSet();
	AttributeSet::Pointer outData = AttributeSet::New();
	CellArray::Pointer OutConn = CellArray::New();
	UnsignedIntArray::Pointer OutType = UnsignedIntArray::New();
	Points::Pointer OutPoints = Points::New();
	UnstructuredMesh::Pointer OutMesh = UnstructuredMesh::New();
	std::vector<CellClip::InterpolateEdge>OriginEdge;
	std::vector<igIndex> originCell;
	auto Points = m_VolumeMesh->GetPoints();
	auto PointNum = m_VolumeMesh->GetNumberOfPoints();
	igIndex PointId = 0;
	FloatArray::Pointer PointClipArray = FloatArray::New();
	PointClipArray->Resize(PointNum);
	float* PointClipValue = PointClipArray->RawPointer();
	for (PointId = 0; PointId < PointNum; PointId++) {
		PointClipValue[PointId] = GetPointValue(PointId, Points);
	}
	igIndex CellId = 0;
	IGsize CellNum = m_VolumeMesh->GetNumberOfVolumes();
	igIndex* vhs = nullptr;
	igIndex vcnt = 0, i = 0, j = 0, k = 0;
	float CellClipValue[IGAME_CELL_MAX_SIZE];
	Cell::Pointer cell;
	int allIn = 1, allOut = 1;
	if (m_VolumeMesh->GetIsPolyhedronType()) {
		auto faces = m_VolumeMesh->GetFaces();
		igIndex fhs[IGAME_CELL_MAX_SIZE] = { 0 };
		igIndex fcnt = 0;
		igIndex faceVhs[IGAME_CELL_MAX_SIZE] = { 0 };
		Polyhedron::Pointer polyhedron = Polyhedron::New();
		igIndex offset = 0;
		for (CellId = 0; CellId < CellNum; CellId++) {
			fcnt = m_VolumeMesh->GetVolumeFaceIds(CellId, fhs);
			polyhedron->m_Points->Reset();
			polyhedron->m_PointIds->Reset();
			polyhedron->m_FaceOffset->Reset();
			offset = 0;
			polyhedron->m_FaceOffset->AddId(offset);
	
			for (i = 0; i < fcnt; i++) {
				vcnt = faces->GetCellIds(fhs[i], faceVhs);
				for (j = 0; j < vcnt; j++) {
					polyhedron->m_PointIds->AddId(faceVhs[j]);
					polyhedron->m_Points->AddPoint(Points->GetPoint(faceVhs[j]));
				}
				offset += vcnt;
				polyhedron->m_FaceOffset->AddId(offset);
			}
			vhs = polyhedron->m_PointIds->RawPointer();
			vcnt = polyhedron->GetNumberOfPoints();
			allIn = 1;
			allOut = 1;
			for (i = 0; i < vcnt; i++) {
				CellClipValue[i] = PointClipValue[vhs[i]];
				if (CellClipValue[i] < 0.0) {
					allOut = 0;
				}
				else if (CellClipValue[i] > 0.0) {
					allIn = 0;
				}
				else {
					allIn = 0;
					allOut = 0;
				}
			}
			if (allIn || allOut) {
				continue;
			}
			CellClip::Clip(polyhedron, CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, m_Slice);
		}
	}
	else {
		for (CellId = 0; CellId < CellNum; CellId++) {
			cell = m_VolumeMesh->GetVolume(CellId);
			vhs = cell->m_PointIds->RawPointer();
			vcnt = cell->GetNumberOfPoints();
			allIn = 1;
			allOut = 1;
			for (i = 0; i < vcnt; i++) {
				CellClipValue[i] = PointClipValue[vhs[i]];
				if (CellClipValue[i] < 0.0) {
					allOut = 0;
				}
				else if (CellClipValue[i] > 0.0) {
					allIn = 0;
				}
				else {
					allIn = 0;
					allOut = 0;
				}
			}
			if (allIn || allOut) {
				continue;
			}
			switch (cell->GetCellType())
			{
			case IG_TETRA:
				CellClip::Clip(DynamicCast<Tetra>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, m_Slice);
				break;
			default:
				if (Cell::GetCellDimension(cell->GetCellType()) == 3) {
					CellClip::Clip(DynamicCast<Volume>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, PointClipValue, m_Slice);
				}
				break;
			}
		}

	}

	auto outCellNum = OutConn->GetNumberOfCells();
	auto outPointNum = OutPoints->GetNumberOfPoints();
	auto inAllAttr = inData->GetAllAttributes();
	double values[IGAME_CELL_MAX_SIZE] = { 0 };
	double values_1[IGAME_CELL_MAX_SIZE] = { 0 };
	double values_2[IGAME_CELL_MAX_SIZE] = { 0 };
	for (i = 0; i < inAllAttr->GetNumberOfElements(); i++) {
		auto attr = inAllAttr->GetElement(i);
		auto inArray = attr.pointer;
		auto outArray = FloatArray::New();
		outArray->SetName(inArray->GetName());
		outArray->SetDimension(inArray->GetDimension());
		if (attr.attachmentType == IG_CELL) {
			outArray->Resize(outCellNum);
			for (j = 0; j < outCellNum; j++) {
				inArray->GetElement(originCell[j], values);
				outArray->SetElement(j, values);
			}
			outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
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
			outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
		}
	}
	OutMesh->SetCells(OutConn, OutType);
	OutMesh->SetPoints(OutPoints);
	OutMesh->SetAttributeSet(outData);
	this->SetOutput(0, OutMesh);
	std::vector<igIndex>().swap(originCell);
	std::vector<CellClip::InterpolateEdge>().swap(OriginEdge);
	return true;
}
bool ModelClip::ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm)
{
	m_SurfaceMesh = sm;
	if (!m_SurfaceMesh)return false;
	AttributeSet::Pointer inData = m_SurfaceMesh->GetAttributeSet();
	AttributeSet::Pointer outData = AttributeSet::New();

	CellArray::Pointer OutConn = CellArray::New();
	UnsignedIntArray::Pointer OutType = UnsignedIntArray::New();
	Points::Pointer OutPoints = Points::New();
	UnstructuredMesh::Pointer OutMesh = UnstructuredMesh::New();
	std::vector<CellClip::InterpolateEdge>OriginEdge;
	std::vector<igIndex> originCell;
	auto Points = m_SurfaceMesh->GetPoints();
	auto PointNum = m_SurfaceMesh->GetNumberOfPoints();
	igIndex PointId = 0;
	FloatArray::Pointer PointClipArray = FloatArray::New();
	PointClipArray->Resize(PointNum);
	float* PointClipValue = PointClipArray->RawPointer();
	for (PointId = 0; PointId < PointNum; PointId++) {
		PointClipValue[PointId] = GetPointValue(PointId, Points);
	}
	igIndex CellId = 0;
	IGsize CellNum = m_SurfaceMesh->GetNumberOfFaces();
	igIndex* vhs = nullptr;
	igIndex vcnt = 0, i = 0, j = 0, k = 0;
	float CellClipValue[IGAME_CELL_MAX_SIZE];
	Face::Pointer cell;
	int allIn=1,allOut=1;
	for (CellId = 0; CellId < CellNum; CellId++) {
		cell = m_SurfaceMesh->GetFace(CellId);
		vhs = cell->m_PointIds->RawPointer();
		vcnt = cell->GetNumberOfPoints();
		allIn = 1;
		allOut = 1;
		for (i = 0; i < vcnt; i++) {
			CellClipValue[i] = PointClipValue[vhs[i]];
			if (CellClipValue[i] < 0.0) {
				allOut = 0;
			}
			else if (CellClipValue[i] > 0.0) {
				allIn = 0;
			}
			else {
				allIn = 0;
				allOut = 0;
			}
		}
		if (allIn || allOut) {
			continue;
		}
		switch (cell->GetNumberOfPoints())
		{
		case 3:
			CellClip::Clip(DynamicCast<Triangle>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, m_Slice);
			break;
		case 4:
			CellClip::Clip(DynamicCast<Quad>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, m_Slice);
			break;
		default:
			CellClip::Clip(DynamicCast<Polygon>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, originCell, m_Slice);
			break;
		}
	}

	auto outCellNum = OutConn->GetNumberOfCells();
	auto outPointNum = OutPoints->GetNumberOfPoints();
	auto inAllAttr = inData->GetAllAttributes();
	double values[IGAME_CELL_MAX_SIZE] = { 0 };
	double values_1[IGAME_CELL_MAX_SIZE] = { 0 };
	double values_2[IGAME_CELL_MAX_SIZE] = { 0 };
	for (i = 0; i < inAllAttr->GetNumberOfElements(); i++) {
		auto attr = inAllAttr->GetElement(i);
		auto inArray = attr.pointer;
		auto outArray = FloatArray::New();
		outArray->SetName(inArray->GetName());
		outArray->SetDimension(inArray->GetDimension());
		if (attr.attachmentType == IG_CELL) {
			outArray->Resize(outCellNum);
			for (j = 0; j < outCellNum; j++) {
				inArray->GetElement(originCell[j], values);
				outArray->SetElement(j, values);
			}
			outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
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
			outData->AddAttribute(attr.type, attr.attachmentType, outArray,attr.GetDataRange());
		}
	}
	OutMesh->SetCells(OutConn, OutType);
	OutMesh->SetPoints(OutPoints);
	OutMesh->SetAttributeSet(outData);
	this->SetOutput(0, OutMesh);
	std::vector<igIndex>().swap(originCell);
	std::vector<CellClip::InterpolateEdge>().swap(OriginEdge);
	return true;
}
IGAME_NAMESPACE_END