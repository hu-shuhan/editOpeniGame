#ifndef iGameCell_h
#define iGameCell_h

#include "iGameCellType.h"
#include "iGameIdArray.h"
#include "iGameObject.h"
#include "iGamePoints.h"
#define IGAME_CELL_MAX_SIZE 256
IGAME_NAMESPACE_BEGIN
class Cell : public Object {
public:
	I_OBJECT(Cell);

	virtual IGenum GetCellType() const noexcept = 0;
	virtual int GetCellSize() const noexcept {
		return this->m_PointIds->GetNumberOfIds();
	}

    virtual int GetNumberOfPoints() { return this->m_PointIds->GetNumberOfIds(); }
    virtual int GetNumberOfEdges() = 0;
    virtual int GetNumberOfFaces() = 0;
	// return   position and id of the i th point in cell
    igIndex GetPointId(const int id) { return this->m_PointIds->GetId(id); }
    Point& GetPoint(const int id) { return this->m_Points->GetPoint(id); }
    const Point& GetPoint(const int id) const {
        return this->m_Points->GetPoint(id);
    }

	virtual Cell* GetEdge(const int edgeId) = 0;
	virtual Cell* GetFace(const int faceId) = 0;

	virtual void Reset() {
		this->m_PointIds->Reset();
		this->m_Points->Reset();
	}
	static igIndex GetCellDimension(igIndex celltype) {
		return GetCellDimension(IGCellType(celltype));
	}
	static igIndex GetCellDimension(IGCellType celltype) {
		switch (celltype) {
		case IG_EMPTY_CELL:
			return 0;
		case IG_VERTEX:
			return 1;
		case IG_LINE:
		case IG_FACE:
		case IG_TRIANGLE:
		case IG_QUAD:
		case IG_POLYGON:
			return 2;
		case IG_VOLUME:
		case IG_POLYHEDRON:
		case IG_TETRA:
		case IG_HEXAHEDRON:
		case IG_PYRAMID:
		case IG_PRISM:
			return 3;
		case IG_QUADRATIC_EDGE:
		case IG_QUADRATIC_TRIANGLE:
		case IG_QUADRATIC_QUAD:
		case IG_QUADRATIC_POLYGON:
		case IG_QUADRATIC_TETRA:
		case IG_QUADRATIC_HEXAHEDRON:
		case IG_QUADRATIC_PRISM:
		case IG_QUADRATIC_PYRAMID:
		case IG_BIQUADRATIC_QUAD:
		case IG_TRIQUADRATIC_HEXAHEDRON:
		case IG_TRIQUADRATIC_PYRAMID:
		case IG_QUADRATIC_LINEAR_QUAD:
		case IG_QUADRATIC_LINEAR_WEDGE:
		case IG_BIQUADRATIC_QUADRATIC_WEDGE:
		case IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
		case IG_BIQUADRATIC_TRIANGLE:
			return 4;
		default:
			return 0;
			break;
		}
	}

	Points::Pointer m_Points{};
	IdArray::Pointer m_PointIds{};

protected:
	Cell() {
		this->m_Points = Points::New();
		this->m_PointIds = IdArray::New();
	};
	~Cell() override = default;
};



IGAME_NAMESPACE_END
#endif