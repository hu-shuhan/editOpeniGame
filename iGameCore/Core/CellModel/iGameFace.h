#ifndef iGameFace_h
#define iGameFace_h

#include "iGameCell.h"
#include "iGameLine.h"

IGAME_NAMESPACE_BEGIN
class Face : public Cell {
public:
	I_OBJECT(Face);

	int GetNumberOfFaces() override { return 0; }
    Cell* GetEdge(const int edgeId) override 
	{
		int edgeIdPlus = (edgeId + 1) % this->GetNumberOfEdges();

		m_Line->m_PointIds->SetId(0, this->m_PointIds->GetId(edgeId));
		m_Line->m_PointIds->SetId(1, this->m_PointIds->GetId(edgeIdPlus));

		m_Line->m_Points->SetPoint(0, this->m_Points->GetPoint(edgeId));
		m_Line->m_Points->SetPoint(1, this->m_Points->GetPoint(edgeIdPlus));

		return m_Line;
	}
    Cell* GetFace(const int) override { return nullptr; }

	virtual Vector3f GetNormal() = 0;

protected:
	Face()
	{
		m_Line = Line::New();
	}
	~Face() override = default;

	Line::Pointer m_Line;
};
IGAME_NAMESPACE_END
#endif