#pragma once

#include <iGameFilter.h>
#include <iGameSurfaceMesh.h>

IGAME_NAMESPACE_BEGIN

// SubdivideFilter
// 对三角面做「线性 1-to-4 细分」：在每条边取中点，一个三角形分成四个小三角形。
//   - 旧顶点位置与点属性保持不变（不移动旧点）；
//   - 中点坐标 = 两端坐标平均，中点的点属性 = 两端属性平均；
//   - 相邻三角形共享边复用同一个中点（不会产生裂缝 / 重复点）；
//   - 单元属性（CellData）：4 个子三角形全部继承父面，细分不丢 CellData。
// 与 ParaView 的 Subdivide（vtkLinearSubdivisionFilter）一致：只加密网格、不改变形状。
// 非三角面先用三角形扇拆成三角形再细分；输出全部为三角形。
//
// 参数：
//   NumberOfSubdivisions 细分次数，每细分一次三角形数量 ×4。
class SubdivideFilter : public Filter {
public:
    I_OBJECT(SubdivideFilter);
    static Pointer New() { return new SubdivideFilter; }

    bool Execute() override;

    void SetNumberOfSubdivisions(int n);
    int GetNumberOfSubdivisions() const;

protected:
    SubdivideFilter();
    ~SubdivideFilter() override = default;

private:
    int m_NumberOfSubdivisions{1};
};

IGAME_NAMESPACE_END
