# Generate Surface Tangents 静态测试

三个文件均为 legacy ASCII VTK POLYDATA，全部使用三角形，已自带点法向量 Normals 和活动纹理坐标 TCoords。
直接打开文件 → Apply → Surface Tangents（旧版本叫 Generate Surface Tangents）。
不需要 Texture Map to Plane、Extract Surface、Triangulate 或 Generate Surface Normals；重复映射会改变本测试的 UV。

## 01_planar_reference.vtk：精确数值基准（推荐先测）

包含四块独立平面，共 468 点、768 个三角形。每块 117 点、192 个三角形。

| PointID 范围 | 预期点 Tangents | 预期模长 |
|---|---|---|
| 0～116 | (1,0,0) | 1 |
| 117～233 | (0,1,0) | 1 |
| 234～350 | (-1,0,0) | 1 |
| 351～467 | (0.6,0.8,0) | 1 |

ExpectedTangents 数组提供逐点参考值。检查 Spreadsheet View 的 Tangents 三个分量，建议误差容限 1e-5。
输出应无 NaN、Inf、零切向量；网格规模应保持不变。
可用 Calculator 表达式 `mag(Tangents-ExpectedTangents)`，误差应接近 0。
也可分别按 Tangents 的 X、Y 分量着色，手动固定范围为 [-1,1]。
按 Magnitude 着色时固定范围 [0,1]，四块应有相同的映射颜色（光照可能造成亮暗差异）。
这里测试的是默认点切向量，不要求未归一化的单元切向量与 ExpectedTangents 相等。

## 02_cylinder_valid_uv.vtk：曲面与 UV 接缝

包含圆柱侧面，1105 点、2048 个三角形，没有端面。UV 沿圆周与轴向变化，不存在退化 UV 三角形。
接缝两侧几何位置相同，但点编号不同，分别对应 u=0 和 u=1，避免 UV 从 1 跳回 0。
检查切向量有限、模长接近 1，方向大体沿圆周正向。
解析曲面方向为 (-sin(theta),cos(theta),0)，离散三角形与点平均会带来角度误差；不能按解析方向逐点零误差验收。
尤其接缝为未焊接边界，邻接三角形不同，两侧结果可能略有差别。
若两软件结果不同，需要进一步统一三角连接、点法向正交化和累加规则，不能只比较颜色。

## 03_degenerate_uv.vtk：异常处理

与基准相同的几何，将第二块（PointID 117～233）的 UV 压成一条线，故无法定义有效二维纹理切向基。
检查其他三块仍有效，同时记录第二块的处理：NaN、零向量、警告或失败。
当前 iGameVis 代码跳过退化三角形，该独立块预期为零向量。
ParaView 的具体行为需实测；若显示 NaN 特殊颜色，不应把它当作有效方向。
不要要求两个软件在异常输入上采用同样的兜底数值，也不要把零向量解释为有效单位切向量。

## 验证范围

`python generate.py` 重建文件，并逐个验证基准三角形从几何和 UV 算出的方向。
已进行生成器解析公式和连接范围检查；未在 ParaView/iGameVis 中实际执行。
