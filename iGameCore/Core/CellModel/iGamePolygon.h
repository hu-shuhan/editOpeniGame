#ifndef iGamePolygon_h
#define iGamePolygon_h

#include "iGameFace.h"
#include "iGameTriangle.h"

IGAME_NAMESPACE_BEGIN
class Polygon : public Face {
public:
	I_OBJECT(Polygon);
	static Pointer New() { return new Polygon; }

	IGenum GetCellType() const noexcept override { return IG_POLYGON; }
    int GetNumberOfEdges() override { return this->GetCellSize(); }

	Vector3f GetNormal() override
	{
		return this->Self::ComputrAverageNormal(this->m_Points);
	}

	static Vector3f ComputrAverageNormal(Points::Pointer points)
	{
		int npts = points->GetNumberOfPoints();
		Vector3f center(0);
		for (int i = 0; i < npts; i++) {
			center += points->GetPoint(i);
		}
		center /= npts;

		Vector3f normal;
		normal.setZero();

		for (int i = 0; i < npts - 1; i++) {
			const Point& v0 = points->GetPoint(i);
			const Point& v1 = points->GetPoint(i + 1);
			normal += Triangle::ComputrNormal(center, v0, v1);
		}
		const Point& v0 = points->GetPoint(npts - 1);
		const Point& v1 = points->GetPoint(0);
		normal += Triangle::ComputrNormal(center, v0, v1);

		normal.normalize();
		return normal;
	}

protected:
	Polygon() = default;
	~Polygon() override = default;
};
IGAME_NAMESPACE_END
#endif