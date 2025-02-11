#include "iGameSTLWriter.h"

/**
 * https://en.wikipedia.org/wiki/STL_(file_format)
 */

IGAME_NAMESPACE_BEGIN

bool STLWriter::GenerateBuffers()
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

	m_Buffers.resize(1, nullptr);
	if (m_Binary) {
		if (!WriteBinaryToBuffer(m_Buffers[0])) {
			return false;
		}
	}
	else {
		if (!WriteAsciiToBuffer(m_Buffers[0])) {
			return false;
		}
	}
	return true;
}

bool STLWriter::WriteBinaryToBuffer(CharArray::Pointer& file)
{
	if (file == nullptr) {
		file = CharArray::New();
	}
	char buffer[256];
	memset(buffer, 0, sizeof(buffer));

	auto AddChars = [&](const char* c, int size) -> void {
		for(int i = 0; i < size; ++i)
			file->AddValue(*c++);
		};

	auto AddVector3f = [&](const Vector3f& v) -> void {
		for (int i = 0; i < 3; ++i) {
			AddChars((const char*)&v[i], 4);
		}
		};

	snprintf(buffer, 256, "%s", m_SurfaceMesh->GetName().c_str());
	AddChars(buffer, 80);

	uint16_t trash = 0;
	uint32_t fnb = m_SurfaceMesh->GetNumberOfFaces();
	AddChars((const char*)&fnb, 4);

	for (int i = 0; i < fnb; ++i) {
		auto face = m_SurfaceMesh->GetFace(i);
		auto normal = face->GetNormal();
		AddVector3f(normal);
		// must be a triangle
		for (int j = 0; j < 3; ++j) {
			auto point = face->GetPoint(j);
			AddVector3f(point);
		}
		AddChars((const char*)&trash, 2);
	}

	return true;
}

bool STLWriter::WriteAsciiToBuffer(CharArray::Pointer& file)
{
	if (file == nullptr) {
		file = CharArray::New();
	}
	char buffer[256];
	memset(buffer, 0, sizeof(buffer));

	auto AddChars = [&](const char* c, int size) -> void {
		for (int i = 0; i < size; ++i)
			file->AddValue(*c++);
		};

	auto AddString = [&](const std::string& s) -> void {
		for (char c : s)
			file->AddValue(c);
		};

	static const std::string key_start = "solid ";
	static const std::string key1 = "  facet normal ";
	static const std::string key2 = "    outer loop\n";
	static const std::string key3 = "      vertex ";
	static const std::string key4 = "    endloop\n";
	static const std::string key5 = "  endfacet\n";
	static const std::string key_end = "endsolid igame";
	

	AddString(key_start);
	AddString(m_SurfaceMesh->GetName());
	file->AddValue('\n');
	IGsize fnb = m_SurfaceMesh->GetNumberOfFaces();
	for (int i = 0; i < fnb; ++i) {


		auto face = m_SurfaceMesh->GetFace(i);
		auto normal = face->GetNormal();
		int length;

		// add normal
		AddString(key1);
		length = snprintf(buffer, 256, " %.6e %.6e %.6e\n", normal[0], normal[1], normal[2]);
		AddChars(buffer, length);

		AddString(key2);

		// must be a triangle
		for (int j = 0; j < 3; ++j) {
			auto point = face->GetPoint(j);
			AddString(key3);
			length = snprintf(buffer, 256, " %.6e %.6e %.6e\n", point[0], point[1], point[2]);
			AddChars(buffer, length);
		}

		AddString(key4);
		AddString(key5);
	}
	AddString(key_end);


	return true;
}

IGAME_NAMESPACE_END