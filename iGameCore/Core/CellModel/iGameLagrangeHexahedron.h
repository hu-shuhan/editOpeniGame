#ifndef iGameLagrangeHexahedron_h
#define iGameLagrangeHexahedron_h

#include "iGameLagrangeLine.h"
#include "iGameLagrangeQuadrilateral.h"
#include "iGameLagrangeVolume.h"
#include <vector>

IGAME_NAMESPACE_BEGIN

class LagrangeHexahedron : public LagrangeVolume {
public:
    I_OBJECT(LagrangeHexahedron);
    static Pointer New() { return new LagrangeHexahedron; }

    IGenum GetCellType() const noexcept override { return IG_LAGRANGE_HEXAHEDRON; }
    int GetNumberOfEdges() override { return 12; }
    int GetNumberOfFaces() override { return 6; }

    int GetNumberOfPoints() override {
        if (m_Order <= 0) return 0;
        return (m_Order + 1) * (m_Order + 1) * (m_Order + 1);
    }

    // GetEdge 和 GetFace 的函数体定义在文件末尾
    Cell* GetEdge(const int edgeId) override;
    Cell* GetFace(const int faceId) override;

    int GetEdgePointIds(const int edgeId, const igIndex*& ptIds) override {
        if (edgeId < 0 || edgeId >= 12) return 0;
        switch (m_Order) {
            case 1: { // 线性 (2节点) 边
                static const igIndex edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                                     {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
                ptIds = edges[edgeId];
                return 2;
            }
            case 2: { // 二次 (3节点) 边
                static const igIndex edges[12][3] = {{0, 1, 8},  {1, 2, 9},  {2, 3, 10}, {3, 0, 11},
                                                     {4, 5, 12}, {5, 6, 13}, {6, 7, 14}, {7, 4, 15},
                                                     {0, 4, 16}, {1, 5, 17}, {2, 6, 18}, {3, 7, 19}};
                ptIds = edges[edgeId];
                return 3;
            }
            case 3: { // 三次 (4节点) 边
                static const igIndex edges[12][4] = {{0, 1, 8, 9},   {1, 2, 10, 11}, {2, 3, 12, 13}, {3, 0, 14, 15},
                                                     {4, 5, 16, 17}, {5, 6, 18, 19}, {6, 7, 20, 21}, {7, 4, 22, 23},
                                                     {0, 4, 24, 25}, {1, 5, 26, 27}, {2, 6, 28, 29}, {3, 7, 30, 31}};
                ptIds = edges[edgeId];
                return 4;
            }
            case 4: { // 四次 (5节点) 边
                static const igIndex edges[12][5] = {{0, 1, 8, 9, 10},   {1, 2, 11, 12, 13}, {2, 3, 14, 15, 16},
                                                     {3, 0, 17, 18, 19}, {4, 5, 20, 21, 22}, {5, 6, 23, 24, 25},
                                                     {6, 7, 26, 27, 28}, {7, 4, 29, 30, 31}, {0, 4, 32, 33, 34},
                                                     {1, 5, 35, 36, 37}, {2, 6, 38, 39, 40}, {3, 7, 41, 42, 43}};
                ptIds = edges[edgeId];
                return 5;
            }
            default:
                return 0;
        }
    }

    int GetFacePointIds(const int faceId, const igIndex*& ptIds) override {
        if (faceId < 0 || faceId >= 6) return 0;
        switch (m_Order) {
            case 1: { // 线性 (4节点面)
                static const igIndex faces[6][4] = {{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4},
                                                    {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
                ptIds = faces[faceId];
                return 4;
            }
            case 2: { // 二次 (9节点面), 纯拉格朗日 (27节点体)
                // 节点顺序: [4角点, 4边中点, 1面心点]
                // 节点编号: 角(0-7), 边(8-19), 面(20-25), 体(26)
                static const igIndex faces[6][9] = {
                        {0, 1, 2, 3, 8, 9, 10, 11, 24},   
                        {4, 5, 6, 7, 12, 13, 14, 15, 25}, 
                        {0, 1, 5, 4, 8, 17, 12, 16, 22},  
                        {1, 2, 6, 5, 9, 19, 13, 17, 21},  
                        {2, 3, 7, 6, 10, 18, 14, 19, 23}, 
                        {3, 0, 4, 7, 11, 16, 15, 18, 20}  
                };
                ptIds = faces[faceId];
                return 9;
            }
            case 3: { // 三次 (16节点面), 纯拉格朗日 (64节点体)
                // 节点顺序: [4角点, 8边节点(每边2个), 4面内节点]
                // 节点编号: 角(0-7), 边(8-31), 面(32-55), 体(56-63)
                static const igIndex faces[6][16] = {
                        {0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15, 52, 53, 54, 55},   // Bottom
                        {4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 56, 57, 58, 59}, // Top
                        {0, 1, 5, 4, 8, 9, 26, 27, 17, 16, 24, 25, 48, 49, 50, 51},   // Front
                        {1, 2, 6, 5, 10, 11, 28, 29, 19, 18, 26, 27, 44, 45, 46, 47}, // Right
                        {2, 3, 7, 6, 12, 13, 30, 31, 21, 20, 28, 29, 40, 41, 42, 43}, // Back
                        {3, 0, 4, 7, 14, 15, 24, 25, 23, 22, 30, 31, 36, 37, 38, 39}  // Left
                };
                ptIds = faces[faceId];
                return 16;
            }
            case 4: { // 四次 (25节点面), 纯拉格朗日 (125节点体)
                // 节点顺序: [4角点, 12边节点(每边3个), 9面内节点]
                // 节点编号: 角(0-7), 边(8-43), 面(44-97), 体(98-124)
                static const igIndex faces[6][25] = {
                        {0,  1,  2,  3,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                         17, 18, 19, 89, 90, 91, 92, 93, 94, 95, 96, 97}, // Bottom
                        {4,  5,  6,  7,  20, 21, 22, 23, 24, 25, 26, 27, 28,
                         29, 30, 31, 80, 81, 82, 83, 84, 85, 86, 87, 88}, // Top
                        {0,  1,  5,  4,  8,  9,  10, 35, 36, 37, 22, 21, 20,
                         32, 33, 34, 62, 63, 64, 65, 66, 67, 68, 69, 70}, // Front
                        {1,  2,  6,  5,  11, 12, 13, 38, 39, 40, 25, 24, 23,
                         35, 36, 37, 71, 72, 73, 74, 75, 76, 77, 78, 79}, // Right
                        {2,  3,  7,  6,  14, 15, 16, 41, 42, 43, 28, 27, 26,
                         29, 30, 31, 53, 54, 55, 56, 57, 58, 59, 60, 61}, // Back
                        {3,  0,  4,  7,  17, 18, 19, 32, 33, 34, 31, 30, 29,
                         41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52} // Left
                };
                ptIds = faces[faceId];
                return 25;
            }
            default:
                return 0;
        }
    }

protected:
    LagrangeHexahedron() {
        m_Edge = LagrangeLine::New();
        m_QuadFace = LagrangeQuadrilateral::New();
    }
    ~LagrangeHexahedron() override = default;

private:
    LagrangeLine::Pointer m_Edge;
    LagrangeQuadrilateral::Pointer m_QuadFace;
};


// --- GetEdge 和 GetFace 的函数体定义 ---
inline Cell* LagrangeHexahedron::GetEdge(const int edgeId) {
    const igIndex* ptIds = nullptr;
    int numEdgePoints = GetEdgePointIds(edgeId, ptIds);
    if (numEdgePoints == 0) return nullptr;
    m_Edge->SetOrder(m_Order);
    m_Edge->Reset();
    m_Edge->m_PointIds->SetNumberOfIds(numEdgePoints);
    m_Edge->m_Points->SetNumberOfPoints(numEdgePoints);
    for (int i = 0; i < numEdgePoints; ++i) {
        m_Edge->m_PointIds->SetId(i, this->m_PointIds->GetId(ptIds[i]));
        m_Edge->m_Points->SetPoint(i, this->m_Points->GetPoint(ptIds[i]));
    }
    return m_Edge.get();
}

inline Cell* LagrangeHexahedron::GetFace(const int faceId) {
    const igIndex* ptIds = nullptr;
    int numFacePoints = GetFacePointIds(faceId, ptIds);
    if (numFacePoints == 0) return nullptr;
    LagrangeQuadrilateral* face = m_QuadFace.get();
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

IGAME_NAMESPACE_END
#endif