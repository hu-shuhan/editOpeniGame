#ifndef iGamePrism_h
#define iGamePrism_h

#include "iGameVolume.h"
#include <iGameTetra.h>
IGAME_NAMESPACE_BEGIN
class Prism : public Volume {
public:
	I_OBJECT(Prism);
	static Pointer New() { return new Prism; }

	IGenum GetCellType() const noexcept override { return IG_PRISM; }
    int GetCellSize() const noexcept override { return 6; }
    int GetNumberOfEdges() override { return 9; }
    int GetNumberOfFaces() override { return 5; }

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
        Face* face;
        if (faces[faceId][MaxFaceSize] == 3) {
            face = m_Triangle.get();
        } else {
            face = m_Quad.get();
        }
        for (int i = 0; i < faces[faceId][MaxFaceSize]; ++i) {
            face->PointIds->SetId(i, this->PointIds->GetId(verts[i]));
            face->Points->SetPoint(i, this->Points->GetPoint(verts[i]));
        }

        return face;
    }
    std::vector<iGame::Cell::Pointer> clipCelltoTetra() override {
        std::vector<iGame::Cell::Pointer> result;
        for (int i = 0; i < 6; i++) {
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
	static constexpr int NumberOfPoints = 6;

	/**
	 * The number of edges of the cell.
	 */
	static constexpr int NumberOfEdges = 9;

	/**
	 * The number of faces of the cell.
	 */
	static constexpr int NumberOfFaces = 5;

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
	  { 2, 0 },
	  { 3, 4 },
	  { 4, 5 },
	  { 5, 3 },
	  { 0, 3 },
	  { 1, 4 },
	  { 2, 5 },
	};

	// pointIds of a face
	static constexpr int faces[NumberOfFaces][MaxFaceSize + 1] = {
	  { 0, 1, 2, -1, 3 },
	  { 3, 5, 4, -1, 3 },
	  { 0, 3, 4, 1,  4 },
	  { 1, 4, 5, 2,  4 },
	  { 2, 5, 3, 0,  4 },
	};

	// edgeIds of a face
	static constexpr int faceEdges[NumberOfFaces][MaxFaceSize + 1] = {
	  { 0, 1, 2, -1, 3},
	  { 5, 4, 3, -1, 3 },
	  { 6, 3, 7, 0, 4 },
	  { 7, 4, 8, 1, 4 },
	  { 8, 5, 6, 2, 4 },
	};

	// The adjacent face number of the edge
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

	// Face adjacent face index number
	static constexpr int faceToNeighborFaces[NumberOfFaces][MaxFaceSize + 1] = {
	  { 4, 3, 2, -1, 3 },
	  { 2, 3, 4, -1, 3 },
	  { 0, 3, 1, 4 , 4 },
	  { 0, 4, 1, 2 , 4 },
	  { 0, 2, 1, 3 , 4 },
	};

	// The sequence number of the adjacent edges of the vertex
	static constexpr int pointToNeighborEdges[NumberOfPoints][MaxValence + 1] = {
	  { 0, 6, 2, 3 },
	  { 0, 1, 7, 3 },
	  { 1, 2, 8, 3 },
	  { 3, 5, 6, 3 },
	  { 3, 7, 4, 3 },
	  { 4, 8, 5, 3 },
	};

	// The sequence number of the adjacent face of the vertex
	static constexpr int pointToNeighborFaces[NumberOfPoints][MaxValence + 1] = {
	  { 2, 4, 0, 3 },
	  { 0, 3, 2, 3 },
	  { 0, 4, 3, 3 },
	  { 1, 4, 2, 3 },
	  { 2, 3, 1, 3 },
	  { 3, 4, 1, 3 },
	};

	// PointIds of a neighborhood of a point
	static constexpr int pointToOneRingPoints[NumberOfPoints][MaxValence + 1] = {
	  { 1, 3, 2, 3 },
	  { 0, 2, 4, 3 },
	  { 1, 0, 5, 3 },
	  { 4, 5, 0, 3 },
	  { 3, 1, 5, 3 },
	  { 4, 2, 3, 3 },
	};	
	static constexpr int clipedCell[6][4] = {
	  { 0, 1, 2, 5 },
	  { 0, 3, 4, 5 },
	  { 0, 1, 4, 5 },
	  { 0, 3, 4, 1},
	  { 1, 4, 5, 2},
	  { 2, 5, 3, 0},
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
	Prism()
	{
		this->Points->SetNumberOfPoints(6);
		this->PointIds->SetNumberOfIds(6);
		for (int i = 0; i < 6; i++)
		{
			this->Points->SetPoint(i, 0.0, 0.0, 0.0);
			this->PointIds->SetId(i, 0);
		}

		m_Line = Line::New();
		m_Triangle = Triangle::New();
		m_Quad = Quad::New();
	}
	~Prism() override = default;

	Line::Pointer m_Line;
    Triangle::Pointer m_Triangle;
    Quad::Pointer m_Quad;
};


IGAME_NAMESPACE_END
#endif