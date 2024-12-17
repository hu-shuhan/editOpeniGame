#ifndef MyTestFilter_h
#define MyTestFilter_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include <random>

IGAME_NAMESPACE_BEGIN
class MyTestFilter : public Filter {
public:
	I_OBJECT(MyTestFilter);
	static Pointer New() { return new MyTestFilter; }

	void SetMesh(DataObject::Pointer dataObject) {
		this->m_Mesh = DynamicCast<SurfaceMesh>(dataObject);
	}
	bool Execute() override {
		this->m_Mesh = DynamicCast<SurfaceMesh>(GetInput(0));
		if (m_Mesh == nullptr) { return false; }
		m_Mesh->RequestEditStatus();
		for (int faceid = 0; faceid < m_Mesh->GetNumberOfFaces(); faceid++) {
			auto face = m_Mesh->GetFace(faceid);

			for (int i = 0; i < 3; i++) {
				auto p = face->GetPoint(i);
				std::cout << face->GetPointId(i) << '\n';
				std::cout << face->GetNumberOfPoints() << '\n';
				std::cout << p[0] << " " << p[1] << " " << p[2] << '\n';
			}
		}

		return true;
	}

protected:
	MyTestFilter()
	{
		SetNumberOfInputs(1);
		SetNumberOfOutputs(1);
	}
	~MyTestFilter() override = default;

	SurfaceMesh::Pointer m_Mesh{};


};
IGAME_NAMESPACE_END
#endif