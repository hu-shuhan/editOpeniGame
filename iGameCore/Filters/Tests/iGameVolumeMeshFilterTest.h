#ifndef VolumeMeshFilterTest_h
#define VolumeMeshFilterTest_h

#include "iGameFilter.h"
#include "iGameVolumeMesh.h"
#include <random>

IGAME_NAMESPACE_BEGIN
class VolumeMeshFilterTest : public Filter {
public:
	I_OBJECT(VolumeMeshFilterTest);
	static Pointer New() { return new VolumeMeshFilterTest; }

	bool Execute() override {
        m_Mesh = DynamicCast<VolumeMesh>(GetInput(0));
        if (m_Mesh == nullptr) { return false; }

        //DeletePointTest();
        //DeleteEdgeTest();
        //DeleteFaceTest();
        DeleteVolumeTest();

        SetOutput(0, m_Mesh);
        return true;
    }

protected:
	VolumeMeshFilterTest()
	{
		SetNumberOfInputs(1);
		SetNumberOfOutputs(1);
	}
	~VolumeMeshFilterTest() override = default;

	VolumeMesh::Pointer m_Mesh{};

	void AddFaceTest() {
		int id = m_Mesh->AddPoint(Point(1, 1, 0));
		igIndex face[3]{ 1,id,2 };
		m_Mesh->AddFace(face, 3);
	}

	void DeleteVolumeTest() {
		std::cout << "-----------------------------------------" << std::endl;
		std::cout << "before" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;
		std::cout << "cell number: " << m_Mesh->GetNumberOfVolumes() << std::endl;

		m_Mesh->RequestEditStatus();
		for (int i = 0; i < m_Mesh->GetNumberOfVolumes() / 2; i++) {
			m_Mesh->DeleteVolume(i);
		}
		m_Mesh->GarbageCollection();

		std::cout << "after" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;
		std::cout << "cell number: " << m_Mesh->GetNumberOfVolumes() << std::endl;
	}

	void DeleteFaceTest() {
		std::cout << "-----------------------------------------" << std::endl;
		std::cout << "before" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;
		std::cout << "cell number: " << m_Mesh->GetNumberOfVolumes() << std::endl;

		m_Mesh->RequestEditStatus();
		for (int i = 0; i < m_Mesh->GetNumberOfFaces()/2; i++) {
			m_Mesh->DeleteFace(i);
		}
		m_Mesh->GarbageCollection();

		std::cout << "after" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;
		std::cout << "cell number: " << m_Mesh->GetNumberOfVolumes() << std::endl;
	}

	void DeletePointTest() {
		std::cout << "-----------------------------------------" << std::endl;
		std::cout << "before" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;
		std::cout << "cell number: " << m_Mesh->GetNumberOfVolumes() << std::endl;

		m_Mesh->RequestEditStatus();
		for (int i = 0; i < m_Mesh->GetNumberOfPoints()/2; i++) {
			m_Mesh->DeletePoint(i);
		}
		m_Mesh->GarbageCollection();

		std::cout << "after" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;
		std::cout << "cell number: " << m_Mesh->GetNumberOfVolumes() << std::endl;
	}

	void DeleteEdgeTest() {
		std::cout << "-----------------------------------------" << std::endl;
		std::cout << "before" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;
		std::cout << "cell number: " << m_Mesh->GetNumberOfVolumes() << std::endl;

		m_Mesh->RequestEditStatus();
		for (int i = 0; i < m_Mesh->GetNumberOfEdges() / 2; i++) {
			m_Mesh->DeleteEdge(i);
		}
		m_Mesh->GarbageCollection();

		std::cout << "after" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;
		std::cout << "cell number: " << m_Mesh->GetNumberOfVolumes() << std::endl;
	}
};
IGAME_NAMESPACE_END
#endif