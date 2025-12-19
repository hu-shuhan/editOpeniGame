#ifndef iGameLagrangePyramid_h
#define iGameLagrangePyramid_h

#include "iGameLagrangeQuadrilateral.h"
#include "iGameLagrangeTriangle.h"
#include "iGameLagrangeVolume.h"
#include "iGameLagrangeLine.h"

IGAME_NAMESPACE_BEGIN

class LagrangePyramid : public LagrangeVolume {
public:
    I_OBJECT(LagrangePyramid);
    static Pointer New() { return new LagrangePyramid; }

    IGenum GetCellType() const noexcept override { return IG_LAGRANGE_PYRAMID; }
    int GetNumberOfEdges() override { return 8; }
    int GetNumberOfFaces() override { return 5; }

    int GetNumberOfPoints() override {
        if (m_Order <= 0) return 0;
        // 公式: N = (p+1)(p+2)(2p+3)/6
        return (m_Order + 1) * (m_Order + 2) * (2 * m_Order + 3) / 6;
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
        if (faceId == 0) { // 第一个面是四边形底面
            face = m_QuadFace.get();
        } else { // 后四个面是三角形侧面
            face = m_TriFace.get();
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
        if (edgeId < 0 || edgeId >= 8) return 0;
        switch (m_Order) {
            case 1: { // 2-node linear edges
                static const igIndex edges[8][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 4}, {2, 4}, {3, 4}};
                ptIds = edges[edgeId];
                return 2;
            }
            case 2: { // 3-node quadratic edges
                static const igIndex edges[8][3] = {{0, 1, 5}, {1, 2, 6},  {2, 3, 7},  {3, 0, 8},
                                                    {0, 4, 9}, {1, 4, 10}, {2, 4, 11}, {3, 4, 12}};
                ptIds = edges[edgeId];
                return 3;
            }
            case 3: { // 4-node cubic edges
                static const igIndex edges[8][4] = {{0, 1, 5, 6},   {1, 2, 7, 8},   {2, 3, 9, 10},  {3, 0, 11, 12},
                                                    {0, 4, 13, 14}, {1, 4, 15, 16}, {2, 4, 17, 18}, {3, 4, 19, 20}};
                ptIds = edges[edgeId];
                return 4;
            }
            case 4: { // 5-node quartic edges
                static const igIndex edges[8][5] = {{0, 1, 5, 6, 7},    {1, 2, 8, 9, 10},   {2, 3, 11, 12, 13},
                                                    {3, 0, 14, 15, 16}, {0, 4, 17, 18, 19}, {1, 4, 20, 21, 22},
                                                    {2, 4, 23, 24, 25}, {3, 4, 26, 27, 28}};
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
            case 1: { // 线性 (5节点)
                static const igIndex faces[5][4] = {{0, 1, 2, 3},
                                                    {0, 4, 1, -1},
                                                    {1, 4, 2, -1},
                                                    {2, 4, 3, -1},
                                                    {3, 4, 0, -1}};
                ptIds = faces[faceId];
                return (faceId == 0) ? 4 : 3;
            }
            case 2: { // Order 2 (14 points: 5 corners, 8 edges, 1 face)
                static const igIndex faces[5][9] = {
                        // Face 0 (Base 0-3-2-1): E3(rev), E2(rev), E1(rev), E0(rev) + FaceNode 13
                        // E3(3-0) is pt 8 -> Rev is invalid for pt? No, pt is symmetric.
                        // Points: 0, 3, 2, 1, 8, 7, 6, 5, 13
                        {0, 3, 2, 1, 8, 7, 6, 5, 13},

                        // Face 1 (0-1-4): E0, E5, E4(rev)
                        {0, 1, 4, 5, 10, 9, -1, -1, -1},

                        // Face 2 (1-2-4): E1, E6, E5(rev)
                        {1, 2, 4, 6, 11, 10, -1, -1, -1},

                        // Face 3 (2-3-4): E2, E7, E6(rev)
                        {2, 3, 4, 7, 12, 11, -1, -1, -1},

                        // Face 4 (3-0-4): E3, E4, E7(rev)
                        {3, 0, 4, 8, 9, 12, -1, -1, -1}};
                ptIds = faces[faceId];
                return (faceId == 0) ? 9 : 6;
            }
            case 3: { // Order 3 (30 points)
                // Edges: 2 pts. Quad Face: 4 pts. Tri Face: 1 pt.
                // Edge Offsets: E0(5,6), E1(7,8), E2(9,10), E3(11,12)
                //               E4(13,14), E5(15,16), E6(17,18), E7(19,20)
                static const igIndex faces[5][16] = {
                        // Face 0 (0-3-2-1): E3(rev), E2(rev), E1(rev), E0(rev) + FaceNodes 21-24
                        // E3(11,12)->Rev(12,11); E2(9,10)->Rev(10,9); E1(7,8)->Rev(8,7); E0(5,6)->Rev(6,5)
                        {0, 3, 2, 1, 12, 11, 10, 9, 8, 7, 6, 5, 21, 22, 23, 24},

                        // Face 1 (0-1-4): E0, E5, E4(rev) + FaceNode 25
                        // E0(5,6); E5(15,16); E4_rev(14,13)
                        {0, 1, 4, 5, 6, 15, 16, 14, 13, 25, -1, -1, -1, -1, -1, -1},

                        // Face 2 (1-2-4): E1, E6, E5(rev) + FaceNode 26
                        // E1(7,8); E6(17,18); E5_rev(16,15)
                        {1, 2, 4, 7, 8, 17, 18, 16, 15, 26, -1, -1, -1, -1, -1, -1},

                        // Face 3 (2-3-4): E2, E7, E6(rev) + FaceNode 27
                        // E2(9,10); E7(19,20); E6_rev(18,17)
                        {2, 3, 4, 9, 10, 19, 20, 18, 17, 27, -1, -1, -1, -1, -1, -1},

                        // Face 4 (3-0-4): E3, E4, E7(rev) + FaceNode 28
                        // E3(11,12); E4(13,14); E7_rev(20,19)
                        {3, 0, 4, 11, 12, 13, 14, 20, 19, 28, -1, -1, -1, -1, -1, -1}};
                ptIds = faces[faceId];
                return (faceId == 0) ? 16 : 10;
            }
            case 4: { // Order 4 (55 points)
                // Edges: 3 pts. Quad Face: 9 pts. Tri Face: 3 pts.
                // Edge Offsets: E0(5,6,7)... E7(26,27,28)
                static const igIndex faces[5][25] = {
                        // Face 0 (0-3-2-1): E3(rev), E2(rev), E1(rev), E0(rev) + FaceNodes 29-37
                        // E3(14-16)->Rev(16,15,14); E2(11-13)->Rev(13,12,11)
                        // E1(8-10)->Rev(10,9,8); E0(5-7)->Rev(7,6,5)
                        {0, 3,  2,  1,  16, 15, 14, 13, 12, 11, 10, 9,  8,  7, 6,
                         5, 29, 30, 31, 32, 33, 34, 35, 36, 37}, // Quad padding if needed

                        // Face 1 (0-1-4): E0, E5, E4(rev) + FaceNodes 38-40
                        // E0(5,6,7); E5(20,21,22); E4(17,18,19)->Rev(19,18,17)
                        {0, 1, 4, 5, 6, 7, 20, 21, 22, 19, 18, 17, 38, 39, 40, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},

                        // Face 2 (1-2-4): E1, E6, E5(rev) + FaceNodes 41-43
                        // E1(8,9,10); E6(23,24,25); E5_rev(22,21,20)
                        {1, 2, 4, 8, 9, 10, 23, 24, 25, 22, 21, 20, 41, 42, 43, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},

                        // Face 3 (2-3-4): E2, E7, E6(rev) + FaceNodes 44-46
                        // E2(11,12,13); E7(26,27,28); E6_rev(25,24,23)
                        {2,  3,  4,  11, 12, 13, 26, 27, 28, 25, 24, 23, 44,
                         45, 46, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},

                        // Face 4 (3-0-4): E3, E4, E7(rev) + FaceNodes 47-49
                        // E3(14,15,16); E4(17,18,19); E7_rev(28,27,26)
                        {3,  0,  4,  14, 15, 16, 17, 18, 19, 28, 27, 26, 47,
                         48, 49, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};
                ptIds = faces[faceId];
                return (faceId == 0) ? 25 : 15;
            }
            default:
                return 0;
        }
    }

protected:
    LagrangePyramid() {
        m_Edge = LagrangeLine::New();
        m_TriFace = LagrangeTriangle::New();
        m_QuadFace = LagrangeQuadrilateral::New();
    }
    ~LagrangePyramid() override = default;

    LagrangeLine::Pointer m_Edge;
    LagrangeTriangle::Pointer m_TriFace;
    LagrangeQuadrilateral::Pointer m_QuadFace;
};

IGAME_NAMESPACE_END
#endif