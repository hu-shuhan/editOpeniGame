#ifndef iGameQuadraticPrism_h
#define iGameQuadraticPrism_h

#include "iGameVolume.h"
#include "iGameQuadraticTriangle.h"
#include "iGameQuadraticQuad.h"
#include "./Base/iGameQuadraticVolume.h"

IGAME_NAMESPACE_BEGIN
class QuadraticPrism : public QuadraticVolume {
public:
	I_OBJECT(QuadraticPrism);
	static Pointer New() { return new QuadraticPrism; }

	IGenum GetCellType() const noexcept override { return IG_QUADRATIC_PRISM; }
	int GetCellSize() const noexcept override { return NumberOfPoints; }
	int GetNumberOfEdges() override { return NumberOfEdges; }
	int GetNumberOfFaces() override { return NumberOfFaces; }

	Cell* GetEdge(const int edgeId) override {
		const int* verts = edges[edgeId];
		m_Line->m_PointIds->SetId(0, this->m_PointIds->GetId(verts[0]));
		m_Line->m_PointIds->SetId(1, this->m_PointIds->GetId(verts[1]));
		m_Line->m_PointIds->SetId(2, this->m_PointIds->GetId(verts[2]));
		m_Line->m_Points->SetPoint(0, this->m_Points->GetPoint(verts[0]));
		m_Line->m_Points->SetPoint(1, this->m_Points->GetPoint(verts[1]));
		m_Line->m_Points->SetPoint(2, this->m_Points->GetPoint(verts[2]));
		return m_Line.get();
	}
	Cell* GetFace(const int faceId) override {
		const int* verts = faces[faceId];
		const int* edges = faceEdges[faceId];
		QuadraticFace* face;
		if (faces[faceId][MaxFaceSize] == 8) {
			face = m_Triangle.get();
		}
		else {
			face = m_Quad.get();
		}
		for (int i = 0; i < verts[MaxFaceSize]; ++i) {
			face->m_PointIds->SetId(i, this->m_PointIds->GetId(verts[i]));
			face->m_Points->SetPoint(i, this->m_Points->GetPoint(verts[i]));
		}
		for (int i = 0; i < edges[MaxFaceSize / 2]; ++i) {
			face->EdgeIds->SetId(i, EdgeIds->GetId(edges[i]));
		}
		return face;
	}

	static constexpr int NumberOfPoints = 15;

	static constexpr int NumberOfEdges = 9;

	static constexpr int NumberOfFaces = 5;

	static constexpr int MaxFaceSize = 8;

	static constexpr int MaxValence = 3;

	static constexpr int edges[NumberOfEdges][3] = {
	  { 0, 1 ,6 },
	  { 1, 2 ,7 },
	  { 2, 0 ,8 },
	  { 3, 4 ,9 },
	  { 4, 5 ,10 },
	  { 5, 3 ,11 },
	  { 0, 3 ,12 },
	  { 1, 4 ,13 },
	  { 2, 5 ,14 },
	};

	static constexpr int faces[NumberOfFaces][MaxFaceSize + 1] = {
	  { 0, 1, 2, 6, 7, 8, -1, -1, 6 },
	  { 3, 5, 4, 11, 10, 9, -1, -1, 6 },
	  { 0, 3, 4, 1, 12, 9, 13, 6, 8 },
	  { 1, 4, 5, 2, 13, 10, 14, 7, 8 },
	  { 2, 5, 3, 0, 14, 11, 12, 8, 8 },
	};

	static constexpr int faceEdges[NumberOfFaces][MaxFaceSize / 2 + 1] = {
	  { 0, 1, 2, -1, 3},
	  { 5, 4, 3, -1, 3 },
	  { 6, 3, 7, 0, 4 },
	  { 7, 4, 8, 1, 4 },
	  { 8, 5, 6, 2, 4 },
	};

	static constexpr int edgeToNeighborFaces[NumberOfEdges][2] = {
	  { 0, 2 },
	  { 0, 3 },
	  { 0, 4 },
	  { 1, 2 },
	  { 1, 3 },
	  { 1, 4 },
	  { 2, 4 },
	  { 2, 3 },
	  { 3, 4 },
	};

	static constexpr int faceToNeighborFaces[NumberOfFaces][MaxFaceSize / 2 + 1] = {
	  { 4, 3, 2, -1, 3 },
	  { 2, 3, 4, -1, 3 },
	  { 0, 3, 1, 4 , 4 },
	  { 0, 4, 1, 2 , 4 },
	  { 0, 2, 1, 3 , 4 },
	};

	static constexpr int pointToNeighborEdges[NumberOfPoints][MaxValence + 1] = {
	  { 0, 6, 2, 3 },
	  { 0, 1, 7, 3 },
	  { 1, 2, 8, 3 },
	  { 3, 5, 6, 3 },
	  { 3, 7, 4, 3 },
	  { 4, 8, 5, 3 },
	};

	static constexpr int pointToNeighborFaces[NumberOfPoints][MaxValence + 1] = {
	  { 2, 4, 0, 3 },
	  { 0, 3, 2, 3 },
	  { 0, 4, 3, 3 },
	  { 1, 4, 2, 3 },
	  { 2, 3, 1, 3 },
	  { 3, 4, 1, 3 },
	};

	static constexpr int pointToOneRingPoints[NumberOfPoints][MaxValence + 1] = {
	  { 1, 3, 2, 3 },
	  { 0, 2, 4, 3 },
	  { 1, 0, 5, 3 },
	  { 4, 5, 0, 3 },
	  { 3, 1, 5, 3 },
	  { 4, 2, 3, 3 },
	};

	int GetEdgePointIds(const int edgeId, const igIndex*& pts) override {
		pts = edges[edgeId];
		return 3;
	}
	int GetFacePointIds(const int faceId, const igIndex*& pts) override {
		pts = faces[faceId];
		return faces[faceId][MaxFaceSize];
	}
	int GetFaceEdgeIds(const int faceId, const igIndex*& edgeIds) override {
		edgeIds = faceEdges[faceId];
		return faceEdges[faceId][MaxFaceSize / 2];
	}
	int GetEdgeToNeighborFaces(const int edgeId, const igIndex*& pts) override {
		pts = edgeToNeighborFaces[edgeId];
		return 2;
	}
	int GetFaceToNeighborFaces(const int faceId, const igIndex*& faceIds) override {
		faceIds = faceToNeighborFaces[faceId];
		return faceToNeighborFaces[faceId][MaxFaceSize / 2];
	}
	int GetPointToNeighborEdges(const int pointId, const igIndex*& edgeIds) override {
		edgeIds = pointToNeighborEdges[pointId];
		return pointToNeighborEdges[pointId][MaxValence];
	}
	int GetPointToNeighborFaces(const int pointId, const igIndex*& faceIds) override {
		faceIds = pointToNeighborFaces[pointId];
		return pointToNeighborFaces[pointId][MaxValence];
	}
	int GetPointToOneRingPoints(const int pointId, const igIndex*& pts) override {
		pts = pointToOneRingPoints[pointId];
		return pointToOneRingPoints[pointId][MaxValence];
	}

private:
	QuadraticPrism()
	{
		this->m_Points->SetNumberOfPoints(NumberOfPoints);
		this->m_PointIds->SetNumberOfIds(NumberOfPoints);
		for (int i = 0; i < NumberOfPoints; i++)
		{
			this->m_Points->SetPoint(i, 0.0, 0.0, 0.0);
			this->m_PointIds->SetId(i, 0);
		}

		m_Line = QuadraticLine::New();
		m_Triangle = QuadraticTriangle::New();
		m_Quad = QuadraticQuad::New();
	}
	~QuadraticPrism() override = default;

	QuadraticLine::Pointer m_Line;
	QuadraticTriangle::Pointer m_Triangle;
	QuadraticQuad::Pointer m_Quad;
};


IGAME_NAMESPACE_END
#endif