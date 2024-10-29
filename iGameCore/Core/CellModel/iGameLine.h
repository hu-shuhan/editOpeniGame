#ifndef iGameLine_h
#define iGameLine_h

#include "iGameCell.h"

IGAME_NAMESPACE_BEGIN
class Line : public Cell {
public:
	I_OBJECT(Line);
	static Pointer New() { return new Line; }

	IGenum GetCellType() const noexcept override { return IG_LINE; }
    int GetCellSize() const noexcept override { return 2; }
    int GetNumberOfEdges() override { return 0; }
    int GetNumberOfFaces() override { return 0; }
	Cell* GetEdge(const int) override { return nullptr; }
    Cell* GetFace(const int) override { return nullptr; }

	static double ComputePointToLineDis(const Vector3d& linePoint,
                                        const Vector3d& normal,
                                        const Vector3d& point) 
	{
        return std::fabs(CrossProduct(normal, point - linePoint).length() / normal.length());
    }

protected:
	Line()
	{
		this->m_Points->SetNumberOfPoints(2);
		this->m_PointIds->SetNumberOfIds(2);
		for (int i = 0; i < 2; i++)
		{
			this->m_Points->SetPoint(i, 0.0, 0.0, 0.0);
			this->m_PointIds->SetId(i, 0);
		}
	}
	~Line() override = default;
};
IGAME_NAMESPACE_END
#endif