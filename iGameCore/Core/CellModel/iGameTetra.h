#ifndef iGameTetra_h
#define iGameTetra_h

#include "iGameVolume.h"

IGAME_NAMESPACE_BEGIN
class Tetra : public Volume {
public:
	I_OBJECT(Tetra);
    static Pointer New() { return new Tetra; }
    static Pointer Create(Vector3f p0, Vector3f p1, Vector3f p2, Vector3f p3,
                       igIndex ph0, igIndex ph1, igIndex ph2, igIndex ph3) {
        return new Tetra( p0,  p1,  p2,  p3,
                          ph0,  ph1,  ph2,  ph3);
    }
	
	IGenum GetCellType() const noexcept override { return IG_TETRA; }
    int GetCellSize() const noexcept override { return 4; }
    int GetNumberOfEdges() override { return 6; }
    int GetNumberOfFaces() override { return 4; }

    Cell* GetEdge(const int edgeId) override {
        const int* verts = edges[edgeId];

        m_Line->PointIds->SetId(0, PointIds->GetId(verts[0]));
        m_Line->PointIds->SetId(1, PointIds->GetId(verts[1]));

        m_Line->Points->SetPoint(0, Points->GetPoint(verts[0]));
        m_Line->Points->SetPoint(1, Points->GetPoint(verts[1]));

        return m_Line.get();
    }
    Cell* GetFace(const int faceId) override {
        const int* verts = faces[faceId];
        const int* edges = faceEdges[faceId];
        for (int i = 0; i < 3; ++i) {
            m_Triangle->PointIds->SetId(i, PointIds->GetId(verts[i]));
            m_Triangle->Points->SetPoint(i, Points->GetPoint(verts[i]));
        }
        return m_Triangle.get();
    }

	/**
	 * The number of points of the cell
	 */
	static constexpr int NumberOfPoints = 4;

	/**
	 * The number of edges of the cell.
	 */
	static constexpr int NumberOfEdges = 6;

	/**
	 * The number of faces of the cell.
	 */
	static constexpr int NumberOfFaces = 4;

	/**
	 * The largest number of points of the element face.
	 */
	static constexpr int MaxFaceSize = 3;

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
	  { 0, 3 },
	  { 1, 3 },
	  { 2, 3 },
	};

	// pointIds of a face
	static constexpr int faces[NumberOfFaces][MaxFaceSize + 1] = {
	  { 0, 1, 3, 3 },
	  { 1, 2, 3, 3 },
	  { 2, 0, 3, 3 },
	  { 0, 2, 1, 3 },
	}; 

	// edgeIds of a face
	static constexpr int faceEdges[NumberOfFaces][MaxFaceSize + 1] = {
	  { 0, 4, 3, 3 },
	  { 1, 5, 4, 3 },
	  { 2, 3, 5, 3 },
	  { 2, 1, 0, 3 },
	};

	// The adjacent face number of the edge
	static constexpr int edgeToNeighborFaces[NumberOfEdges][2] = {
	  { 0, 3 },
	  { 1, 3 },
	  { 2, 3 },
	  { 0, 2 },
	  { 0, 1 },
	  { 1, 2 },
	};

	// Face adjacent face index number
	static constexpr int faceToNeighborFaces[NumberOfFaces][MaxFaceSize + 1] = {
	  { 3, 1, 2, 3 },
	  { 3, 2, 0, 3 },
	  { 3, 0, 1, 3 },
	  { 2, 1, 0, 3 },
	};

	// The sequence number of the adjacent edges of the vertex
	static constexpr int pointToNeighborEdges[NumberOfPoints][MaxValence + 1] = {
	  { 0, 3, 2, 3 },
	  { 0, 1, 4, 3 },
	  { 1, 2, 5, 3 },
	  { 3, 4, 5, 3 },
	};

	// The sequence number of the adjacent face of the vertex
	static constexpr int pointToNeighborFaces[NumberOfPoints][MaxValence + 1] = {
	  { 0, 2, 3, 3 },
	  { 3, 1, 0, 3 },
	  { 3, 2, 1, 3 },
	  { 0, 1, 2, 3 },
	};

	// PointIds of a neighborhood of a point
	static constexpr int pointToOneRingPoints[NumberOfPoints][MaxValence + 1] = {
	  { 1, 3, 2, 3 },
	  { 0, 2, 3, 3 },
	  { 1, 0, 3, 3 },
	  { 0, 1, 2, 3 },
	};

	int GetEdgePointIds(const int edgeId, const igIndex*& ptIds) override {
        ptIds = edges[edgeId];
        return 2;
	}
    int GetFacePointIds(const int faceId, const igIndex*& ptIds) override {
        ptIds = faces[faceId];
		return faces[faceId][MaxFaceSize];
	}
	int GetFaceEdgeIds(const int faceId, const igIndex*& edgeIds) override {
		edgeIds = faceEdges[faceId];
		return faceEdges[faceId][MaxFaceSize];
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
		return faceToNeighborFaces[faceId][MaxFaceSize];
	}

	static int EdgePointIds(const int edgeId, const igIndex*& ptIds) {
		ptIds = edges[edgeId];
		return 2;
	}
	static int FacePointIds(const int faceId, const igIndex*& ptIds) {
		ptIds = faces[faceId];
		return faces[faceId][MaxFaceSize];
	}
	static int FaceEdgeIds(const int faceId, const igIndex*& edgeIds) {
		edgeIds = faceEdges[faceId];
		return faceEdges[faceId][MaxFaceSize];
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
		return faceToNeighborFaces[faceId][MaxFaceSize];
	}

private:
	Tetra()
	{
		this->Points->SetNumberOfPoints(4);
		this->PointIds->SetNumberOfIds(4);
		for (int i = 0; i < 4; i++)
		{
			this->Points->SetPoint(i, 0.0, 0.0, 0.0);
			this->PointIds->SetId(i, 0);
		}

		m_Line = Line::New();
        m_Triangle = Triangle::New();
	}
    Tetra(Vector3f p0, Vector3f p1, Vector3f p2, Vector3f p3, igIndex ph0,igIndex ph1, igIndex ph2, igIndex ph3) {
        this->Points->SetNumberOfPoints(4);
        this->PointIds->SetNumberOfIds(4);

        this->Points->SetPoint(0, p0[0], p0[1], p0[2]);
        this->PointIds->SetId(0, ph0);

		this->Points->SetPoint(1, p1[0], p1[1], p1[2]);
        this->PointIds->SetId(0, ph1);

        this->Points->SetPoint(2, p2[0], p2[1], p2[2]);
        this->PointIds->SetId(0, ph2);

        this->Points->SetPoint(3, p3[0], p3[1], p3[2]);
        this->PointIds->SetId(0, ph3);
        m_Line = Line::New();
        m_Triangle = Triangle::New();
    }
	~Tetra() override = default;

	Line::Pointer m_Line;
    Triangle::Pointer m_Triangle;
};


IGAME_NAMESPACE_END
#endif