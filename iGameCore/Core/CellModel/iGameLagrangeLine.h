
#ifndef iGameLagrangeLine_h
#define iGameLagrangeLine_h

#include "iGameCell.h"

IGAME_NAMESPACE_BEGIN

class LagrangeLine : public Cell {
public:
    I_OBJECT(LagrangeLine);
    static Pointer New() { return new LagrangeLine; }

    // 设置/获取单元的阶数
    void SetOrder(int order) { m_Order = order; }
    int GetOrder() const { return m_Order; }

    // 统一使用 IG_QUADRATIC_EDGE 代表所有高阶边
    IGenum GetCellType() const noexcept override { return IG_QUADRATIC_EDGE; }

    // 顶点数由阶数动态决定
    int GetNumberOfPoints() override { return m_Order + 1; }

    // 1D单元没有边和面
    int GetNumberOfEdges() override { return 0; }
    int GetNumberOfFaces() override { return 0; }
    Cell* GetEdge(const int) override { return nullptr; }
    Cell* GetFace(const int) override { return nullptr; }

protected:
    LagrangeLine() : m_Order(0) {}
    ~LagrangeLine() override = default;

    int m_Order;
};

IGAME_NAMESPACE_END
#endif