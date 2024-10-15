#ifndef iGamePolyhedron_h
#define iGamePolyhedron_h

#include "iGameVolume.h"
#include "iGamePolygon.h"

IGAME_NAMESPACE_BEGIN
class Polyhedron : public Cell {
public:
	I_OBJECT(Polyhedron);
	static Pointer New() { return new Polyhedron; }

	IGenum GetCellType() const noexcept override { return IG_POLYHEDRON; }
	int GetCellSize() const noexcept override { return Points->GetNumberOfPoints(); }
	int GetNumberOfEdges() override { return 0; }
	int GetNumberOfFaces() override { return m_FaceOffset->GetNumberOfIds() - 1; }

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
			m_Polygon->PointIds->AddId(PointIds->GetId(i));
			m_Polygon->Points->AddPoint(Points->GetPoint(i));
		}
		return m_Polygon.get();
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