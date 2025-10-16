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
            case 1: { // 线性 (6节点)
                static const igIndex faces[5][4] = {{0, 1, 2, -1},
                                                    {3, 5, 4, -1},
                                                    {0, 1, 4, 3},
                                                    {1, 2, 5, 4},
                                                    {2, 0, 3, 5}};
                ptIds = faces[faceId];
                return (faceId < 2) ? 3 : 4;
            }
            case 2: { // 二次 (18节点)
                static const igIndex faces[5][9] = {{0, 1, 2, 6, 7, 8, -1, -1, -1},
                                                    {3, 4, 5, 9, 10, 11, -1, -1, -1},
                                                    {0, 1, 4, 3, 6, 13, 9, 12, 15},
                                                    {1, 2, 5, 4, 7, 14, 10, 13, 16},
                                                    {2, 0, 3, 5, 8, 12, 11, 14, 17}};
                ptIds = faces[faceId];
                return (faceId < 2) ? 6 : 9;
            }
            case 3: { // 三次 (40节点)
                static const igIndex faces[5][16] = {{0, 1, 2, 6, 7, 8, 9, 10, 11, 36, -1, -1, -1, -1, -1, -1},
                                                     {3, 4, 5, 12, 13, 14, 15, 16, 17, 37, -1, -1, -1, -1, -1, -1},
                                                     {0, 1, 4, 3, 6, 7, 20, 21, 13, 12, 19, 18, 24, 25, 26, 27},
                                                     {1, 2, 5, 4, 8, 9, 22, 23, 15, 14, 21, 20, 28, 29, 30, 31},
                                                     {2, 0, 3, 5, 10, 11, 18, 19, 17, 16, 23, 22, 32, 33, 34, 35}};
                ptIds = faces[faceId];
                return (faceId < 2) ? 10 : 16;
            }
            case 4: { // 四次 (75节点)
                static const igIndex faces[5][25] = {
                        {0, 1, 2, 6, 7, 8, 9, 10, 11, 12, 13, 14, 60, 61, 62, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                        {3,  4,  5,  15, 16, 17, 18, 19, 20, 21, 22, 23, 63,
                         64, 65, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                        {0, 1, 4, 3, 6, 7, 8, 27, 28, 29, 17, 16, 15, 26, 25, 24, 33, 34, 35, 36, 37, 38, 39, 40, 41},
                        {1, 2, 5, 4, 9, 10, 11, 30, 31, 32, 19, 18, 17, 29, 28, 27, 42, 43, 44, 45, 46, 47, 48, 49, 50},
                        {2,  0,  3,  5,  12, 13, 14, 24, 25, 26, 23, 22, 21,
                         32, 31, 30, 51, 52, 53, 54, 55, 56, 57, 58, 59}};
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
