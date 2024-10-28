#ifndef iGameQuadraticTetra_h
#define iGameQuadraticTetra_h

#include "Base/iGameQuadraticVolume.h"
#include "iGameQuadraticTriangle.h"
IGAME_NAMESPACE_BEGIN
class QuadraticTetra : public QuadraticVolume {
public:
	I_OBJECT(QuadraticTetra);
	static Pointer New() { return new QuadraticTetra; }

	IGenum GetCellType() const noexcept override { return IG_QUADRATIC_TETRA; }
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
		for (int i = 0; i < 6; ++i) {
			m_Triangle->m_PointIds->SetId(i,  m_PointIds->GetId(verts[i]));
			m_Triangle->m_Points->SetPoint(i, m_Points->GetPoint(verts[i]));
		}
		for (int i = 0; i < 3; ++i) {
			m_Triangle->EdgeIds->SetId(i, EdgeIds->GetId(edges[i]));
		}
		return m_Triangle.get();
	}

	static constexpr int NumberOfPoints = 10;

	static constexpr int NumberOfEdges = 6;

	static constexpr int NumberOfFaces = 4;

	static constexpr int MaxFaceSize = 6;

	static constexpr int MaxValence = 3;

	static constexpr int edges[NumberOfEdges][3] = {
	  { 0, 1, 4},
	  { 1, 2, 5},
	  { 2, 0, 6},
	  { 0, 3, 7},
	  { 1, 3, 8},
	  { 2, 3, 9},
	};

	static constexpr int faces[NumberOfFaces][MaxFaceSize + 1] = {
	  { 0, 1, 3, 4, 8, 7, 6 },
	  { 1, 2, 3, 5, 9, 8, 6 },
	  { 2, 0, 3, 6, 7, 9, 6 },
	  { 0, 2, 1, 6, 5, 4, 6 },
	};

	static constexpr int faceEdges[NumberOfFaces][MaxFaceSize / 2 + 1] = {
	  { 0, 4, 3, 3 },
	  { 1, 5, 4, 3 },
	  { 2, 3, 5, 3 },
	  { 2, 1, 0, 3 },
	};

	static constexpr int edgeToNeighborFaces[NumberOfEdges][2] = {
	  { 0, 3 },
	  { 1, 3 },
	  { 2, 3 },
	  { 0, 2 },
	  { 0, 1 },
	  { 1, 2 },
	};

	static constexpr int faceToNeighborFaces[NumberOfFaces][MaxFaceSize / 2 + 1] = {
	  { 3, 1, 2, 3 },
	  { 3, 2, 0, 3 },
	  { 3, 0, 1, 3 },
	  { 2, 1, 0, 3 },
	};

	static constexpr int pointToNeighborEdges[NumberOfPoints][MaxValence + 1] = {
	  { 0, 3, 2, 3 },
	  { 0, 1, 4, 3 },
	  { 1, 2, 5, 3 },
	  { 3, 4, 5, 3 },
	};

	static constexpr int pointToNeighborFaces[NumberOfPoints][MaxValence + 1] = {
	  { 0, 2, 3, 3 },
	  { 3, 1, 0, 3 },
	  { 3, 2, 1, 3 },
	  { 0, 1, 2, 3 },
	};

	static constexpr int pointToOneRingPoints[NumberOfPoints][MaxValence + 1] = {
	  { 1, 3, 2, 3 },
	  { 0, 2, 3, 3 },
	  { 1, 0, 3, 3 },
	  { 0, 1, 2, 3 },
	};

	static int constexpr SubTetras[3][8][4] = {
	  { { 0, 4, 6, 7 }, { 4, 1, 5, 8 }, { 6, 5, 2, 9 }, { 7, 8, 9, 3 }, { 5, 9, 6, 7 }, { 5, 6, 4, 7 },
	    { 5, 4, 8, 7 }, { 5, 8, 9, 7 } },
	  { { 0, 4, 6, 7 }, { 4, 1, 5, 8 }, { 6, 5, 2, 9 }, { 7, 8, 9, 3 }, { 6, 4, 5, 8 }, { 6, 5, 9, 8 },
		{ 6, 9, 7, 8 }, { 6, 7, 4, 8 } },
	  { { 0, 4, 6, 7 }, { 4, 1, 5, 8 }, { 6, 5, 2, 9 }, { 7, 8, 9, 3 }, { 4, 8, 5, 9 }, { 4, 5, 6, 9 },
		{ 4, 6, 7, 9 }, { 4, 7, 8, 9 } }
	};

	int GetEdgePointIds(const int edgeId, const igIndex*& ptIds) override {
		ptIds = edges[edgeId];
		return 3;
	}
	int GetFacePointIds(const int faceId, const igIndex*& ptIds) override {
		ptIds = faces[faceId];
		return faces[faceId][MaxFaceSize];
	}
	int GetFaceEdgeIds(const int faceId, const igIndex*& edgeIds) override {
		edgeIds = faceEdges[faceId];
		return faceEdges[faceId][MaxFaceSize / 2];
	}

	int GetPointToOneRingPoints(const int pointId, const igIndex*& ptIds) override {
		ptIds = pointToOneRingPoints[pointId];
		return pointToOneRingPoints[pointId][MaxValence];
	}
	int GetPointToNeighborEdges(const int pointId, const igIndex*& edgeIds) override {
		edgeIds = pointToNeighborEdges[pointId];
		return pointToNeighborEdges[pointId][MaxValence];
	}
	int GetPointToNeighborFaces(const int pointId, const igIndex*& faceIds) override {
		faceIds = pointToNeighborFaces[pointId];
		return pointToNeighborFaces[pointId][MaxValence];
	}
	int GetEdgeToNeighborFaces(const int edgeId, const igIndex*& ptIds) override {
		ptIds = edgeToNeighborFaces[edgeId];
		return 2;
	}
	int GetFaceToNeighborFaces(const int faceId, const igIndex*& faceIds) override {
		faceIds = faceToNeighborFaces[faceId];
		return faceToNeighborFaces[faceId][MaxFaceSize / 2];
	}

	static int EdgePointIds(const int edgeId, const igIndex*& ptIds) {
		ptIds = edges[edgeId];
		return 3;
	}
	static int FacePointIds(const int faceId, const igIndex*& ptIds) {
		ptIds = faces[faceId];
		return faces[faceId][MaxFaceSize];
	}
	static int FaceEdgeIds(const int faceId, const igIndex*& edgeIds) {
		edgeIds = faceEdges[faceId];
		return faceEdges[faceId][MaxFaceSize / 2];
	}

	static int PointToOneRingPoints(const int pointId, const igIndex*& ptIds) {
		ptIds = pointToOneRingPoints[pointId];
		return pointToOneRingPoints[pointId][MaxValence];
	}
	static int PointToNeighborEdges(const int pointId, const igIndex*& edgeIds) {
		edgeIds = pointToNeighborEdges[pointId];
		return pointToNeighborEdges[pointId][MaxValence];
	}
	static int PointToNeighborFaces(const int pointId, const igIndex*& faceIds) {
		faceIds = pointToNeighborFaces[pointId];
		return pointToNeighborFaces[pointId][MaxValence];
	}
	static int EdgeToNeighborFaces(const int edgeId, const igIndex*& ptIds) {
		ptIds = edgeToNeighborFaces[edgeId];
		return 2;
	}
	static int FaceToNeighborFaces(const int faceId, const igIndex*& faceIds) {
		faceIds = faceToNeighborFaces[faceId];
		return faceToNeighborFaces[faceId][MaxFaceSize / 2];
	}

private:
	QuadraticTetra()
	{
		this->m_Points->SetNumberOfPoints(10);
		this->m_PointIds->SetNumberOfIds(10);
		for (int i = 0; i < 10; i++)
		{
			this->m_Points->SetPoint(i, 0.0, 0.0, 0.0);
			this->m_PointIds->SetId(i, 0);
		}

		m_Line = QuadraticLine::New();
		m_Triangle = QuadraticTriangle::New();
	}
	~QuadraticTetra() override = default;

	QuadraticLine::Pointer m_Line;
	QuadraticTriangle::Pointer m_Triangle;
};


IGAME_NAMESPACE_END
#endif