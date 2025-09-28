#ifndef iGameLagrangeTriangle_h
#define iGameLagrangeTriangle_h

#include "iGameLagrangeFace.h"

IGAME_NAMESPACE_BEGIN

class LagrangeTriangle : public LagrangeFace {
public:
    I_OBJECT(LagrangeTriangle);
    static Pointer New() { return new LagrangeTriangle; }

    IGenum GetCellType() const noexcept override { return IG_LAGRANGE_TRIANGLE; }

    // 使用通用公式，支持任意阶
    int GetNumberOfPoints() override {
        if (m_Order <= 0) return 0;
        return (m_Order + 1) * (m_Order + 2) / 2;
    }
    int GetNumberOfEdges() override { return 3; }

    int GetEdgePointIds(const int edgeId, const igIndex*& ptIds) override {
        if (edgeId < 0 || edgeId >= 3) return 0;
        switch (m_Order) {
            case 1: { // 线性 (2节点边)
                static const igIndex linearEdges[3][2] = {{0, 1}, {1, 2}, {2, 0}};
                ptIds = linearEdges[edgeId];
                return 2;
            }
            case 2: { // 二次 (3节点边)
                static const igIndex quadraticEdges[3][3] = {{0, 1, 3}, {1, 2, 4}, {2, 0, 5}};
                ptIds = quadraticEdges[edgeId];
                return 3;
            }
            case 3: { // 三次 (4节点边)
                static const igIndex cubicEdges[3][4] = {{0, 1, 3, 4}, {1, 2, 5, 6}, {2, 0, 7, 8}};
                ptIds = cubicEdges[edgeId];
                return 4;
            }
            case 4: { // 四次 (5节点边)
                static const igIndex quarticEdges[3][5] = {{0, 1, 3, 4, 5}, {1, 2, 6, 7, 8}, {2, 0, 9, 10, 11}};
                ptIds = quarticEdges[edgeId];
                return 5;
            }
            default:
                return 0;
        }
    }

protected:
    LagrangeTriangle() = default;
    ~LagrangeTriangle() override = default;
};

IGAME_NAMESPACE_END
#endif