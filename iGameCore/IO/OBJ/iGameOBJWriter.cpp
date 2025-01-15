#include <OBJ/iGameOBJWriter.h>

IGAME_NAMESPACE_BEGIN



bool OBJWriter::GenerateBuffers()
{
	switch (m_DataObject->GetDataObjectType())
	{
	case IG_SURFACE_MESH:
		m_SurfaceMesh = DynamicCast<SurfaceMesh>(m_DataObject);
		break;
	case IG_UNSTRUCTURED_MESH:
		m_SurfaceMesh = DynamicCast<UnstructuredMesh>(m_DataObject)->TransferToSurfaceMesh();
		break;
	default:
		return false;
	}
	if (!m_SurfaceMesh) {
		return false;
	}
	m_Buffers.resize(2, nullptr);
	WritePointsToBuffer(m_Buffers[0]);
	WriteFacesToBuffer(m_Buffers[1]);
	return true;
}
const void OBJWriter::WritePointsToBuffer(CharArray::Pointer& buffer)
{
	if (buffer == nullptr) {
		buffer = CharArray::New();
	}
	auto Points = m_SurfaceMesh->GetPoints();
	int VertexNum = Points->GetNumberOfPoints();
	std::string data;
	Point p;
	for (int i = 0; i < VertexNum; i++) {
		p = Points->GetPoint(i);
		buffer->AddValue('v');
		for (int j = 0; j < 3; j++) {
			buffer->AddValue(' ');
			data = std::to_string(p[j]);
			AddStringToBuffer(data, buffer);
		}
		buffer->AddValue('\n');
	}
}
const void OBJWriter::WriteFacesToBuffer(CharArray::Pointer& buffer)
{
	if (buffer == nullptr) {
		buffer = CharArray::New();
	}
	auto Faces = m_SurfaceMesh->GetFaces();
	int FaceNum = m_SurfaceMesh->GetNumberOfFaces();
	igIndex vhs[IGAME_CELL_MAX_SIZE];
	for (int i = 0; i < FaceNum; i++) {
		auto fn = Faces->GetCellIds(i, vhs);
		buffer->AddValue('f');
		for (int j = 0; j < fn; j++) {
			buffer->AddValue(' ');
			std::string data = std::to_string(vhs[j] + 1);
			AddStringToBuffer(data, buffer);
		}
		buffer->AddValue('\n');
	}
}


IGAME_NAMESPACE_END