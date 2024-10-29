#ifndef iGameQuadraticLine_h
#define iGameQuadraticLine_h

#include "iGameCell.h"

IGAME_NAMESPACE_BEGIN
class QuadraticLine : public Cell {
public:
	I_OBJECT(QuadraticLine);
	static Pointer New() { return new QuadraticLine; }

	IGenum GetCellType() const noexcept override { return IG_QUADRATIC_EDGE; }
	int GetCellSize() const noexcept override { return 3; }
	int GetNumberOfEdges() override { return 0; }
	int GetNumberOfFaces() override { return 0; }
	Cell* GetEdge(const int) override { return nullptr; }
	Cell* GetFace(const int) override { return nullptr; }


protected:
	QuadraticLine()
	{
		this->m_Points->SetNumberOfPoints(3);
		this->m_PointIds->SetNumberOfIds(3);
		for (int i = 0; i < 3; i++)
		{
			this->m_Points->SetPoint(i, 0.0, 0.0, 0.0);
			this->m_PointIds->SetId(i, 0);
		}
	}
	~QuadraticLine() override = default;
};
IGAME_NAMESPACE_END
#endif