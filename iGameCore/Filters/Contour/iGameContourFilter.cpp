#include "iGameContourFilter.h"

#include "iGameThreadPool.h"
IGAME_NAMESPACE_BEGIN
ContourFilter::ContourFilter()
{

	this->SetNumberOfInputs(1);
	this->SetNumberOfOutputs(1);

}

ContourFilter::~ContourFilter()
{

}
bool ContourFilter::SetPlane(double o[3], double n[3])
{
	if (m_Inputs->GetNumberOfElements() == 0) {
		return false;
	}
	auto input = m_Inputs->GetElement(0);
	if (!input) {
		return false;
	}
	auto PointSet=DynamicCast<iGame::PointSet>(input);
	if(PointSet==nullptr)return false;
	auto Points= PointSet->GetPoints();
	auto PointNum=PointSet->GetNumberOfPoints();
	if(PointNum==0)return false;
	DoubleArray::Pointer ScalarData=DoubleArray::New();
	ScalarData->Resize(PointNum);
	double* scalarData= ScalarData->RawPointer();
	Point p{0,0,0};
	for (int i = 0; i < PointNum; i++)
	{
		p=Points->GetPoint(i);
		scalarData[i]= n[0] * (p[0] - o[0]) + n[1] * (p[1] - o[1]) + n[2] * (p[2] - o[2]);
	}
	this->SetIsoScalarData(ScalarData,0.0,0);
	return true;
}
bool ContourFilter::Execute()
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
		return false;
	}
	return true;
}
void ContourFilter::SetIsoScalarData(ArrayObject::Pointer array, double value, int dimension) {
	this->m_SelectedScalar = array;
	this->m_SeletectDimension = dimension;
	this->m_ContourValue = value;
}

bool ContourFilter::ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer input)
{
	if (!input)return false;
	AttributeSet::Pointer inData = input->GetAttributeSet();
	AttributeSet::Pointer outData = AttributeSet::New();

	CellArray::Pointer OutConn = CellArray::New();
	UnsignedIntArray::Pointer OutType = UnsignedIntArray::New();
	Points::Pointer OutPoints = Points::New();
	UnstructuredMesh::Pointer OutMesh = UnstructuredMesh::New();
	std::vector<CellContour::InterpolateEdge>OriginEdge;
	std::vector<igIndex> OriginCell;
	auto inPoints = input->GetPoints();
	auto inPointNum = input->GetNumberOfPoints();
	auto inCells = input->GetCells();
	auto inTypes = input->GetCellTypes();
	igIndex inCellNum = input->GetNumberOfCells();

	DoubleArray::Pointer PointContourArray = DoubleArray::New();
	CharArray::Pointer CellVisible = CharArray::New();
	ComputePointValueAndCellVisible(inPoints, inCells, PointContourArray, CellVisible);
	auto PointContourValue = PointContourArray->RawPointer();
	auto cellVisible = CellVisible->RawPointer();
	igIndex vcnt = 0;
	igIndex* vhs = nullptr;
	igIndex CellId = 0;
	igIndex i = 0;
	Cell::Pointer cell = nullptr;
	double  CellContourValue[IGAME_CELL_MAX_SIZE] = { 0 };
	for (CellId = 0; CellId < inCellNum; CellId++) {
		//if (cellVisible[CellId]) {
		//	continue;
		//}
		cell = input->GetCell(CellId);
		vhs = cell->m_PointIds->RawPointer();
		vcnt = cell->GetNumberOfPoints();
		for (i = 0; i < vcnt; i++) {
			 CellContourValue[i] = PointContourValue[vhs[i]];
		}
		switch (cell->GetCellType())
		{
		case IG_TRIANGLE:
			 CellContour::Contour(DynamicCast<Triangle>(cell),  CellContourValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell );
			break;
		case IG_QUAD:
			 CellContour::Contour(DynamicCast<Quad>(cell),  CellContourValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell );
			break;
		case IG_POLYGON:
			 CellContour::Contour(DynamicCast<Polygon>(cell),  CellContourValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell );
			break;
		case IG_TETRA:
			 CellContour::Contour(DynamicCast<Tetra>(cell),  CellContourValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell );
			break;
		case IG_PRISM:
			CellContour::Contour(DynamicCast<Prism>(cell), CellContourValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell);
			break;
		case IG_PYRAMID:
			CellContour::Contour(DynamicCast<Pyramid>(cell), CellContourValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell);
			break;
		case IG_HEXAHEDRON:
			CellContour::Contour(DynamicCast<Hexahedron>(cell), CellContourValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell);
			break;
		case IG_QUADRATIC_TETRA:
			 CellContour::Contour(DynamicCast<QuadraticTetra>(cell),  CellContourValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell );
			break;
		case IG_POLYHEDRON:
			 CellContour::Contour(DynamicCast<Polyhedron>(cell),  CellContourValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell );
			break;
		default:
			if (Cell::GetCellDimension(cell->GetCellType()) == 3) {
				 CellContour::Contour(DynamicCast<Volume>(cell),  CellContourValue, OutPoints, OutConn, OutType, nullptr, nullptr, CellId, OriginEdge, OriginCell, PointContourValue );
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
	std::vector< CellContour::InterpolateEdge>().swap(OriginEdge);
	
	return true;



}
bool ContourFilter::ExecuteWithVolumeMesh(VolumeMesh::Pointer vm)
{
	auto um = UnstructuredMesh::New();
	um->GenerateFromVolumeMesh(vm);
	return this->ExecuteWithUnstructuredMesh(um);
}
bool ContourFilter::ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm)
{
	auto um = UnstructuredMesh::New();
	um->GenerateFromSurfaceMesh(sm);
	return this->ExecuteWithUnstructuredMesh(um);
	return true;
}
bool ContourFilter::ExecuteWithVolumeMeshWithPolyhedronType(VolumeMesh::Pointer vm)
{
	auto um = UnstructuredMesh::New();
	um->GenerateFromVolumeMesh(vm);
	return this->ExecuteWithUnstructuredMesh(um);
	return true;
}
void ContourFilter::ComputePointValueAndCellVisible(Points::Pointer inPoints, CellArray::Pointer inCells, DoubleArray::Pointer PointContourArray, CharArray::Pointer CellVisible)
{
	igIndex PointId = 0;
	igIndex inPointNum = inPoints->GetNumberOfPoints();
	PointContourArray->Resize(inPointNum);
	double* PointContourValue = PointContourArray->RawPointer();
	for (PointId = 0; PointId < inPointNum; PointId++) {
		PointContourValue[PointId] = this->m_SelectedScalar->GetElementValue(PointId, m_SeletectDimension) - m_ContourValue;
	}
	igIndex CellId = 0;
	IGsize CellNum = inCells->GetNumberOfCells();
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
			vcnt = inCells->GetCellIds(cellId, vhs);
			allIn = 1;
			allOut = 1;
			for (i = 0; i < vcnt; i++) {
				value = PointContourValue[vhs[i]];
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
	PointContourValue = nullptr;
}
void ContourFilter::CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum, AttributeSet::Pointer inData, AttributeSet::Pointer outData,
	std::vector<CellContour::InterpolateEdge>OriginEdge, std::vector<igIndex> OriginCell)
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