# ElevationFilter 使用说明

## 功能简介

ElevationFilter（高程标量场过滤器，DIME #19）以**低点 → 高点**定义的有向线段为标尺，将每个点的位置参数化为标量（公式与 vtkElevationFilter 完全一致）：

```
v = 高点 - 低点
t = clamp( ((p - 低点) · v) / |v|² , 0, 1 )
Elevation = RangeLow + t × (RangeHigh - RangeLow)
```

生成名为 `"Elevation"` 的点标量属性（数组名可通过 `SetArrayName` 自定义）。核心行为：

- **两端饱和**：低于低点输出 RangeLow、高于高点输出 RangeHigh，任何输入都不产生 NaN；
- **垂直分量归零**：低/高点垂直于线段方向的分量在点积中消去，不影响结果；
- **线段参数化**：t 按线段长度参数化（非单位投影）——线段变长、刻度变疏，如低/高点 (0,0,0)-(1,1,0) 下 t = (x+y)/2，而 (0,0,0)-(2,2,0) 下 t = (x+y)/4；
- **标尺固定、不随数据自适应**：标尺不变时移动/修改点位，输出值与模型颜色都会改变；
- **取色范围 grow-only**（与 ParaView 颜色条行为一致）：dataRange 首次执行挂载输出数据的实际范围，此后每次执行用新数据范围单调扩张——标量范围改大再改小，颜色条保持历史最大范围。

**独立输出语义**：过滤器生成新的输出数据对象，几何（点/面/单元）与输入共享，属性集独立；Elevation 数组挂在输出对象上，**输入对象保持原样**。**输入未变时重复执行复用同一输出对象**（仅替换数组并保留取色范围），模型树中始终只有一个独立输出节点（名称为 `原名_Elevation`）。

典型用途：地形高程着色、沿任意方向生成梯度标量场、给点云添加投影坐标等。

## 与 ParaView Elevation 的参数对应

| ParaView 表单项 | 本过滤器 API | 默认值 | 说明 |
| --- | --- | --- | --- |
| Low Point | `SetLowPoint(x, y, z)` | (0, 0, 0) | 标尺线段低点（t = 0 锚点） |
| High Point | `SetHighPoint(x, y, z)` | (0, 0, 1) | 标尺线段高点（t = 1 锚点） |
| Scalar Range | `SetScalarRange(low, high)` | [0, 1] | 输出值域（要求 low < high） |
| X / Y / Z 轴按钮 | UI 对话框与参数面板的轴按钮 | 默认选中 X | 按包围盒铺满：被选轴取 min/max，其余轴取中心 |

## 调用方法

```cpp
#include "Elevation/iGameElevationFilter.h"

auto filter = iGame::ElevationFilter::New();
filter->SetInput(mesh);                  // PointSet 派生类型：SurfaceMesh / UnstructuredMesh / 点云等
filter->SetLowPoint(0.0, 0.0, 0.0);      // 标尺低点，默认 (0,0,0)（可选）
filter->SetHighPoint(0.0, 0.0, 3.0);     // 标尺高点，默认 (0,0,1)（可选；与低点重合时 Execute 被拒绝）
filter->SetScalarRange(0.0, 1.0);        // 标量范围，默认 [0,1]（可选；要求 low < high）
filter->SetArrayName("Elevation");       // 输出数组名（可选）
if (filter->Execute()) {
    auto output = filter->GetOutput();   // 独立输出对象（类型与输入一致）
    // output->GetAttributeSet() 中含 IG_SCALAR/IG_POINT 的 "Elevation" 数组
}
```

## 使用示例

**UI 方式（两段式，交互对齐 ParaView）**：

1. **入口对话框**：菜单【算法处理】→【高程 (elevation)】，弹出参数对话框——顶部为 X/Y/Z 轴按钮（互斥选中，**默认选中 X**；点击按当前模型包围盒铺满低/高点：被选轴取 min/max、其余轴取中心），下方为低点 X/Y/Z、高点 X/Y/Z、标量范围下限/上限共 8 个输入框（默认值 = 包围盒 X 轴铺满 + 范围 [0,1]），点击应用。
2. **实时参数面板**：应用成功后模型树出现 `<模型名>_Elevation` 独立节点，同时右侧弹出「高程 (Elevation)」参数面板（与对话框同构：X/Y/Z 轴按钮 + 8 个输入框 + 应用按钮）。在面板中修改参数后点击应用即**就地重渲染**（复用同一输出对象，模型树不堆叠节点）；点击轴按钮 = 铺满 + 立即应用。
3. **颜色条 grow-only**：标量范围改大再改小，颜色条保持历史最大范围（与 ParaView 一致）；重新从菜单打开对话框会生成新的滤波器实例，取色范围重新从初始状态开始。

**代码方式**（见 `Examples/Filter/Elevation/TestElevation.cpp`）：

```cpp
// 斜面网格 z = 0,1,2,3，低点 (0,0,0)、高点 (0,0,3)、范围 [0,1]：
// t = clamp(z/3, 0, 1) = 0, 1/3, 2/3, 1 -> 输出 {0, 1/3, 2/3, 1}
auto mesh = MakeSlopeMesh();               // 4 点 2 三角形，z = x + 2y
auto filter = iGame::ElevationFilter::New();
filter->SetHighPoint(0.0, 0.0, 3.0);
filter->SetInput(mesh);
filter->Execute();                          // 输出 = {0, 1/3, 2/3, 1}
auto output = filter->GetOutput();          // 独立对象，mesh 上没有 Elevation 数组

// 同一标尺、整体上移 3 的相同网格：t = 1, 4/3, 5/3, 2 -> 全部饱和输出 {1,1,1,1}
// ——标尺不变、点位改变，输出与颜色随之改变（ParaView 兼容语义）
```

## 注意事项

1. **输入不被修改**：Elevation 数组只挂在输出对象上；对同一输入重复执行不会累积污染输入。
2. **输出复用**：同一输入对象重复 `Execute()` 复用同一输出对象（仅替换数组指针、保留 dataRange），模型树不堆节点；更换输入或新建滤波器实例则生成新的独立输出。
3. **几何共享**：输出对象与输入共享点/面/单元指针（浅共享），不复制几何数据；修改一方几何会同时影响另一方。若需完全独立请自行深拷贝。
4. **饱和而非特判**：平面网格（所有投影值相同）按标尺公式正常计算——如 z=5 平面在低/高点 (0,0,0)-(0,0,1) 下全输出 1、在 (0,0,4)-(0,0,6) 下全输出 0.5。
5. **取色范围 grow-only（已锁定）**：Elevation 属性挂载时即锁定范围（`rangeLocked + ExpandOnly`），框架的"每帧按数据重算"（UpdateAllDataRange）不会覆盖过滤器维护的只扩不缩范围——标量范围改大再改小，颜色条保持历史最大范围。需要重置时：在标量场面板把映射范围模式切回"每帧调整"（解锁并按数据重算），或重新从菜单打开对话框生成新的滤波器实例。
6. **参数校验**：low ≥ high 的标量范围被拒绝并保持原值；低点与高点重合时 `Execute()` 返回 `false`（UI 侧已前置校验并提示）。
7. **输出类型**：SurfaceMesh / VolumeMesh / StructuredMesh 输入得到 SurfaceMesh 输出（共享面）；UnstructuredMesh 输入得到 UnstructuredMesh 输出（共享单元）；裸 PointSet（点云）得到 PointSet 输出。
8. **配套测试**：`Examples/Filter/Elevation/TestElevation.cpp`（构建目标 `testElevation`，12 个用例），配套模型 `Examples/Models/ElevationSlopeTerrain.vtk`、`ElevationTerraces.vtk`。
