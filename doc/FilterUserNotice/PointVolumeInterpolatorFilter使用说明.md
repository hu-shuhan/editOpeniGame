# PointVolumeInterpolator 使用说明

点体积插值（Point Volume Interpolator）过滤器：把输入点云（或任意数据集）上的**点属性**，按**核函数**插值到一个**规则体网格**（StructuredMesh）的格点上。语义对齐 ParaView 的同名过滤器（`vtkPointInterpolator`），只需点、不需要单元连接关系。

## 一、功能

- 输入：单个数据集，**用它的点作为插值样本**（点云、网格都可以，单元被忽略），其所有 `POINT` 属性都会被插值。
- 输出：规则体网格 `StructuredMesh`（iGame 中相当于 `vtkImageData`），覆盖输入包围盒（或指定区域），携带插值后的点属性与有效点掩码 `vtkValidPointMask`（1=有效，0=空点）。
- 邻域检索：KD-tree（nanoflann），支持两种取邻域方式：
  - `Radius`：半径内的所有点；
  - `N Closest`：最近的 N 个点。
- 核函数（权重按总和归一化）：
  | 核 | 权重 | 说明 |
  | --- | --- | --- |
  | Voronoi | 1（最近点） | 默认；重合点精确还原 |
  | Gaussian 高斯 | `exp(-(Sharpness * r / Radius)^2)` | 锐度默认 2.0 |
  | Shepard 逆距离 | `1 / r^PowerParameter` | 幂默认 2.0 |
  | Linear 等权 | 1（basis 内等权平均） | 最简单 |
- **未实现**：`EllipsoidalGaussian`（各向异性椭球高斯，需逐点法向/标量）；SPH 系列（粒子/流体专用，VTK 亦注明非通用插值）；`ProbabilisticVoronoi`（无概率数组时与 Voronoi 完全相同、ParaView 界面也不暴露概率数组，属冗余选项）。本工具聚焦通用网格/点云插值，故不提供这三类。
- 空邻域按 `NullPointsStrategy` 处理：Mask Points（写 NullValue、掩码 0）/ Null Value（写 NullValue，默认）/ Closest Point（退化为最近点）。

## 二、界面入口

主界面 →【算法处理】菜单 → **点体积插值 (Point Volume Interpolator)**，面板参数：

| 控件 | 含义 | 默认 |
| --- | --- | --- |
| 核函数-类型 | Voronoi / Gaussian / Shepard / Linear | Voronoi |
| 核函数-邻域 | Radius / N Closest | Radius |
| 半径 (Radius) | 邻域半径 | 1.0 |
| 近邻数 (N Closest) | 最近邻个数 | 8 |
| 高斯锐度 (Sharpness) | 高斯衰减 | 2.0 |
| Shepard 幂 (Power) | 逆距离指数 | 2.0 |
| 空邻域策略 | Mask Points / Null Value / Closest Point | Null Value |
| 空值 (NullValue) | 空点写入值 | 0 |
| 使用输入包围盒 | 输出网格覆盖输入包围盒 | 勾选 |
| 分辨率 X/Y/Z | 输出体网格格点数 | 64 / 64 / 64 |
| 采样包围盒 X/Y/Z min/max | 关闭"使用输入包围盒"后的采样范围（自动填入当前模型包围盒） | 输入包围盒 |
| 插值数组（勾选） | 只插值勾选的点属性数组 | 全部勾选 |

点击【执行】后，输出体网格会加入模型树；结果区显示网格维度、点数与命中点数。

## 三、参数与注意事项

- 只有**点属性（PointData）**会被插值；`CELL` 属性不参与。输出属性为 `FloatArray`，维度与源属性一致。
- 输入点云可以没有单元（例如 `POLYDATA + VERTICES`），过滤器只用点。
- `KernelFootprint = N Closest` 时 `Radius` 不起作用；`Radius` 缺省 1.0，需按数据尺度调整。
- 空点（半径内无样本）的默认输出是 `NullValue` 且掩码为 0；如需"总取最近点"，选 `Closest Point`。
- **对应 ParaView**：在 ParaView 里就是 Properties 面板的 **`Kernel` 下拉**直接选核；本实现的参数与之一一对应，便于做效果对比。
- **精确命中**：Voronoi 取最近点（重合即精确）；Shepard 在 d=0 时取该点；Gaussian / Linear 按公式自然计算（与 VTK 一致，不再做全局"重合点短路"）。
- **分辨率保护**：GUI 里格点数超过 2,000,000 会弹确认框；内核硬上限 100,000,000（超过直接失败）。
- **数组选择**：可只勾选部分点属性进行插值（全不勾会提示）；未勾选的原数组不会出现在输出里。
- **分辨率**：ParaView 的 `BoundedVolumeSource` 默认 100×100×100；本实现默认 64×64×64。因为 `StructuredMesh` 会显式生成单元，分辨率越高内存/构建时间增长越快（KD-tree 只加速找邻域，不减少体网格本身开销）。

## 四、调用方式（代码）

```cpp
#include "Interpolation/iGamePointVolumeInterpolatorFilter.h"

auto filter = iGame::PointVolumeInterpolatorFilter::New();
filter->SetInput(pointCloud);                                       // 点云/数据集（用其点与 POINT 属性）
filter->SetKernelType(iGame::PointKernelType::Gaussian);            // Voronoi / Gaussian / Shepard
filter->SetKernelFootprint(iGame::PointKernelFootprint::Radius);    // Radius / NClosest
filter->SetRadius(1.0);
filter->SetSharpness(2.0);
filter->SetNullPointsStrategy(iGame::PointNullPointsStrategy::NullValue);
filter->SetNullValue(0.0);
filter->SetUseInputBounds(true);
filter->SetResolution(64, 64, 64);
filter->SetInterpolateArrayNames({"field"});          // 只插值指定数组；留空=全部 POINT 数组
// filter->SetUseInputBounds(false);
// filter->SetSamplingBounds(-2,2, -2,2, -1,1);       // 手动采样范围（6 个数）
if (!filter->Execute()) {
    std::cerr << filter->GetMessage() << "\n";                      // 失败原因
    return 1;
}
auto volume = iGame::DynamicCast<iGame::StructuredMesh>(filter->GetOutput()); // 规则体网格
```

## 五、使用示例（自动测试，无需手动输入）

仓库自带两个程序化/AI 生成的点云模型（`Examples/Models/`，均含点标量 `field` 与向量 `momentum`）：

- `AIGen_Points_ScatterCloud.vtk`：`POLYDATA` 散点云，2000 点（GUI 示例默认读取）。
- `AIGen_Points_VertexCloud.vtk`：`UNSTRUCTURED_GRID + VTK_VERTEX` 散点云，2000 点。

运行（构建后工作目录为 `Examples`，路径写死、自动读取）：

```powershell
.\Release\testPointVolumeInterpolator.exe            # 显示输入点云与插值出的体网格
.\Release\testPointVolumeInterpolatorSelfCheck.exe    # 无 GUI 自检：25 项 PASS，返回 0

ctest -R testPointVolumeInterpolator
ctest -R testPointVolumeInterpolatorSelfCheck
```

自检覆盖：Voronoi 重合点精确还原（标量+向量）、常数场（Voronoi/Gaussian/Shepard/Linear 权重归一）、N 近邻、三种空邻域策略、数组选择与超大分辨率拒绝、空点不污染多分量(向量)属性 的回归（共 36 项）。

## 六、相关文件

| 文件 | 说明 |
| --- | --- |
| `iGameCore/Filters/Interpolation/iGamePointVolumeInterpolatorFilter.h/.cpp` | 过滤器实现 |
| `iGameCore/Filters/Interpolation/iGamePointKdTree.h` | KD-tree 薄封装（nanoflann） |
| `iGameCore/Filters/Interpolation/iGamePointInterpolationKernel.h` | 核类型/邻域/空点策略枚举 + 权重函数 |
| `Examples/Filter/Interpolation/TestPointVolumeInterpolator.cpp` | GUI 示例（默认读 `AIGen_Points_ScatterCloud.vtk`） |
| `Examples/Filter/Interpolation/TestPointVolumeInterpolatorSelfCheck.cpp` | 无 GUI 自动回归 |
| `Examples/Models/AIGen_Points_ScatterCloud.vtk` / `AIGen_Points_VertexCloud.vtk` | 点云测试模型（程序化/AI 生成） |
