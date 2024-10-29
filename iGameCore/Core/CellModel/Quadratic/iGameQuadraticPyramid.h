#ifndef iGameQuadraticPyramid_h
#define iGameQuadraticPyramid_h

#include "Base/iGameQuadraticVolume.h"
#include "iGameQuadraticTriangle.h"
#include "iGameQuadraticQuad.h"

IGAME_NAMESPACE_BEGIN
class QuadraticPyramid : public QuadraticVolume {
public:
	I_OBJECT(QuadraticPyramid);
	static Pointer New() { return new QuadraticPyramid; }

	IGenum GetCellType() const noexcept override { return IG_QUADRATIC_PYRAMID; }
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
		if (faces[faceId][MaxFaceSize] == 6) {
			face = m_Triangle.get();
		}
		else {
			face = m_Quad.get();
		}
		for (int i = 0; i < verts[MaxFaceSize]; ++i) {
			face->m_PointIds->SetId(i,  this->m_PointIds->GetId(verts[i]));
			face->m_Points->SetPoint(i, this->m_Points->GetPoint(verts[i]));
		}
		for (int i = 0; i < edges[MaxFaceSize / 2]; ++i) {
			face->EdgeIds->SetId(i, EdgeIds->GetId(edges[i]));
		}
		return face;
	}

	static constexpr int NumberOfPoints = 13;

	static constexpr int NumberOfEdges = 8;

	static constexpr int NumberOfFaces = 5;

	static constexpr int MaxFaceSize = 8;

	static constexpr int MaxValence = 4;

	static constexpr int edges[NumberOfEdges][3] = {
	  { 0, 1, 5 },
	  { 1, 2, 6 },
	  { 2, 3, 7 },
	  { 3, 0, 8 },
	  { 0, 4, 9 },
	  { 1, 4, 10 },
	  { 2, 4, 11 },
	  { 3, 4, 12 },
	};

	static constexpr int faces[NumberOfFaces][MaxFaceSize + 1] = {
	  { 0, 3, 2, 1, 8, 7, 6, 5, 8 },
	  { 0, 1, 4, 5, 10, 9, -1, -1, 6},
	  { 1, 2, 4, 6, 11, 10, -1, -1, 6},
	  { 2, 3, 4, 7, 12, 11, -1, -1, 6},
	  { 3, 0, 4, 8, 9, 12, -1, -1, 6},
	};

	static constexpr int faceEdges[NumberOfFaces][MaxFaceSize / 2 + 1] = {
	  { 3, 2, 1, 0, 4},
	  { 0, 5, 4, -1, 3},
	  { 1, 6, 5, -1, 3},
	  { 2, 7, 6, -1, 3},
	  { 3, 4, 7, -1, 3},
	};

	static constexpr int edgeToNeighborFaces[NumberOfEdges][2] = {
	  { 0, 1 },
	  { 0, 2 },
	  { 0, 3 },
	  { 0, 4 },
	  { 1, 4 },
	  { 1, 2 },
	  { 2, 3 },
	  { 3, 4 },
	};

	static constexpr int faceToNeighborFaces[NumberOfFaces][MaxFaceSize / 2 + 1] = {
	  { 4, 3, 2, 1,  4 },
	  { 0, 2, 4, -1, 3 },
	  { 0, 3, 1, -1, 3 },
	  { 0, 4, 2, -1, 3 },
	  { 0, 1, 3, -1, 3 },
	};

	static constexpr int pointToNeighborEdges[NumberOfPoints][MaxValence + 1] = {
	  { 0, 4, 3, -1, 3 },
	  { 0, 1, 5, -1, 3 },
	  { 1, 2, 6, -1, 3 },
	  { 2, 3, 7, -1, 3 },
	  { 4, 5, 6, 7 , 4 },
	};

	static constexpr int pointToNeighborFaces[NumberOfPoints][MaxValence + 1] = {
	  { 1, 4, 0, -1, 3 },
	  { 0, 2, 1, -1, 3 },
	  { 0, 3, 2, -1, 3 },
	  { 0, 4, 3, -1, 3 },
	  { 1, 2, 3, 4 , 4 },
	};

	static constexpr int pointToOneRingPoints[NumberOfPoints][MaxValence + 1] = {
	  { 1, 4, 3, -1, 3 },
	  { 0, 2, 4, -1, 3 },
	  { 1, 3, 4, -1, 3 },
	  { 2, 0, 4, -1, 3 },
	  { 0, 1, 2, 3 , 4 },
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
	QuadraticPyramid()
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
	~QuadraticPyramid() override = default;

	QuadraticLine::Pointer m_Line;
	QuadraticTriangle::Pointer m_Triangle;
	QuadraticQuad::Pointer m_Quad;
};


IGAME_NAMESPACE_END
#endif