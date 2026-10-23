#ifndef iGameSurfaceVectorsFilter_h
#define iGameSurfaceVectorsFilter_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameArrayObject.h"

#include <string>
#include <vector>
#include <array>

IGAME_NAMESPACE_BEGIN

class UnstructuredMesh;

class SurfaceVectorsFilter : public Filter {
public:
    I_OBJECT(SurfaceVectorsFilter);
    static Pointer New() { return new SurfaceVectorsFilter; }

    ~SurfaceVectorsFilter() override = default;

    bool Execute() override;

    void SetVectorAttributeByName(const std::string& name) { m_VectorAttrName = name; }
    void SetVectorAttributeByIndex(int index) { m_VectorAttrIndex = index; }

    // ConstraintMode: 与 ParaView vtkSurfaceVectors 对齐
    //   Parallel           - v' = v - (v·n)·n  去除法向分量，保留切向 (默认)
    //   Perpendicular      - v' = (v·n)·n      仅保留法向分量
    //   PerpendicularScale - 输出标量 = v·n (法向投影长度)
    // 在独立输出模型中替换同名数组，输入模型及属性不变。
    // 输出模型中 Velocity 的值就是投影结果，不新增其他名称的数组。
    enum ConstraintMode { Parallel = 0, Perpendicular = 1, PerpendicularScale = 2 };
    void SetConstraintMode(ConstraintMode m) { m_ConstraintMode = m; }
    ConstraintMode GetConstraintMode() const { return m_ConstraintMode; }
    void SetConstraintModeToParallel() { m_ConstraintMode = Parallel; }
    void SetConstraintModeToPerpendicular() { m_ConstraintMode = Perpendicular; }
    void SetConstraintModeToPerpendicularScale() { m_ConstraintMode = PerpendicularScale; }

    std::string GetMessage() const { return m_Message; }

protected:
    SurfaceVectorsFilter();

private:
    static std::array<double, 3> ComputeFaceNormal(SurfaceMesh::Pointer mesh, igIndex faceId);
    static bool ComputePointNormals(SurfaceMesh::Pointer mesh,
                                    std::vector<std::array<double, 3>>& outNormals);
    static void Normalize(std::array<double, 3>& v);

    // 环绕一致性传播（对齐 VTK vtkPolyDataNormals 的 Consistency 行为）：
    // BFS 遍历面邻接关系，相邻面在共享边上的顶点方向若相同则环绕不一致，标记翻转。
    // 返回每个面是否需要取反法向。网格环绕不一致时（如分块三角化的球面极点带），
    // 不修正会让反向面法向抵消正向面，点/单元法向被严重拉偏（实测达 16~26 度）。
    static std::vector<char> ComputeConsistencyFlips(SurfaceMesh::Pointer mesh);

    std::string   m_VectorAttrName;
    int           m_VectorAttrIndex{-1};
    ConstraintMode m_ConstraintMode{Parallel};
    std::string   m_Message;
};

IGAME_NAMESPACE_END
#endif
