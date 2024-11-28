#ifndef iGamePolyhedron_h
#define iGamePolyhedron_h

#include "iGameVolume.h"
#include "iGamePolygon.h"

IGAME_NAMESPACE_BEGIN
class Polyhedron : public Volume {
public:
	I_OBJECT(Polyhedron);
	static Pointer New() { return new Polyhedron; }

	IGenum GetCellType() const noexcept override { return IG_POLYHEDRON; }
	int GetCellSize() const noexcept override { return m_Points->GetNumberOfPoints(); }
	int GetNumberOfEdges() override { return 0; }
	int GetNumberOfFaces() override { return m_FaceOffset->GetNumberOfIds() - 1; }

	int GetEdgePointIds(const int edgeId, const igIndex*& ptIds)override { return 0; };
	int GetFacePointIds(const int faceId, const igIndex*& ptIds) override { return 0; };
	int GetFaceEdgeIds(const int faceId, const igIndex*& edgeIds) override { return 0; };
	int GetPointToOneRingPoints(const int ptId, const igIndex*& ptIds) override { return 0; };
	int GetPointToNeighborEdges(const int ptId, const igIndex*& edgeIds)override { return 0; };
	int GetPointToNeighborFaces(const int ptId, const igIndex*& faceIds)override { return 0; };
	int GetEdgeToNeighborFaces(const int edgeId, const igIndex*& faceIds)override { return 0; };
	int GetFaceToNeighborFaces(const int faceId, const igIndex*& faceIds)override { return 0; };
	std::vector<iGame::Cell::Pointer> clipCelltoTetra() override { return std::vector<iGame::Cell::Pointer>(); };

	Cell* GetEdge(const int edgeId) override {
		return nullptr;
	}
	Cell* GetFace(const int faceId) override {
		if (faceId >= GetNumberOfFaces())return nullptr;
		m_Polygon->Reset();
		igIndex st = m_FaceOffset->GetId(faceId);
		igIndex ed = m_FaceOffset->GetId(faceId + 1);
		//std::cout << st << ' ' << ed << '\n';
		for (int i = st; i < ed; ++i) {
			m_Polygon->m_PointIds->AddId(m_PointIds->GetId(i));
			m_Polygon->m_Points->AddPoint(m_Points->GetPoint(i));
		}
		return m_Polygon.get();
	}


	virtual void Reset() {
		this->Cell::Reset();
		this->m_FaceOffset->Reset();
	}

	IdArray::Pointer m_FaceOffset;

protected:
	Polyhedron() {
		m_Polygon = Polygon::New();
		m_FaceOffset = IdArray::New();
	}
	~Polyhedron() override = default;
	Polygon::Pointer m_Polygon;

};
IGAME_NAMESPACE_END
#endif