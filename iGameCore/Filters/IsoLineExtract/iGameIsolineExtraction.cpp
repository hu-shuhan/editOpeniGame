#include "iGameIsolineExtraction.h"

IGAME_NAMESPACE_BEGIN
namespace IsolineExtraction
{
	// 线性插值函数
	Point_3D VertexInterp(Point_3D p1, Point_3D p2, float iso_val) {
		Point_3D result;

		if (std::abs(iso_val - p1.value) < 1e-6) { return p1; }

		if (std::abs(iso_val - p2.value) < 1e-6) { return p2; }

		if (std::abs(p1.value - p2.value) < 1e-6) { return p1; }

		float alpha = (iso_val - p1.value) / (p2.value - p1.value);

		result.x = p1.x + alpha * (p2.x - p1.x);
		result.y = p1.y + alpha * (p2.y - p1.y);
		result.z = p1.z + alpha * (p2.z - p1.z);
		result.value = iso_val;
		return result;
	}


} // namespace IsolineExtraction
bool iGameIsolineExtraction::Execute() {

	// 输入数据从 inInfo 中获取，输出数据放到 outInfo
	if (!this->GetNumberOfInputs()) { return false; }
	m_Mesh = this->GetInput(0);
	switch (m_Mesh->GetDataObjectType())
	{
	case IG_SURFACE_MESH:
		m_SurfaceMesh = DynamicCast<SurfaceMesh>(m_Mesh);
		break;
	case IG_UNSTRUCTURED_MESH:
		m_UnstructuredMesh = DynamicCast<UnstructuredMesh>(m_Mesh);
		m_SurfaceMesh = m_UnstructuredMesh->TransferToSurfaceMesh();
		break;
	default:
		return false;
	}
	if (!m_SurfaceMesh) { return false; }




	// TODO::
	// m_Value就是等值线的值，需要被导入
	auto FaceNum = m_SurfaceMesh->GetNumberOfFaces();
	auto PointNum = m_SurfaceMesh->GetNumberOfPoints();
	auto Points = m_SurfaceMesh->GetPoints();
	auto PointData = m_SurfaceMesh->GetAttributeSet()->GetAllPointAttributes();
	if (!PointData->GetNumberOfElements()) { return false; }
	this->SetAttribute(PointData->GetElement(0));

	// TODO::
	// 将三角形面片导入vector<Triangle_3D> tris
	// 是其它类型的面片就先拆一下

	std::vector<IsolineExtraction::Point_3D> lines;
	std::vector<IsolineExtraction::Triangle_3D> tris;
	igIndex i = 0, j = 0, k = 0;
	igIndex vhs[IGAME_CELL_MAX_SIZE] = { 0 };
	igIndex vcnt = 0;
	double values[IGAME_CELL_MAX_SIZE];
	double value = 0.0;
	int dimensionSize = m_Scalar->GetDimension();
	Point p = { 0,0,0 };
	for (i = 0; i < FaceNum; i++) {

		vcnt = m_SurfaceMesh->GetFacePointIds(i, vhs);
		IsolineExtraction::Triangle_3D tri;
		p = Points->GetPoint(vhs[0]);
		tri.p[0].x = p[0];
		tri.p[0].y = p[1];
		tri.p[0].z = p[2];
		tri.p[0].value = m_Scalar->GetElementValue(vhs[0], m_Dimension);
		for (j = 2; j < vcnt; j++) {
			p = Points->GetPoint(vhs[j - 1]);
			tri.p[1].x = p[0];
			tri.p[1].y = p[1];
			tri.p[1].z = p[2];
			tri.p[1].value = m_Scalar->GetElementValue(vhs[j - 1], m_Dimension);
			p = Points->GetPoint(vhs[j]);
			tri.p[2].x = p[0];
			tri.p[2].y = p[1];
			tri.p[2].z = p[2];
			tri.p[2].value = m_Scalar->GetElementValue(vhs[j], m_Dimension);
			tris.emplace_back(tri);
		}
	}


	auto ResultMesh = SurfaceMesh::New();
	auto ResultPoints = Points::New();
	auto ResultLines = CellArray::New();
	ResultMesh->SetPoints(ResultPoints);
	IsolineExtraction::Point_3D P1, P2;
	for (auto tri : tris) {
		// 如果是001（1表示点的值大于m_Value，0表示小于）
		if (tri.p[0].value < m_Value && tri.p[1].value < m_Value &&
			tri.p[2].value > m_Value) {
			P1 = VertexInterp(tri.p[0], tri.p[2], m_Value);
			P2 = VertexInterp(tri.p[1], tri.p[2], m_Value);
		}
		// 如果是010（1表示点的值大于m_Value，0表示小于）
		if (tri.p[0].value < m_Value && tri.p[1].value > m_Value &&
			tri.p[2].value < m_Value) {
			P1 = VertexInterp(tri.p[1], tri.p[0], m_Value);
			P2 = VertexInterp(tri.p[1], tri.p[2], m_Value);
		}
		// 如果是011（1表示点的值大于m_Value，0表示小于）
		if (tri.p[0].value < m_Value && tri.p[1].value > m_Value &&
			tri.p[2].value > m_Value) {
			P1 = VertexInterp(tri.p[0], tri.p[1], m_Value);
			P2 = VertexInterp(tri.p[0], tri.p[2], m_Value);
		}
		// 如果是100（1表示点的值大于m_Value，0表示小于）
		if (tri.p[0].value > m_Value && tri.p[1].value < m_Value &&
			tri.p[2].value < m_Value) {
			P1 = VertexInterp(tri.p[0], tri.p[1], m_Value);
			P2 = VertexInterp(tri.p[0], tri.p[2], m_Value);
		}
		// 如果是101（1表示点的值大于m_Value，0表示小于）
		if (tri.p[0].value > m_Value && tri.p[1].value < m_Value &&
			tri.p[2].value > m_Value) {
			P1 = VertexInterp(tri.p[1], tri.p[0], m_Value);
			P2 = VertexInterp(tri.p[1], tri.p[2], m_Value);
		}
		// 如果是110（1表示点的值大于m_Value，0表示小于）
		if (tri.p[0].value > m_Value && tri.p[1].value < m_Value &&
			tri.p[2].value > m_Value) {
			P1 = VertexInterp(tri.p[2], tri.p[1], m_Value);
			P2 = VertexInterp(tri.p[2], tri.p[0], m_Value);
		}
		ResultPoints->AddPoint(P1.x, P1.y, P1.z);
		ResultPoints->AddPoint(P2.x, P2.y, P2.z);
	}
	m_Outputs->SetElement(0, ResultMesh);
	return true;
}
IGAME_NAMESPACE_END