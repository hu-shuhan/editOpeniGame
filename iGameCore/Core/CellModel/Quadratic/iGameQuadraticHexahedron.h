#ifndef iGameQuadraticHexahedron_h
#define iGameQuadraticHexahedron_h

#include "Base/iGameQuadraticVolume.h"
#include "iGameQuadraticQuad.h"

IGAME_NAMESPACE_BEGIN
class QuadraticHexahedron : public QuadraticVolume {
public:
	I_OBJECT(QuadraticHexahedron);
	static Pointer New() { return new QuadraticHexahedron; }

	IGenum GetCellType() const noexcept override { return IG_QUADRATIC_HEXAHEDRON; }
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
		for (int i = 0; i < 8; ++i) {
			m_Quad->m_PointIds->SetId(i, m_PointIds->GetId(verts[i]));
			m_Quad->m_Points->SetPoint(i, m_Points->GetPoint(verts[i]));
		}
		for (int i = 0; i < 4; ++i) {
			m_Quad->EdgeIds->SetId(i, EdgeIds->GetId(edges[i]));
		}
		return m_Quad.get();
	}


	static constexpr int NumberOfPoints = 20;

	static constexpr int NumberOfEdges = 12;

	static constexpr int NumberOfFaces = 6;

	static constexpr int MaxFaceSize = 8;

	static constexpr int MaxValence = 3;

	static constexpr int edges[NumberOfEdges][3] = {
	  { 0, 1 ,8 },
	  { 1, 2 ,9 },
	  { 3, 2 ,10 },
	  { 0, 3 ,11 },
	  { 4, 5 ,12 },
	  { 5, 6 ,13 },
	  { 7, 6 ,14 },
	  { 4, 7 ,15 },
	  { 0, 4 ,16 },
	  { 1, 5 ,17 },
	  { 2, 6 ,18 },
	  { 3, 7 ,19 },
	};

	static constexpr int faces[NumberOfFaces][MaxFaceSize + 1] = {
	  { 0, 4, 7, 3, 16, 15, 19, 11, 4 },
	  { 1, 2, 6, 5, 9, 18, 13, 17, 4 },
	  { 0, 1, 5, 4, 8, 17, 12, 16, 4 },
	  { 3, 7, 6, 2, 19, 14, 18, 10, 4 },
	  { 0, 3, 2, 1, 11, 10, 9, 8, 4 },
	  { 4, 5, 6, 7, 12, 13, 14, 15, 4 },
	};

	static constexpr int faceEdges[NumberOfFaces][MaxFaceSize / 2 + 1] = {
	  { 8, 7, 11, 3, 4 },
	  { 1, 10, 5, 9, 4 },
	  { 0, 9, 4, 8, 4 },
	  { 11, 6, 10, 2, 4 },
	  { 3, 2, 1, 0, 4 },
	  { 4, 5, 6, 7, 4 },
	};

	static constexpr int edgeToNeighborFaces[NumberOfEdges][2] = {
	  { 2, 4 },
	  { 1, 4 },
	  { 3, 4 },
	  { 0, 4 },
	  { 2, 5 },
	  { 1, 5 },
	  { 3, 5 },
	  { 0, 5 },
	  { 0, 2 },
	  { 1, 2 },
	  { 1, 3 },
	  { 0, 3 },
	};

	static constexpr int faceToNeighborFaces[NumberOfFaces][MaxFaceSize / 2 + 1] = {
		{ 4, 2, 5, 3, 4 },
		{ 4, 3, 5, 2, 4 },
		{ 4, 1, 5, 0, 4 },
		{ 0, 5, 1, 4, 4 },
		{ 0, 3, 1, 2, 4 },
		{ 2, 1, 0, 3, 4 },
	};

	static constexpr int pointToNeighborEdges[NumberOfPoints][MaxValence + 1] = {
		{ 0, 8,  3,  3 },
		{ 0, 1,  9,  3 },
		{ 1, 2,  10, 3 },
		{ 2, 3,  11, 3 },
		{ 7, 8,  4,  3 },
		{ 4, 9,  5,  3 },
		{ 5, 10, 6,  3 },
		{ 6, 11, 7,  3 },
	};

	static constexpr int pointToNeighborFaces[NumberOfPoints][MaxValence + 1] = {
		{ 2, 0, 4, 3 },
		{ 4, 1, 2, 3 },
		{ 4, 3, 1, 3 },
		{ 4, 0, 3, 3 },
		{ 5, 2, 0, 3 },
		{ 2, 1, 5, 3 },
		{ 1, 3, 5, 3 },
		{ 3, 0, 5, 3 },
	};

	static constexpr int pointToOneRingPoints[NumberOfPoints][MaxValence + 1] = {
		{ 1, 4, 3, 3 },
		{ 0, 2, 5, 3 },
		{ 1, 3, 6, 3 },
		{ 2, 0, 7, 3 },
		{ 5, 7, 0, 3 },
		{ 4, 1, 6, 3 },
		{ 5, 2, 7, 3 },
		{ 6, 3, 4, 3 },
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
	QuadraticHexahedron()
	{
		this->m_Points->SetNumberOfPoints(20);
		this->m_PointIds->SetNumberOfIds(20);
		for (int i = 0; i < 20; i++)
		{
			this->m_Points->SetPoint(i, 0.0, 0.0, 0.0);
			this->m_PointIds->SetId(i, 0);
		}

		m_Line = QuadraticLine::New();
		m_Quad = QuadraticQuad::New();
	}
	~QuadraticHexahedron() override = default;

	QuadraticLine::Pointer m_Line;
	QuadraticQuad::Pointer m_Quad;
};


IGAME_NAMESPACE_END
#endif