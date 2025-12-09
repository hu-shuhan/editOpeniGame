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

            case 3: { // 3阶 (16节点)

                static const igIndex faces[6][16] = {// Face 0: Bottom (Z=0) - 正常

                                                     {0, 3, 2, 1, 15, 14, 13, 12, 11, 10, 9, 8, 48, 50, 51, 49},


                                                     // Face 1: Top (Z=1) - 正常

                                                     {4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 52, 53, 54, 55},


                                                     // Face 2: Front (Y=0) - 正常

                                                     {0, 1, 5, 4, 8, 9, 26, 27, 17, 16, 25, 24, 40, 41, 43, 42},


                                                     // Face 3: Right (X=1) - 正常 (Block 2)

                                                     {1, 2, 6, 5, 10, 11, 30, 31, 19, 18, 27, 26, 36, 37, 39, 38},


                                                     // Face 4: Back (Y=1) - 【已验证需翻转】 (Block 4)

                                                     // 原序 44,45 -> 改为 45,44 (X: 0.33, 0.67)

                                                     {2, 3, 7, 6, 12, 13, 28, 29, 21, 20, 31, 30, 44, 45, 47, 46},


                                                     // Face 5: Left (X=0) - 【推测需翻转】 (Block 1)

                                                     // 原序 32,33 -> 改为 33,32 (Y: 0.33, 0.67)

                                                     {3, 0, 4, 7, 14, 15, 24, 25, 23, 22, 29, 28, 33, 32, 34, 35}};

                ptIds = faces[faceId];

                return 16;
            }

            case 4: { // Fourth Order (25 nodes per face)
                static const igIndex faces[6][25] = {
                        // Face 0: Bottom (Z=0)
                        // Edges: 0, 1, 2, 3 (All Forward)
                        // Internals: Block 4 [80-88]
                        // Permutation: Column-Major (Transposed) to match Ord3's {0,3,2,1} pattern
                        // 0,3,6, 1,4,7, 2,5,8 -> 80,83,86, 81,84,87, 82,85,88
                        {0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 80, 83, 86, 81, 84, 87, 82, 85, 88},

                        // Face 1: Top (Z=1)
                        // Edges: 4, 5, 6, 7 (All Forward)
                        // Internals: Block 5 [89-97]
                        // Permutation: Standard Row-Major (Match Ord3)
                        {4,  5,  6,  7,  20, 21, 22, 23, 24, 25, 26, 27, 28,
                         29, 30, 31, 89, 90, 91, 92, 93, 94, 95, 96, 97},

                        // Face 2: Front (Y=0)
                        // Edges: 0(F), 9(F), 4(Rev), 8(Rev)
                        // Internals: Block 2 [62-70]
                        // Permutation: Standard Row-Major
                        {0, 1, 5, 4, 8, 9, 10, 35, 36, 37, 22, 21, 20, 34, 33, 32, 62, 63, 64, 65, 66, 67, 68, 69, 70},

                        // Face 3: Right (X=1)
                        // Edges: 1(F), 10(F), 5(Rev), 9(Rev)
                        // Internals: Block 1 [53-61]
                        // Permutation: Standard Row-Major
                        // Note: E10 indices (41-43) are HIGHER than E11 (38-40) in your generation logic
                        {1,  2,  6,  5,  11, 12, 13, 41, 42, 43, 25, 24, 23,
                         37, 36, 35, 53, 54, 55, 56, 57, 58, 59, 60, 61},

                        // Face 4: Back (Y=1)
                        // Edges: 2(F), 11(F), 6(Rev), 10(Rev)
                        // Internals: Block 3 [71-79]
                        // Permutation: Row Reversal (2,1,0...)
                        // 73,72,71, 76,75,74, 79,78,77
                        {2,  3,  7,  6,  14, 15, 16, 38, 39, 40, 28, 27, 26,
                         43, 42, 41, 73, 72, 71, 76, 75, 74, 79, 78, 77},

                        // Face 5: Left (X=0)
                        // Edges: 3(F), 8(F), 7(Rev), 11(Rev)
                        // Internals: Block 0 [44-52]
                        // Permutation: Row Reversal (2,1,0...)
                        // 46,45,44, 49,48,47, 52,51,50
                        {3,  0,  4,  7,  17, 18, 19, 32, 33, 34, 31, 30, 29,
                         40, 39, 38, 46, 45, 44, 49, 48, 47, 52, 51, 50}};
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