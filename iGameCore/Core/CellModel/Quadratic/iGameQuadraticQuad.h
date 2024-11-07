#ifndef iGameQuadraticQuad_h
#define iGameQuadraticQuad_h

#include "Base/iGameQuadraticFace.h"
#include "iGameTriangle.h"

IGAME_NAMESPACE_BEGIN
class QuadraticQuad : public QuadraticFace {
public:
	I_OBJECT(QuadraticQuad);
	static Pointer New() { return new QuadraticQuad; }

	IGenum GetCellType() const noexcept override { return IG_QUADRATIC_QUAD; }
	int GetCellSize() const noexcept override { return 8; }
	int GetNumberOfEdges() override { return 4; }

	Vector3f GetNormal() override
	{
		const Vector3f& v0 = this->m_Points->GetPoint(0);
		const Vector3f& v1 = this->m_Points->GetPoint(1);
		const Vector3f& v2 = this->m_Points->GetPoint(2);
		const Vector3f& v3 = this->m_Points->GetPoint(3);
		return this->Self::ComputrAverageNormal(v0, v1, v2, v3);
	}

	static Vector3f ComputrAverageNormal(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, const Vector3f& v3)
	{
		Vector3f center = (v0 + v1 + v2 + v3) / 4;

		Vector3f normal;
		normal.setZero();

		normal += Triangle::ComputrNormal(center, v0, v1);
		normal += Triangle::ComputrNormal(center, v1, v2);
		normal += Triangle::ComputrNormal(center, v2, v3);
		normal += Triangle::ComputrNormal(center, v3, v0);

		normal.normalize();
		return normal;
	}

	static int constexpr SubTriangles[6][3] = { {0,4,7},{4,1,3},{5,2,6},{6,3,7},{4,3,7},{5,6,7}};
protected:
	QuadraticQuad()
	{
		this->m_Points->SetNumberOfPoints(8);
		this->m_PointIds->SetNumberOfIds(8);
		for (int i = 0; i < 8; i++)
		{
			this->m_Points->SetPoint(i, 0.0, 0.0, 0.0);
			this->m_PointIds->SetId(i, 0);
		}
	}
	~QuadraticQuad() override = default;
};
IGAME_NAMESPACE_END
#endif