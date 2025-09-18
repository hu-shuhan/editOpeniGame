
#ifndef iGameLagrangeFace_h
#define iGameLagrangeFace_h

#include "iGameCell.h"
#include "iGameLagrangeLine.h" 

IGAME_NAMESPACE_BEGIN

class LagrangeFace : public Cell {
public:
    // 设置/获取单元的阶数
    void SetOrder(int order) { m_Order = order; }
    int GetOrder() const { return m_Order; }

    // 2D单元没有面
    int GetNumberOfFaces() override { return 0; }
    Cell* GetFace(const int) override { return nullptr; }

    // 动态生成一个LagrangeLine对象来表示边
    Cell* GetEdge(const int edgeId) override {
        const igIndex* ptIds = nullptr;
        int numEdgePoints = GetEdgePointIds(edgeId, ptIds);
        if (numEdgePoints == 0) return nullptr;

        m_Edge->SetOrder(numEdgePoints - 1); // 边的阶数是节点数-1
        m_Edge->Reset();
        m_Edge->m_PointIds->SetNumberOfIds(numEdgePoints);
        m_Edge->m_Points->SetNumberOfPoints(numEdgePoints);

        for (int i = 0; i < numEdgePoints; ++i) {
            m_Edge->m_PointIds->SetId(i, this->m_PointIds->GetId(ptIds[i]));
            m_Edge->m_Points->SetPoint(i, this->m_Points->GetPoint(ptIds[i]));
        }
        return m_Edge.get();
    }

    // 子类必须实现这些纯虚函数
    virtual int GetNumberOfEdges() = 0;
    virtual int GetEdgePointIds(const int edgeId, const igIndex*& ptIds) = 0;

protected:
    LagrangeFace() : m_Order(0) { m_Edge = LagrangeLine::New(); }
    ~LagrangeFace() override = default;

    int m_Order;
    LagrangeLine::Pointer m_Edge;
};

IGAME_NAMESPACE_END
#endif