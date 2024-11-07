#ifndef iGameQuadraticTriangle_h
#define iGameQuadraticTriangle_h

#include "Base/iGameQuadraticFace.h"

IGAME_NAMESPACE_BEGIN
class QuadraticTriangle : public QuadraticFace {
public:
	I_OBJECT(QuadraticTriangle);
	static Pointer New() { return new QuadraticTriangle; }

	IGenum GetCellType() const noexcept override { return IG_QUADRATIC_TRIANGLE; }
	int GetCellSize() const noexcept override { return 6; }
	int GetNumberOfEdges() override { return 3; }

	Vector3f GetNormal() override
	{
		const Vector3f& v0 = this->m_Points->GetPoint(0);
		const Vector3f& v1 = this->m_Points->GetPoint(1);
		const Vector3f& v2 = this->m_Points->GetPoint(2);
		return this->Self::ComputrNormal(v0, v1, v2);
	}

	static Vector3f ComputrNormal(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2)
	{
		Vector3f d10 = v1 - v0;
		Vector3f d20 = v2 - v0;

		Vector3f normal = CrossProduct(d10, d20);
		normal.normalize();
		return normal;
	}
	static int constexpr SubTriangles[4][3] = { 
  { 0, 3, 5 },
  { 3, 1, 4 },
  { 5, 4, 2 },
  { 4, 5, 3 } };

protected:
	QuadraticTriangle()
	{
		this->m_Points->SetNumberOfPoints(6);
		this->m_PointIds->SetNumberOfIds(6);
		for (int i = 0; i < 6; i++)
		{
			this->m_Points->SetPoint(i, 0.0, 0.0, 0.0);
			this->m_PointIds->SetId(i, 0);
		}
	}
	~QuadraticTriangle() override = default;
};
IGAME_NAMESPACE_END
#endif