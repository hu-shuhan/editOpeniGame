#pragma once
#include <iGameBoundingBox.h>
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGamePoints.h>
#include <iGameUnstructuredMesh.h>
#include <vector>
#include <array>
IGAME_NAMESPACE_BEGIN
class ExtractCellsByRegionFilter : public Filter {
public:
    I_OBJECT(ExtractCellsByRegionFilter);
    static Pointer New() { return new ExtractCellsByRegionFilter; }

    void SetBox(const Vector3d& min, const Vector3d& max);
    void SetSphere(const Vector3d& center, double radius);
    void SetOrientedBox(const Vector3d& center, const Vector3d& halfSize,
                        const std::array<Vector3d, 3>& axes);
    // The closed half-space on the negative side of the normal is inside.
    void SetPlane(const Vector3d& origin, const Vector3d& normal);
    void SetRequireAllPoints(bool requireAllPoints);

    // Compute the selected input cell IDs without creating an output mesh.
    bool Preview();
    const std::vector<int>& GetSelectedCellIds() const { return m_Ids; }
    bool Execute() override;

protected:
    ExtractCellsByRegionFilter();
    ~ExtractCellsByRegionFilter() override = default;

private:
    bool IsPointInRegion(const Vector3d& p) const;
    void BuildOutputMesh();
    // 按 oldToNew（原网格点号 -> 输出网格点号）把输入的点/单元属性搬运到输出网格
    void CopyAttributeDataToOutput(const UnstructuredMesh::Pointer& outMesh, const std::vector<int>& oldToNew);

    /* 区域参数 */
    enum RegionType { BOX, SPHERE, ORIENTED_BOX, PLANE };
    RegionType m_RegionType{BOX};
    BoundingBox m_Box;                 // BOX 用（默认构造 = setNull）
    Vector3d m_Center{0.0, 0.0, 0.0};  // SPHERE 用（显式清零，Vector 默认构造不初始化）
    double m_Radius{0.0};              // SPHERE 用
    Vector3d m_HalfSize{0., 0., 0.};
    std::array<Vector3d, 3> m_BoxAxes;
    Vector3d m_Normal{1., 0., 0.};
    bool m_RequireAllPoints{true};     // true=严格（默认）；false=宽松

    /* 输入/输出 */
    UnstructuredMesh::Pointer m_Mesh;
    UnstructuredMesh::Pointer m_OutputMesh;
    std::vector<int> m_Ids;
};
IGAME_NAMESPACE_END
