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
            case 2: { // 二次 (14节点)
                static const igIndex faces[5][9] = {{0, 1, 2, 3, 5, 6, 7, 8, 13},
                                                    {0, 4, 1, 9, 10, 5, -1, -1, -1},
                                                    {1, 4, 2, 10, 11, 6, -1, -1, -1},
                                                    {2, 4, 3, 11, 12, 7, -1, -1, -1},
                                                    {3, 4, 0, 12, 9, 8, -1, -1, -1}};
                ptIds = faces[faceId];
                return (faceId == 0) ? 9 : 6;
            }
            case 3: { // 三次 (31节点)
                static const igIndex faces[5][16] = {{0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 21, 22, 23, 24},
                                                     {0, 4, 1, 13, 14, 15, 16, 21, 25, 22},
                                                     {1, 4, 2, 15, 16, 17, 18, 22, 26, 23},
                                                     {2, 4, 3, 17, 18, 19, 20, 23, 27, 24},
                                                     {3, 4, 0, 19, 20, 13, 14, 24, 28, 21}};
                ptIds = faces[faceId];
                return (faceId == 0) ? 16 : 10;
            }
            case 4: { // 四次 (55节点)
                static const igIndex faces[5][25] = {
                        {0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 29, 30, 31, 32, 33, 34, 35, 36, 37},
                        {0, 4, 1, 17, 18, 19, 20, 21, 22, 7, 6, 5, 38, 39, 40},
                        {1, 4, 2, 20, 21, 22, 23, 24, 25, 10, 9, 8, 41, 42, 43},
                        {2, 4, 3, 23, 24, 25, 26, 27, 28, 13, 12, 11, 44, 45, 46},
                        {3, 4, 0, 26, 27, 28, 17, 18, 19, 16, 15, 14, 47, 48, 49}};
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