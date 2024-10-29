#ifndef iGameQuad_h
#define iGameQuad_h

#include "iGameFace.h"
#include "iGameTriangle.h"

IGAME_NAMESPACE_BEGIN
class Quad : public Face {
public:
	I_OBJECT(Quad);
	static Pointer New() { return new Quad; }

	IGenum GetCellType() const noexcept override { return IG_QUAD; }
    int GetCellSize() const noexcept override { return 4; }
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

protected:
	Quad()
	{
		this->m_Points->SetNumberOfPoints(4);
		this->m_PointIds->SetNumberOfIds(4);
		for (int i = 0; i < 4; i++)
		{
			this->m_Points->SetPoint(i, 0.0, 0.0, 0.0);
			this->m_PointIds->SetId(i, 0);
		}
	}
	~Quad() override = default;
};
IGAME_NAMESPACE_END
#endif