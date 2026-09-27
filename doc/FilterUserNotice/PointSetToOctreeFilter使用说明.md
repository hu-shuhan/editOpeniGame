# PointSetToOctreeFilter 使用说明

## 功能

**Point Set To Octree（点集转八叉树）** 过滤器，是 VTK `vtkPointSetToOctreeImageFilter` 的移植实现。
它根据输入点集的包围盒与 `NumberOfPointsPerCell`，把包围盒划分为一个规则的体网格（分箱数
`numBuckets = numPoints / numPointsPerCell`，经等价于 `vtkBoundingBox::ComputeDivisions` /
`ClampDivisions` 的算法得到各轴分割数），输出一个 `StructuredMesh`，每个体素（cell）带一个
`unsigned char` 单元标量 **`octree`**，用 8 位 bitfield 记录该体素内落入的 8 个八分区（子八叉树节点）：

```
bit0 = (x<=cx, y<=cy, z<=cz)   bit1 = (x>cx, y<=cy, z<=cz)
bit2 = (x<=cx, y>cy, z<=cz)    bit3 = (x>cx, y>cy, z<=cz)
bit4 = (x<=cx, y<=cy, z>cz)    bit5 = (x>cx, y<=cy, z>cz)
bit6 = (x<=cx, y>cy, z>cz)     bit7 = (x>cx, y>cy, z>cz)
```

其中 `(cx, cy, cz)` 为该体素中心的坐标；多个点落入同一体素时按位 OR 累加。
体素归属按 VTK 的公式确定：`ijk[i] = floor((p - origin) / spacing)` 并钳制到
`[0, 分割数-1]`，`outCellId = ijk[0] + ijk[1]*extent[1] + ijk[2]*extent[1]*extent[3]`。

若开启 `ProcessInputPointArray`，则同时处理一个输入点属性数组，并把结果作为一个多分量（每分量一个
统计函数，顺序为 LastValue / Min / Max / Count / Sum / Mean）的单元属性数组附加到输出，与 VTK 一致。
默认勾选 `Min / Max / Count / Mean` 时输出 5 个分量（`Min, Max, Count, Sum, Mean`）。

> 说明：iGameVis 没有独立的 `vtkImageData` / `vtkPartitionedDataSet` 概念，因此 VTK 的「分片数据集 +
> 单张图像」在此直接映射为一个 `StructuredMesh`；图像的 origin / spacing 通过网格点坐标原样表达。

## 参数

| 参数 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `NumberOfPointsPerCell` | `igIndex64` | `1` | 每个体素期望容纳的平均点数，自动钳制到 >= 1 |
| `ProcessInputPointArray` | `bool` | `false` | 是否处理输入点属性数组并统计 |
| `InputPointArrayName` | `std::string` | 空 | 要处理的点属性名；为空时取第一个 `IG_POINT` 上的标量属性 |
| `ComputeLastValue` | `bool` | `false` | 统计：末值 |
| `ComputeMin` / `ComputeMax` | `bool` | `true` | 统计：最小 / 最大值 |
| `ComputeCount` | `bool` | `true` | 统计：计数 |
| `ComputeSum` | `bool` | `false` | 统计：求和 |
| `ComputeMean` | `bool` | `true` | 统计：均值（开启时自动计算 Count 与 Sum） |

### Qt 参数面板

菜单 **算法处理 → 点集转八叉树 (Point Set To Octree)** 打开停靠面板，包含：

- **每体素平均点数**：对应 `NumberOfPointsPerCell`（最小 1）；
- **处理点属性数组**：对应 `ProcessInputPointArray`；未勾选时，属性下拉框与 6 个统计函数复选框
  整体置灰；
- **点属性（仅单分量）**：下拉框只列出当前模型上**单分量**的 `IG_POINT` 数组，对应
  `InputPointArrayName`；若模型上没有可用数组会给出提示；
- **统计函数复选框**：`Last (末值)`、`Min`、`Max`、`Count`、`Sum`、`Mean`，默认勾选 Min/Max/Count/Mean；
  面板会提示「Mean 开启时会自动一并计算 Count 与 Sum」以及「至少需开启一个统计函数」；
- **执行前预估与诊断**：显示输入点数/单元数、预估体素数（= 点数 / 每体素平均点数）与输出分量数；
  预估体素数过大（> 5000 万）时执行前弹窗确认；执行后显示本次运行的诊断信息。

面板本身不直接写模型树：它发出 `resultReady(DataObject::Pointer)` 信号，由主窗口按
「算法结果」加入模型树并刷新渲染。

## 调用方式

```cpp
#include <Convert/iGamePointSetToOctreeFilter.h>

auto filter = iGame::PointSetToOctreeFilter::New();
filter->SetInput(input);                          // 输入：PointSet 子类
filter->SetNumberOfPointsPerCell(1);              // 可选，体素含点阈值
// filter->SetProcessInputPointArray(true);       // 可选，统计点属性
// filter->SetInputPointArrayName("field");       // 可选，指定要统计的数组
// filter->SetComputeLastValue(false);            // 可选，按需勾选统计函数
// filter->SetComputeMean(true);
filter->Execute();

auto out = filter->GetOutput();                   // StructuredMesh，带单元标量 "octree"
// 诊断信息：输出图像维度、体素数、参与统计的点属性与分量数
const std::string& info = filter->GetMessage();
// 以点/表面方式显示：SetViewStyle(IG_POINTS) 或 IG_SURFACE
```

## 使用示例

示例程序：`Examples/Filter/Convert/TestPointSetToOctree.cpp`
测试模型：`Examples/Models/OctreePoints.vtk`（球壳 + 内部随机点共 500 点的点云）

示例硬编码相对路径读入点云后自动执行转换，并以点的形式显示八叉树输出。
运行方式（工作目录为 `build-msvc`）：`.\Release\testPointSetToOctree.exe`

## 注意事项

1. **输入类型**：只接受 `PointSet` 及其子类；无输入时返回 `false`。
2. **体素占用编码**：`octree` 数组是 `unsigned char` 的 8 位 bitfield，渲染/查询时按位判断，而不是
   连续的「占用计数」。
3. **统计函数**：`ProcessInputPointArray=true` 时至少需开启一个统计函数，否则 `Execute` 报错；开启
   `ComputeMean` 会自动连带计算 Count 与 Sum。
4. **输入点数组必须为单分量**：`ProcessInputPointArray=true` 时，指定的点属性数组维数必须为 1，
   否则 `Execute` 报错（与 VTK 的 `SCALARS` 输入约定一致）；Qt 面板的下拉框已按此过滤。
5. **输出维度**：`dimensions[i] = 各轴分割数 + 1`，与 VTK 一致；原点/spacing 由网格点坐标隐式表达。
6. **输入点数组类型**：统计时按输入数组分量逐分量进行，输出为对应分量数的单元属性数组。
7. **数值一致性**：体素归属与 `Min / Max / Count / Sum / Mean` 已用「独立复算」验证——对同一输入点集，
   按 VTK 公式重新计算每个点的体素编号并自行统计，与过滤器输出的每个体素逐项一致（含 Count 之和 =
   输入点数）。

## 输出的着色（继承输入的显示属性）

从一个"有颜色"的点云运行本过滤器时，输出模型曾经会变成统一白色。原因：iGameVis 渲染时按模型
自己的**活动属性**取色（`DrawObject` 的活动属性索引，默认 `-1` 表示没有属性 → 统一白色），而过滤器
输出既没有搬运该数组、也没有设置活动属性。

现在的行为：

- 如果输入模型设置了活动属性（等价 VTK 的活动标量 / 颜色，也就是在模型树里点选的那条属性），
  过滤器会把它**同名、同类型、同维数**地继承到输出的格点上，并把输出的活动属性指向它，因此输出
  模型打开后就是与输入一致的着色；
- 数值取法：先对每个体素内输入点的取值求平均，再把该体素的值散布到它的 8 个角格点上（角点取相邻
  体素均值的平均）。输出是 `StructuredMesh`（`PointSet` 的子类），只支持按点属性着色，因此这里挂成
  点数据而不是单元数据；
- 输入没有设置活动属性时（典型场景：文件刚载入，界面此时还不会自动选属性），自动退化为：优先继承第一个
  真彩色（`IG_RGB`）点属性，其次继承第一个非恒定的 `IG_SCALAR` 点属性，保证输出依然有颜色可看；
  两者都没有时保持原有行为不变。

过滤器面板的提示信息里会写明继承了哪条属性。