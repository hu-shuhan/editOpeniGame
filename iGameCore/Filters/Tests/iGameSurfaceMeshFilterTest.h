#ifndef SurfaceMeshFilterTest_h
#define SurfaceMeshFilterTest_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include <random>

IGAME_NAMESPACE_BEGIN
class SurfaceMeshFilterTest : public Filter {
public:
	I_OBJECT(SurfaceMeshFilterTest);
	static Pointer New() { return new SurfaceMeshFilterTest; }

	bool Execute() override {
        m_Mesh = DynamicCast<SurfaceMesh>(GetInput(0));
        if (m_Mesh == nullptr) { return false; }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        //AddFaceTest();
        //DeletePointTest();
        //DeleteEdgeTest();
        DeleteFaceTest();

        SetOutput(0, m_Mesh);
        return true;
    }

protected:
	SurfaceMeshFilterTest()
	{
		SetNumberOfInputs(1);
		SetNumberOfOutputs(1);
	}
	~SurfaceMeshFilterTest() override = default;

	SurfaceMesh::Pointer m_Mesh{};

	void AddFaceTest() {
		int id = m_Mesh->AddPoint(Point(1, 1, 0));
		igIndex face[3]{ 1,id,2 };
		m_Mesh->AddFace(face, 3);
	}

	void DeleteFaceTest() {
		std::cout << "-----------------------------------------" << std::endl;
		std::cout << "before" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;

		m_Mesh->RequestEditStatus();
		for (int i = 0; i < m_Mesh->GetNumberOfFaces()/2; i++) {
			m_Mesh->DeleteFace(i);
		}
		m_Mesh->GarbageCollection();

		std::cout << "after" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;
	}

	void DeletePointTest() {
		std::cout << "-----------------------------------------" << std::endl;
		std::cout << "before" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;

		m_Mesh->RequestEditStatus();
		for (int i = 0; i < m_Mesh->GetNumberOfPoints()/2; i++) {
			m_Mesh->DeletePoint(i);
		}
		m_Mesh->GarbageCollection();

		std::cout << "after" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;
	}

	void DeleteEdgeTest() {
		std::cout << "-----------------------------------------" << std::endl;
		std::cout << "before" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;

		m_Mesh->RequestEditStatus();
		for (int i = 0; i < m_Mesh->GetNumberOfEdges() / 2; i++) {
			m_Mesh->DeleteEdge(i);
		}
		m_Mesh->GarbageCollection();

		std::cout << "after" << std::endl;
		std::cout << "point number: " << m_Mesh->GetNumberOfPoints() << std::endl;
		std::cout << "edge number: " << m_Mesh->GetNumberOfEdges() << std::endl;
		std::cout << "face number: " << m_Mesh->GetNumberOfFaces() << std::endl;
	}
};
IGAME_NAMESPACE_END
#endif