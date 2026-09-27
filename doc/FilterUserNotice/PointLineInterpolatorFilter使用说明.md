# PointLineInterpolatorFilter 使用说明

## 1. 功能说明

`PointLineInterpolatorFilter` 按照 ParaView Point Line Interpolator 的工作方式，在两个端点之间建立参数化线段，并把输入数据的点属性插值到线段采样点上。它适合提取温度、压力、速度等场量沿指定线段的变化，用于曲线绘制或定量分析。

线段采样点使用下式生成：

```text
t_i = i / Resolution
P_i = (1 - t_i) * Point1 + t_i * Point2
```

其中 `i` 从 0 到 `Resolution`，所以输出包含 `Resolution + 1` 个点和 `Resolution` 个线单元。

## 2. 输入与输出

- 输入：至少包含一个点的 `DataObject`；待插值数组必须是点属性，且元组数等于输入点数。
- 输出：新的 `UnstructuredMesh` 参数化线段，不修改输入模型。
- 输出几何：`Resolution + 1` 个采样点、`Resolution` 个 `IG_LINE` 单元。
- 输出属性：名称和分量数与输入点属性一致；浮点、双精度类型保持不变，整数数组提升为浮点数组以保留小数插值结果。

## 3. 主要参数

| 参数 | 说明 | 约束或选项 |
| --- | --- | --- |
| `Point1` / `Point2` | 采样线段的两个端点 | 三维坐标 |
| `Resolution` | 线段分段数 | 必须大于等于 1 |
| `KernelType` | 插值核 | `VORONOI`、`GAUSSIAN`、`SHEPARD` |
| `KernelFootprint` | 高斯或 Shepard 核的邻域选择 | `RADIUS` 或 `N_CLOSEST` |
| `Radius` | 半径邻域及高斯权重尺度 | 半径模式下必须大于 0 |
| `NumberOfPoints` | 最近点邻域大小 | 最近点模式下必须大于等于 1 |
| `Sharpness` | 高斯核锐度 | 必须大于等于 0 |
| `PowerParameter` | Shepard 反距离幂指数 | 必须大于 0 |
| `NullPointsStrategy` | 邻域为空时的处理 | `MASK_POINTS`、`NULL_VALUE`、`CLOSEST_POINT` |
| `NullValue` | 无有效邻点时写入的值 | 任意实数 |

三种插值核的含义：

- `VORONOI`：使用距离采样点最近的源点值。
- `GAUSSIAN`：对邻域内源点按高斯权重加权并归一化。
- `SHEPARD`：按距离的负幂进行加权并归一化。

## 4. C++ 调用方式

```cpp
#include <PointLineInterpolator/iGamePointLineInterpolatorFilter.h>
#include <iGameFileIO.h>

auto input = iGame::FileIO::ReadFile(
    "./Models/PointLineInterpolatorFilter_Test.vtk");

auto filter = iGame::PointLineInterpolatorFilter::New();
filter->SetInput(input);
filter->SetPoint1(iGame::Point(0.0, 0.0, 0.0));
filter->SetPoint2(iGame::Point(2.0, 0.0, 0.0));
filter->SetResolution(20);
filter->SetKernelType(iGame::PointLineInterpolatorFilter::GAUSSIAN);
filter->SetKernelFootprint(iGame::PointLineInterpolatorFilter::RADIUS);
filter->SetRadius(1.0);
filter->SetSharpness(2.0);
filter->SetNullPointsStrategy(
    iGame::PointLineInterpolatorFilter::CLOSEST_POINT);

if (!filter->Execute()) {
    return 1;
}

auto lineOutput = filter->GetLineOutput();
```

## 5. GUI 使用方法

1. 在 iGameVis 中加载一个带点属性数组的模型并选中该模型。
2. 打开一级菜单“算法处理”。
3. 点击“点线插值 (Point Line Interpolator)”。
4. 参数面板打开时，视图会立即出现绿色起点、红色终点和青色预览线。通过“拖动端点”下拉框选择起点或终点后，可在三维视图中用左键拖动该点；坐标输入框和预览线会同步更新。
5. 也可直接编辑起点、终点坐标；完成编辑后预览会立即刷新。关闭面板会自动移除这些临时预览对象，不会写入模型树。
6. 在中文参数界面设置分辨率、插值核、邻域及空点策略。若显示曲线，“数组分量”会随“曲线数组”自动更新为下拉选项，避免输入无效分量编号。
7. 执行后显示采样线；可选择输出数组查看沿线数值，并在曲线界面分析属性变化。

## 6. 自动测试示例

- 示例源码：`Examples/Filter/PointLineInterpolator/TestPointLineInterpolator.cpp`
- 测试模型：`Examples/Models/PointLineInterpolatorFilter_Test.vtk`
- 运行时相对路径：`./Models/PointLineInterpolatorFilter_Test.vtk`
- CMake 目标：`testPointLineInterpolator`

测试模型是为本 Filter 生成的两点非结构网格，带有 `Temperature` 标量、`Velocity` 向量和 `IntegerSamples` 整数标量。示例内部写死相对路径，运行时无需手动输入文件名或参数。CMake 会把 `Examples/Models` 复制到示例构建目录。

示例自动检查：参数化采样点和线单元数量、Voronoi/高斯/Shepard 三种插值、半径和最近点邻域、整数提升、三种空点策略、重复执行及非法参数处理。

## 7. 注意事项

- 只插值点属性；单元属性、已删除属性或元组数与输入点数不一致的数组会被跳过。
- `Resolution` 表示线段数而不是采样点数，采样点数总是比它多 1。
- 线段可以位于输入模型范围之外。此时结果取决于邻域和空点策略，使用 `CLOSEST_POINT` 会回退到最近源点。
- 使用 `MASK_POINTS` 时会额外生成 `vtkValidPointMask` 点数组；无有效邻点处为 0。可通过 `SetValidPointsMaskArrayName()` 修改名称。
- 采样点与源点位置完全重合时直接使用该源点值，避免距离为零导致的权重异常。
- 更高的 `Resolution` 或更大的邻域会增加计算量，应按分析精度选择。
