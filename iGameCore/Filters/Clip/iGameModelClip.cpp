#include "iGameModelClip.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
#include "iGameThreadPool.h"
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
	//return this->ExecuteTest(input);
	return this->ExecuteTest2(DynamicCast<UnstructuredMesh>(input));
	clock_t time1 = clock();
	switch (input->GetDataObjectType())
	{
	case IG_NONE:
		return true;
	case IG_VOLUME_MESH:
		if (!this->ExecuteWithVolumeMesh(DynamicCast<VolumeMesh>(input)))return false;
		break;
	case IG_SURFACE_MESH:
		if (!this->ExecuteWithSurfaceMesh(DynamicCast<SurfaceMesh>(input)))return false;
		break;
	case IG_UNSTRUCTURED_MESH:
		if (!this->ExecuteWithUnstructuredMesh(DynamicCast<UnstructuredMesh>(input)))return false;
		break;
	case IG_STRUCTURED_MESH:
		if (!this->ExecuteWithVolumeMesh(DynamicCast<VolumeMesh>(input)))return false;
		break;
	default:
		break;
	}

	clock_t time2 = clock();
	std::cout << "first part cost " << time2 - time1 << '\n';

	if (this->GetIsSlice() == false) {
		auto ResultMesh = iGame::DrawObject::New();
		ResultMesh->AddSubDataObject(this->GetOutput());
		auto Result_ExtractPart = iGame::SurfaceMesh::New();
		double o[3];
		double n[3];
		this->GetPlane(o, n);
		iGame::iGameModelGeometryFilter::Pointer surfaceextract =
			iGame::iGameModelGeometryFilter::New();
		surfaceextract->SetClipPlane(o, n);
		surfaceextract->Execute(input, Result_ExtractPart);
		if (Result_ExtractPart) {
			ResultMesh->AddSubDataObject(Result_ExtractPart);
		}
		this->SetOutput(0, ResultMesh);
	}


	clock_t time3 = clock();
	std::cout << "second part cost " << time3 - time2 << '\n';
	return true;
}


bool ModelClip::ExecuteTest(DataObject::Pointer obj)
{
	m_UnstructuredMesh = DynamicCast<UnstructuredMesh>(obj);
	if (!m_UnstructuredMesh)return false;
	AttributeSet::Pointer inData = m_UnstructuredMesh->GetAttributeSet();
	AttributeSet::Pointer outData = AttributeSet::New();

	CellArray::Pointer OutConn = CellArray::New();
	UnsignedIntArray::Pointer OutType = UnsignedIntArray::New();
	Points::Pointer OutPoints = Points::New();
	UnstructuredMesh::Pointer OutMesh = UnstructuredMesh::New();
	std::vector<CellClip::InterpolateEdge>OriginEdge;
	std::vector<igIndex> OriginCell;

	auto inPoints = m_UnstructuredMesh->GetPoints();
	auto PointNum = m_UnstructuredMesh->GetNumberOfPoints();
	PointFinder::Pointer OutPointFinder = PointFinder::New();
	OutPointFinder->Initialize(OutPoints, m_UnstructuredMesh->GetBoundingBox(), PointNum, PointNum);
	igIndex PointId = 0;
	FloatArray::Pointer PointClipArray = FloatArray::New();
	PointClipArray->Resize(PointNum);
	float* PointClipValue = PointClipArray->RawPointer();
	for (PointId = 0; PointId < PointNum; PointId++) {
		PointClipValue[PointId] = GetPointValue(PointId, inPoints);
	}
	igIndex CellId = 0;
	IGsize CellNum = m_UnstructuredMesh->GetNumberOfCells();
	igIndex vcnt = 0, i = 0, j = 0, k = 0;
	igIndex* vhs = nullptr;
	float CellClipValue[IGAME_CELL_MAX_SIZE];
	Cell::Pointer cell = nullptr;
	int allIn = 1, allOut = 1;
	igIndex insertVhs[IGAME_CELL_MAX_SIZE] = { 0 };
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
		if (allOut) {
			continue;
		}
		if (allIn) {
			if (m_Slice)continue;
			for (i = 0; i < vcnt; i++) {
				insertVhs[i] = OutPointFinder->InsertUniquePoint(inPoints->GetPoint(vhs[i]));
				OriginEdge.emplace_back(CellClip::InterpolateEdge::InterpolateEdge(vhs[i]));
			}
			OutConn->AddCellIds(insertVhs, vcnt);
			OutType->AddValue(m_UnstructuredMesh->GetCellType(CellId));
			OriginCell.emplace_back(CellId);
			continue;
		}
		switch (cell->GetCellType())
		{
		case IG_TRIANGLE:
			CellClip::Clip(DynamicCast<Triangle>(cell), CellClipValue, OutPointFinder, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_QUAD:
			CellClip::Clip(DynamicCast<Quad>(cell), CellClipValue, OutPointFinder, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_POLYGON:
			CellClip::Clip(DynamicCast<Polygon>(cell), CellClipValue, OutPointFinder, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_TETRA:
			CellClip::Clip(DynamicCast<Tetra>(cell), CellClipValue, OutPointFinder, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_QUADRATIC_TETRA:
			CellClip::Clip(DynamicCast<QuadraticTetra>(cell), CellClipValue, OutPointFinder, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_POLYHEDRON:
			CellClip::Clip(DynamicCast<Polyhedron>(cell), CellClipValue, OutPointFinder, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		default:
			if (Cell::GetCellDimension(cell->GetCellType()) == 3) {
				CellClip::Clip(DynamicCast<Volume>(cell), CellClipValue, OutPointFinder, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, PointClipValue, m_Slice);
			}
			break;
		}
	}

	this->CopyAttributeSetData(OutPoints->GetNumberOfPoints(), OutConn->GetNumberOfCells(), inData, outData, OriginEdge, OriginCell);

	OutMesh->SetCells(OutConn, OutType);
	OutMesh->SetPoints(OutPoints);
	OutMesh->SetAttributeSet(outData);
	this->SetOutput(0, OutMesh);
	std::vector<igIndex>().swap(OriginCell);
	std::vector<CellClip::InterpolateEdge>().swap(OriginEdge);
	return true;
}

bool ModelClip::ExecuteTest2(UnstructuredMesh::Pointer um)
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
	std::vector<igIndex> OriginCell;
	auto inPoints = m_UnstructuredMesh->GetPoints();
	auto inPointNum = m_UnstructuredMesh->GetNumberOfPoints();
	auto inCells = m_UnstructuredMesh->GetCells();
	auto inTypes = m_UnstructuredMesh->GetCellTypes();



	igIndex PointId = 0;
	FloatArray::Pointer PointClipArray = FloatArray::New();
	PointClipArray->Resize(inPointNum);
	float* PointClipValue = PointClipArray->RawPointer();
	clock_t time__1 = clock();
	for (PointId = 0; PointId < inPointNum; PointId++) {
		PointClipValue[PointId] = GetPointValue(PointId, inPoints);
	}
	clock_t time__2 = clock();
	std::cout << "compute point vis cost  " << time__2 - time__1 << '\n';
	igIndex CellId = 0;
	IGsize CellNum = m_UnstructuredMesh->GetNumberOfCells();
	igIndex vcnt = 0, i = 0, j = 0, k = 0;
	float CellClipValue[IGAME_CELL_MAX_SIZE];
	Cell::Pointer cell = nullptr;
	int allIn = 1, allOut = 1;

	CharArray::Pointer CellVisible = CharArray::New();
	CellVisible->Resize(CellNum);
	auto cellVisible = CellVisible->RawPointer();
	std::fill(cellVisible, cellVisible + CellNum, 0);

	clock_t time1 = clock();
	auto func = [&](igIndex start, igIndex end) -> void {
		igIndex cellId = 0;
		igIndex vhs[IGAME_CELL_MAX_SIZE] = { 0 };
		igIndex vcnt = 0;
		igIndex allIn = 1, allOut = 1;
		double value = 0;
		igIndex i = 0;
		for (cellId = start; cellId < end; cellId++) {
			vcnt = m_UnstructuredMesh->GetCellPointIds(cellId, vhs);
			allIn = 1;
			allOut = 1;
			for (i = 0; i < vcnt; i++) {
				value = PointClipValue[vhs[i]];
				if (value < 0.0) {
					allOut = 0;
				}
				else if (value > 0.0) {
					allIn = 0;
				}
				else {
					allIn = 0;
					allOut = 0;
				}
			}
			if (allIn) {
				cellVisible[cellId] = 1;
			}
			else if (allOut) {
				cellVisible[cellId] = 2;
			}
		}
	};
	ThreadPool::parallelFor(0, CellNum, func);
	clock_t time2 = clock();
	std::cout << "compute cell vis cost  " << time2 - time1 << '\n';
	if (this->GetIsSlice() == false) {
		auto Result_ExtractPart = iGame::UnstructuredMesh::New();
		auto ExtractCells = CellArray::New();
		auto ExtractTypes = UnsignedIntArray::New();
		ExtractCells->Reserve(m_UnstructuredMesh->GetCells()->GetNumberOfCellIds() * 2 / 3);
		ExtractTypes->Reserve(CellNum * 2 / 3);
		OriginCell.reserve(CellNum * 2 / 3);
		clock_t time_1 = clock();
		igIndex cellId = 0;
		igIndex vhs[IGAME_CELL_MAX_SIZE] = { 0 };
		for (cellId = 0; cellId < CellNum; cellId++) {
			if (cellVisible[cellId] == 1) {
				vcnt = inCells->GetCellIds(cellId, vhs);
				ExtractCells->AddCellIds(vhs, vcnt);
				ExtractTypes->AddValue(inTypes->GetValue(cellId));
				OriginCell.emplace_back(cellId);
			}
		}
		clock_t time_2 = clock();
		//std::cout << "process extract cost" << time_2 - time_1 << '\n';
		OutPoints->Resize(inPointNum);
		std::copy(inPoints->RawPointer(), inPoints->RawPointer() + inPointNum * 3, OutPoints->RawPointer());
		OriginEdge.reserve(inPointNum * 1.2);
		for (int pointId = 0; pointId < inPointNum; pointId++) {
			OriginEdge.emplace_back(CellClip::InterpolateEdge(pointId));
		}
		Result_ExtractPart->SetPoints(OutPoints);
		Result_ExtractPart->SetCells(ExtractCells, ExtractTypes);
		OutConn = ExtractCells;
		OutType = ExtractTypes;
	}
	clock_t time3 = clock();
	std::cout << "compute Result_ExtractPart cost  " << time3 - time2 << '\n';
	igIndex* vhs = nullptr;
	for (CellId = 0; CellId < CellNum; CellId++) {
		if (cellVisible[CellId]) {
			continue;
		}
		cell = m_UnstructuredMesh->GetCell(CellId);
		vhs = cell->m_PointIds->RawPointer();
		vcnt = cell->GetNumberOfPoints();
		for (i = 0; i < vcnt; i++) {
			CellClipValue[i] = PointClipValue[vhs[i]];
		}
		switch (cell->GetCellType())
		{
		case IG_TRIANGLE:
			CellClip::Clip(DynamicCast<Triangle>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_QUAD:
			CellClip::Clip(DynamicCast<Quad>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_POLYGON:
			CellClip::Clip(DynamicCast<Polygon>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_TETRA:
			CellClip::Clip(DynamicCast<Tetra>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_QUADRATIC_TETRA:
			CellClip::Clip(DynamicCast<QuadraticTetra>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_POLYHEDRON:
			CellClip::Clip(DynamicCast<Polyhedron>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		default:
			if (Cell::GetCellDimension(cell->GetCellType()) == 3) {
				CellClip::Clip(DynamicCast<Volume>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, PointClipValue, m_Slice);
			}
			break;
		}
	}
	clock_t time4 = clock();
	std::cout << "compute clip part cost  " << time4 - time3 << '\n';
	this->CopyAttributeSetData(OutPoints->GetNumberOfPoints(), OutConn->GetNumberOfCells(), inData, outData, OriginEdge, OriginCell);



	OutMesh->SetCells(OutConn, OutType);
	OutMesh->SetPoints(OutPoints);
	OutMesh->SetAttributeSet(outData);
	this->SetOutput(0, OutMesh);
	std::vector<igIndex>().swap(OriginCell);
	std::vector<CellClip::InterpolateEdge>().swap(OriginEdge);

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
	std::vector<igIndex> OriginCell;
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
			CellClip::Clip(DynamicCast<Triangle>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_QUAD:
			CellClip::Clip(DynamicCast<Quad>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_POLYGON:
			CellClip::Clip(DynamicCast<Polygon>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_TETRA:
			CellClip::Clip(DynamicCast<Tetra>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_QUADRATIC_TETRA:
			CellClip::Clip(DynamicCast<QuadraticTetra>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case IG_POLYHEDRON:
			CellClip::Clip(DynamicCast<Polyhedron>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		default:
			if (Cell::GetCellDimension(cell->GetCellType()) == 3) {
				CellClip::Clip(DynamicCast<Volume>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, PointClipValue, m_Slice);
			}
			break;
		}
	}

	this->CopyAttributeSetData(OutPoints->GetNumberOfPoints(), OutConn->GetNumberOfCells(), inData, outData, OriginEdge, OriginCell);

	OutMesh->SetCells(OutConn, OutType);
	OutMesh->SetPoints(OutPoints);
	OutMesh->SetAttributeSet(outData);
	this->SetOutput(0, OutMesh);
	std::vector<igIndex>().swap(OriginCell);
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
	std::vector<igIndex> OriginCell;
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
			CellClip::Clip(polyhedron, CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
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
				CellClip::Clip(DynamicCast<Tetra>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
				break;
			default:
				if (Cell::GetCellDimension(cell->GetCellType()) == 3) {
					CellClip::Clip(DynamicCast<Volume>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, PointClipValue, m_Slice);
				}
				break;
			}
		}

	}


	this->CopyAttributeSetData(OutPoints->GetNumberOfPoints(), OutConn->GetNumberOfCells(), inData, outData, OriginEdge, OriginCell);

	OutMesh->SetCells(OutConn, OutType);
	OutMesh->SetPoints(OutPoints);
	OutMesh->SetAttributeSet(outData);
	this->SetOutput(0, OutMesh);
	std::vector<igIndex>().swap(OriginCell);
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
	std::vector<igIndex> OriginCell;
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
	int allIn = 1, allOut = 1;
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
			CellClip::Clip(DynamicCast<Triangle>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		case 4:
			CellClip::Clip(DynamicCast<Quad>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		default:
			CellClip::Clip(DynamicCast<Polygon>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, m_Slice);
			break;
		}
	}

	this->CopyAttributeSetData(OutPoints->GetNumberOfPoints(), OutConn->GetNumberOfCells(), inData, outData, OriginEdge, OriginCell);

	OutMesh->SetCells(OutConn, OutType);
	OutMesh->SetPoints(OutPoints);
	OutMesh->SetAttributeSet(outData);
	this->SetOutput(0, OutMesh);
	std::vector<igIndex>().swap(OriginCell);
	std::vector<CellClip::InterpolateEdge>().swap(OriginEdge);
	return true;
}

void ModelClip::CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum, AttributeSet::Pointer inData, AttributeSet::Pointer outData,
	std::vector<CellClip::InterpolateEdge>OriginEdge, std::vector<igIndex> OriginCell)
{
	igIndex i = 0, j = 0, k = 0;
	int dimension = 0;
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
				inArray->GetElement(OriginCell[j], values);
				outArray->SetElement(j, values);
			}
			outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
		}
		else if (attr.attachmentType == IG_POINT) {
			outArray->Resize(outPointNum);
			dimension = inArray->GetDimension();
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
}
IGAME_NAMESPACE_END