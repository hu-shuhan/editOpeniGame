#ifndef iGameLagrangePrism_h
#define iGameLagrangePrism_h

#include "iGameLagrangeLine.h"
#include "iGameLagrangeQuadrilateral.h"
#include "iGameLagrangeTriangle.h"
#include "iGameLagrangeVolume.h"

IGAME_NAMESPACE_BEGIN

class LagrangePrism : public LagrangeVolume {
public:
    I_OBJECT(LagrangePrism);
    static Pointer New() { return new LagrangePrism; }

    IGenum GetCellType() const noexcept override { return IG_LAGRANGE_PRISM; } // 使用Wedge作为Prism的类型
    int GetNumberOfEdges() override { return 9; }
    int GetNumberOfFaces() override { return 5; }

    int GetNumberOfPoints() override {
        if (m_Order <= 0) return 0;
        // 公式: (p+1)*(p+2)/2 * (p+1) = (p+1)^2 * (p+2) / 2
        return (m_Order + 1) * (m_Order + 1) * (m_Order + 2) / 2;
    }

    Cell* GetEdge(const int edgeId) override {
        const igIndex* ptIds = nullptr;
        int numEdgePoints = GetEdgePointIds(edgeId, ptIds);
        if (numEdgePoints == 0) return nullptr;

        m_Edge->SetOrder(numEdgePoints - 1);
        m_Edge->Reset();
        m_Edge->m_PointIds->SetNumberOfIds(numEdgePoints);
        m_Edge->m_Points->SetNumberOfPoints(numEdgePoints);

        for (int i = 0; i < numEdgePoints; ++i) {
            m_Edge->m_PointIds->SetId(i, this->m_PointIds->GetId(ptIds[i]));
            m_Edge->m_Points->SetPoint(i, this->m_Points->GetPoint(ptIds[i]));
        }
        return m_Edge.get();
    }

    Cell* GetFace(const int faceId) override {
        const igIndex* ptIds = nullptr;
        int numFacePoints = GetFacePointIds(faceId, ptIds);
        if (numFacePoints == 0) return nullptr;

        LagrangeFace* face = nullptr;
        // 棱柱体有两个三角形面和三个四边形面
        if (faceId < 2) { // 前两个面是三角形
            face = m_TriFace.get();
        } else { // 后三个面是四边形
            face = m_QuadFace.get();
        }

        face->SetOrder(m_Order);
        face->Reset();
        face->m_PointIds->SetNumberOfIds(numFacePoints);
        face->m_Points->SetNumberOfPoints(numFacePoints);

        for (int i = 0; i < numFacePoints; ++i) {
            face->m_PointIds->SetId(i, this->m_PointIds->GetId(ptIds[i]));
            face->m_Points->SetPoint(i, this->m_Points->GetPoint(ptIds[i]));
        }
        return face;
    }

    int GetEdgePointIds(const int edgeId, const igIndex*& ptIds) override {
        if (edgeId < 0 || edgeId >= 9) return 0;
        switch (m_Order) {
            case 1: { // 2-node linear edges
                static const igIndex edges[9][2] = {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5},
                                                    {5, 3}, {0, 3}, {1, 4}, {2, 5}};
                ptIds = edges[edgeId];
                return 2;
            }
            case 2: { // 3-node quadratic edges
                static const igIndex edges[9][3] = {{0, 1, 6},  {1, 2, 7},  {2, 0, 8},  {3, 4, 9}, {4, 5, 10},
                                                    {5, 3, 11}, {0, 3, 12}, {1, 4, 13}, {2, 5, 14}};
                ptIds = edges[edgeId];
                return 3;
            }
            case 3: { // 4-node cubic edges
                static const igIndex edges[9][4] = {{0, 1, 6, 7},   {1, 2, 8, 9},   {2, 0, 10, 11},
                                                    {3, 4, 12, 13}, {4, 5, 14, 15}, {5, 3, 16, 17},
                                                    {0, 3, 18, 19}, {1, 4, 20, 21}, {2, 5, 22, 23}};
                ptIds = edges[edgeId];
                return 4;
            }
            case 4: { // 5-node quartic edges
                static const igIndex edges[9][5] = {{0, 1, 6, 7, 8},    {1, 2, 9, 10, 11},  {2, 0, 12, 13, 14},
                                                    {3, 4, 15, 16, 17}, {4, 5, 18, 19, 20}, {5, 3, 21, 22, 23},
                                                    {0, 3, 24, 25, 26}, {1, 4, 27, 28, 29}, {2, 5, 30, 31, 32}};
                ptIds = edges[edgeId];
                return 5;
            }
            default:
                return 0;
        }
    }

    int GetFacePointIds(const int faceId, const igIndex*& ptIds) override {
        if (faceId < 0 || faceId >= 5) return 0;
        switch (m_Order) {
            case 2: {                               // 二次 (6或9节点)
                static const igIndex faces[5][9] = {// Face 0 (0-2-1): Edge 2(rev), Edge 1(rev), Edge 0(rev)
                                                    {0, 2, 1, 8, 7, 6, -1, -1, -1},
                                                    // Face 1 (3-4-5): Edge 6, Edge 7, Edge 8
                                                    {3, 4, 5, 9, 10, 11, -1, -1, -1},
                                                    // Face 2 (0-1-4-3): E0, E4, E6(rev), E3(rev)
                                                    {0, 1, 4, 3, 6, 13, 9, 12, 15},
                                                    // Face 3 (1-2-5-4): E1, E5, E7(rev), E4(rev)
                                                    {1, 2, 5, 4, 7, 14, 10, 13, 16},
                                                    // Face 4 (2-0-3-5): E2, E3, E8(rev), E5(rev)
                                                    {2, 0, 3, 5, 8, 12, 11, 14, 17}};
                ptIds = faces[faceId];
                return (faceId < 2) ? 6 : 9;
            }
            case 3: {                                // 三次 (10或16节点)
                static const igIndex faces[5][16] = {// Face 0 (0-2-1)
                                                     {0, 2, 1, 11, 10, 9, 8, 7, 6, 24, -1, -1, -1, -1, -1, -1},
                                                     // Face 1 (3-4-5)
                                                     {3, 4, 5, 12, 13, 14, 15, 16, 17, 25, -1, -1, -1, -1, -1, -1},
                                                     // Face 2 (0-1-4-3)
                                                     {0, 1, 4, 3, 6, 7, 20, 21, 13, 12, 19, 18, 26, 27, 28, 29},
                                                     // Face 3 (1-2-5-4)
                                                     {1, 2, 5, 4, 8, 9, 22, 23, 15, 14, 21, 20, 30, 31, 32, 33},
                                                     // Face 4 (2-0-3-5)
                                                     {2, 0, 3, 5, 10, 11, 18, 19, 17, 16, 23, 22, 34, 35, 36, 37}};
                ptIds = faces[faceId];
                return (faceId < 2) ? 10 : 16;
            }
            case 4: { // Order 4 (Prism 75 points)
                // Edges have 3 internal points.
                // Tri Face has 3 internal points.
                // Quad Face has 9 internal points.
                static const igIndex faces[5][25] = {
                        // Face 0 (0-2-1): E2(rev), E1(rev), E0(rev) + FaceNodes 33-35
                        // E2(12-14)->Rev(14,13,12); E1(9-11)->Rev(11,10,9); E0(6-8)->Rev(8,7,6)
                        {0, 2, 1, 14, 13, 12, 11, 10, 9, 8, 7, 6, 33, 34, 35, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},

                        // Face 1 (3-4-5): E3, E4, E5 + FaceNodes 36-38
                        // E3(15-17); E4(18-20); E5(21-23)
                        {3,  4,  5,  15, 16, 17, 18, 19, 20, 21, 22, 23, 36,
                         37, 38, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},

                        // Face 2 (0-1-4-3): E0, E7, E3(rev), E6(rev) + FaceNodes 39-47
                        // E0(6-8); E7(27-29); E3_rev(17,16,15); E6_rev(26,25,24)
                        {0, 1, 4, 3, 6, 7, 8, 27, 28, 29, 17, 16, 15, 26, 25, 24, 39, 40, 41, 42, 43, 44, 45, 46, 47},

                        // Face 3 (1-2-5-4): E1, E8, E4(rev), E7(rev) + FaceNodes 48-56
                        // E1(9-11); E8(30-32); E4_rev(20,19,18); E7_rev(29,28,27)
                        {1, 2, 5, 4, 9, 10, 11, 30, 31, 32, 20, 19, 18, 29, 28, 27, 48, 49, 50, 51, 52, 53, 54, 55, 56},

                        // Face 4 (2-0-3-5): E2, E6, E5(rev), E8(rev) + FaceNodes 57-65
                        // E2(12-14); E6(24-26); E5_rev(23,22,21); E8_rev(32,31,30)
                        {2,  0,  3,  5,  12, 13, 14, 24, 25, 26, 23, 22, 21,
                         32, 31, 30, 57, 58, 59, 60, 61, 62, 63, 64, 65}};
                ptIds = faces[faceId];
                return (faceId < 2) ? 15 : 25;
            }
            default:
                return 0;
        }
    }

protected:
    LagrangePrism() {
        m_Edge = LagrangeLine::New();
        m_TriFace = LagrangeTriangle::New();
        m_QuadFace = LagrangeQuadrilateral::New();
    }
    ~LagrangePrism() override = default;

     LagrangeLine::Pointer m_Edge;
     LagrangeTriangle::Pointer m_TriFace;
     LagrangeQuadrilateral::Pointer m_QuadFace;
   
};

IGAME_NAMESPACE_END
#endif
