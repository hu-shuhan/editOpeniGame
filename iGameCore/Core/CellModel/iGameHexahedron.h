#ifndef iGameHexahedron_h
#define iGameHexahedron_h

#include "iGameVolume.h"
#include<iGameTetra.h>
IGAME_NAMESPACE_BEGIN
class Hexahedron : public Volume {
public:
	I_OBJECT(Hexahedron);
	static Pointer New() { return new Hexahedron; }

	IGenum GetCellType() const noexcept override { return IG_HEXAHEDRON; }
	int GetCellSize() const noexcept override { return 8; }
	int GetNumberOfEdges() override { return 12; }
	int GetNumberOfFaces() override { return 6; }

	Cell* GetEdge(const int edgeId) override {
		const int* verts = edges[edgeId];

		m_Line->PointIds->SetId(0, this->PointIds->GetId(verts[0]));
		m_Line->PointIds->SetId(1, this->PointIds->GetId(verts[1]));

		m_Line->Points->SetPoint(0, this->Points->GetPoint(verts[0]));
		m_Line->Points->SetPoint(1, this->Points->GetPoint(verts[1]));

		return m_Line.get();
	}
	Cell* GetFace(const int faceId) override {
		const int* verts = faces[faceId];
		const int* edges = faceEdges[faceId];
		for (int i = 0; i < 4; ++i) {
			m_Quad->PointIds->SetId(i, PointIds->GetId(verts[i]));
			m_Quad->Points->SetPoint(i, Points->GetPoint(verts[i]));
		}

		return m_Quad.get();
	}
    std::vector<iGame::Cell::Pointer> clipCelltoTetra() override {
        std::vector<iGame::Cell::Pointer> result;
        for (int i = 0; i < 5; i++) {
		const int* verts = clipedCell[i];
            Tetra::Pointer tetra = Tetra::New();
			for (int j = 0; j < 4; j++) {
                tetra->Points->SetPoint(j, this->Points->GetPoint(verts[j]));
                tetra->PointIds->SetId(j, this->PointIds->GetId(verts[j]));
		}
        result.emplace_back(tetra);
		}
        return result;
	}
	/**
	 * The number of points of the cell
	 */
	static constexpr int NumberOfPoints = 8;

	/**
	 * The number of edges of the cell.
	 */
	static constexpr int NumberOfEdges = 12;

	/**
	 * The number of faces of the cell.
	 */
	static constexpr int NumberOfFaces = 6;

	/**
	 * The largest number of points of the element face.
	 */
	static constexpr int MaxFaceSize = 4;

	/**
	 * The maximum degree of a cell point.
	 */
	static constexpr int MaxValence = 3;

	/***************** The last digit of the array for all Max + 1 indicates the number *****************/

	// pointIds of a edge
	static constexpr int edges[NumberOfEdges][2] = {
	  { 0, 1 },
	  { 1, 2 },
	  { 3, 2 },
	  { 0, 3 },
	  { 4, 5 },
	  { 5, 6 },
	  { 7, 6 },
	  { 4, 7 },
	  { 0, 4 },
	  { 1, 5 },
	  { 2, 6 },
	  { 3, 7 },
	};

	// pointIds of a face
	static constexpr int faces[NumberOfFaces][MaxFaceSize + 1] = {
	  { 0, 4, 7, 3, 4 },
	  { 1, 2, 6, 5, 4 },
	  { 0, 1, 5, 4, 4 },
	  { 3, 7, 6, 2, 4 },
	  { 0, 3, 2, 1, 4 },
	  { 4, 5, 6, 7, 4 },
	};

	// edgeIds of a face
	static constexpr int faceEdges[NumberOfFaces][MaxFaceSize + 1] = {
	  { 8, 7, 11, 3, 4 },
	  { 1, 10, 5, 9, 4 },
	  { 0, 9, 4, 8, 4 },
	  { 11, 6, 10, 2, 4 },
	  { 3, 2, 1, 0, 4 },
	  { 4, 5, 6, 7, 4 },
	};

	// The adjacent face number of the edge
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

	// Face adjacent face index number
	static constexpr int faceToNeighborFaces[NumberOfFaces][MaxFaceSize + 1] = {
		{ 4, 2, 5, 3, 4 },
		{ 4, 3, 5, 2, 4 },
		{ 4, 1, 5, 0, 4 },
		{ 0, 5, 1, 4, 4 },
		{ 0, 3, 1, 2, 4 },
		{ 2, 1, 0, 3, 4 },
	};

	// The sequence number of the adjacent edges of the vertex
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

	// The sequence number of the adjacent face of the vertex
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

	// PointIds of a neighborhood of a point
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
	//PointIds of cliped cell
	static constexpr int clipedCell[5][4] = {
		{ 0, 1, 3, 4 },
		{ 1, 2, 3, 6 },
		{ 3, 4, 6, 7 },
		{ 1, 4, 5, 6 },
		{ 1, 3, 4, 6 },
	};

	int GetEdgePointIds(const int edgeId, const igIndex*& pts) override {
		pts = edges[edgeId];
		return 2;
	}
	int GetFacePointIds(const int faceId, const igIndex*& pts) override {
		pts = faces[faceId];
		return faces[faceId][MaxFaceSize];
	}
	int GetFaceEdgeIds(const int faceId, const igIndex*& edgeIds) override {
		edgeIds = faceEdges[faceId];
		return faceEdges[faceId][MaxFaceSize];
	}
	int GetEdgeToNeighborFaces(const int edgeId, const igIndex*& pts) override {
		pts = edgeToNeighborFaces[edgeId];
		return 2;
	}
	int GetFaceToNeighborFaces(const int faceId, const igIndex*& faceIds) override {
		faceIds = faceToNeighborFaces[faceId];
		return faceToNeighborFaces[faceId][MaxFaceSize];
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
	Hexahedron()
	{
		this->Points->SetNumberOfPoints(4);
		this->PointIds->SetNumberOfIds(4);
		for (int i = 0; i < 4; i++)
		{
			this->Points->SetPoint(i, 0.0, 0.0, 0.0);
			this->PointIds->SetId(i, 0);
		}

		m_Line = Line::New();
		m_Quad = Quad::New();
	}
	~Hexahedron() override = default;

	Line::Pointer m_Line;
	Quad::Pointer m_Quad;
};


IGAME_NAMESPACE_END
#endif