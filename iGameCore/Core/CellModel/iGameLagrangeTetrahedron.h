#ifndef iGameLagrangeTetrahedron_h
#define iGameLagrangeTetrahedron_h

#include "iGameLagrangeVolume.h"
#include "iGameLagrangeQuadrilateral.h"
#include "iGameLagrangeLine.h"

IGAME_NAMESPACE_BEGIN

class LagrangeTetrahedron : public LagrangeVolume {
public:
    I_OBJECT(LagrangeTetrahedron);
    static Pointer New() { return new LagrangeTetrahedron; }

    IGenum GetCellType() const noexcept override { return IG_LAGRANGE_TETRAHEDRON; }
    int GetNumberOfEdges() override { return 6; }
    int GetNumberOfFaces() override { return 4; }

    int GetNumberOfPoints() override {
        if (m_Order <= 0) return 0;
        return (m_Order + 1) * (m_Order + 2) * (m_Order + 3) / 6;
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

        // 四面体的面都是三角形
        LagrangeTriangle* face = m_TriFace.get();

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
        if (edgeId < 0 || edgeId >= 6) return 0;
        switch (m_Order) {
            case 1: { // 线性 (2节点) 边
                static const igIndex edges[6][2] = {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}};
                ptIds = edges[edgeId];
                return 2;
            }
            case 2: { // 二次 (3节点) 边
                // 节点顺序: [角点1, 角点2, 边中点]
                static const igIndex edges[6][3] = {{0, 1, 4}, {1, 2, 5}, {2, 0, 6}, {0, 3, 7}, {1, 3, 8}, {2, 3, 9}};
                ptIds = edges[edgeId];
                return 3;
            }
            case 3: { // 三次 (4节点) 边
                // 节点顺序: [角点1, 角点2, 边内部点1, 边内部点2]
                static const igIndex edges[6][4] = {{0, 1, 4, 5},   {1, 2, 6, 7},   {2, 0, 8, 9},
                                                    {0, 3, 10, 11}, {1, 3, 12, 13}, {2, 3, 14, 15}};
                ptIds = edges[edgeId];
                return 4;
            }
            case 4: { // 四次 (5节点) 边
                // 节点顺序: [角点1, 角点2, 边内部点1, 边内部点2, 边内部点3]
                static const igIndex edges[6][5] = {{0, 1, 4, 5, 6},    {1, 2, 7, 8, 9},    {2, 0, 10, 11, 12},
                                                    {0, 3, 13, 14, 15}, {1, 3, 16, 17, 18}, {2, 3, 19, 20, 21}};
                ptIds = edges[edgeId];
                return 5;
            }
            default:
                return 0;
        }
    }

    int GetFacePointIds(const int faceId, const igIndex*& ptIds) override {
        if (faceId < 0 || faceId >= 4) return 0;
        switch (m_Order) {
            case 1: { // 线性面 (3节点)
                static const igIndex linearFaces[4][3] = {{0, 1, 2}, {0, 3, 1}, {1, 3, 2}, {2, 3, 0}};
                ptIds = linearFaces[faceId];
                return 3;
            }
            case 2: { // 二次面 (6节点)
                static const igIndex quadraticFaces[4][6] = {{0, 1, 2, 4, 5, 6},
                                                             {0, 3, 1, 7, 8, 4},
                                                             {1, 3, 2, 8, 9, 5},
                                                             {2, 3, 0, 9, 7, 6}};
                ptIds = quadraticFaces[faceId];
                return 6;
            }
            case 3: { // 三次面 (10节点)
                static const igIndex cubicFaces[4][10] = {{0, 1, 2, 4, 5, 6, 7, 10, 11, 12},
                                                          {0, 3, 1, 8, 9, 4, 5, 13, 14, 10},
                                                          {1, 3, 2, 9, 6, 8, 7, 15, 16, 11},
                                                          {2, 3, 0, 7, 9, 5, 6, 17, 18, 12}};
                ptIds = cubicFaces[faceId];
                return 10;
            }
            case 4: { // 四次面 (15节点)
                static const igIndex quarticFaces[4][15] = {{0, 1, 2, 4, 5, 6, 7, 8, 12, 13, 14, 15, 21, 22, 23},
                                                            {0, 3, 1, 9, 10, 4, 5, 11, 16, 17, 12, 13, 24, 25, 21},
                                                            {1, 3, 2, 10, 6, 9, 8, 11, 18, 19, 14, 15, 26, 27, 22},
                                                            {2, 3, 0, 11, 7, 10, 9, 5, 20, 16, 17, 18, 28, 29, 23}};
                ptIds = quarticFaces[faceId];
                return 15;
            }
            default:
                return 0;
        }
    }

protected:
    LagrangeTetrahedron()
    {
        m_Edge = LagrangeLine::New();
        m_TriFace = LagrangeTriangle::New();
    }
    ~LagrangeTetrahedron() override = default;

private:
    LagrangeLine::Pointer m_Edge;
    LagrangeTriangle::Pointer m_TriFace;
};

IGAME_NAMESPACE_END
#endif