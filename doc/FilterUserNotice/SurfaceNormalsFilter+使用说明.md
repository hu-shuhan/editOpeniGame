# SurfaceNormalsFilter 使用说明

## 功能
`SurfaceNormalsFilter` 用于计算表面网格（`SurfaceMesh`，即多边形表面网格 / Poly Data）的面法向量与点法向量。

- 面法向量使用 Newell 方法计算并单位化。
- 点法向量按平滑区域计算：法向量夹角不超过特征角的相邻面归为同一平滑区域，区域内所有邻接面单位法向量取平均后单位化。
- 支持按特征角在锐边上分裂共享顶点，使不同平滑区域拥有独立的点法向量。
- 支持统一相邻面的环绕方向（一致性），以及整体翻转法向量。
- 生成独立的 `<输入名称>_normals` 输出节点，不修改原输入。
- 参数语义与 ParaView / VTK `vtkPolyDataNormals` 对齐。

## 支持范围
- 仅支持 `SurfaceMesh`（三角形 / 四边形 / 多边形面片表面网格）。
- 对 `UnstructuredMesh` 等非表面网格类型，`Execute()` 返回 `false`。

## 参数说明

| 参数 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `ComputePointNormals` | `bool` | `true` | 是否计算并输出点法向量。 |
| `ComputeCellNormals` | `bool` | `true` | 是否计算并输出面法向量。 |
| `Splitting` | `bool` | `true` | 是否在锐边上分裂共享顶点，使不同平滑区域拥有独立的点。 |
| `FeatureAngle` | `double`（度） | `30.0` | 锐边特征角，范围 `0~180`；实现中会钳制到该区间。 |
| `FlipNormals` | `bool` | `false` | 是否翻转最终法向量（反转面环绕方向并取反法向量）。 |
| `Consistency` | `bool` | `true` | 是否自动统一相邻面的环绕方向。 |

对应接口位于 `iGameSurfaceNormalsFilter.h`：

- `SetComputePointNormals(bool)` / `GetComputePointNormals()`
- `SetComputeCellNormals(bool)` / `GetComputeCellNormals()`
- `SetSplitting(bool)` / `GetSplitting()`
- `SetFeatureAngle(double)` / `GetFeatureAngle()`
- `SetFlipNormals(bool)` / `GetFlipNormals()`
- `SetConsistency(bool)` / `GetConsistency()`

## 算法说明
- 读取每个面的原始点序，去除连续重复点与首尾重复点，得到计算点序。
- 使用 Newell 方法计算多边形面法向量并单位化；零面积退化面的法向量为零向量。
- `Consistency` 开启时，通过广度优先遍历统一相邻面的环绕方向，避免相邻面法向量方向不一致。
- `FlipNormals` 开启时，统一反转所有面的环绕方向并取反法向量。
- `Splitting` 开启时，先保留全部原始点及其编号，再按特征角把每个原始点周围的面划分为平滑区域；区域 `0` 继续使用原始点，其余区域追加分裂点。
- 点法向量由该输出点周围邻接面的单位法向量求和后归一化得到。

## 输出属性
输出节点保留输入网格的其他 PointData / CellData；同名 `Normals` / `Normals_Magnitude` 会被重新计算并覆盖。

- 面数据（`IG_CELL`）：
  - `Normals`：3 分量法向量（`IG_NORMAL`），已单位化；退化面为 `(0,0,0)`。
  - `Normals_Magnitude`：1 分量标量（`IG_SCALAR`），有效面为 `1.0`，退化面为 `0.0`。
- 点数据（`IG_POINT`）：
  - `Normals`：3 分量法向量（`IG_NORMAL`），已单位化；无邻接有效面的点为 `(0,0,0)`。
  - `Normals_Magnitude`：1 分量标量（`IG_SCALAR`），有效点为 `1.0`，否则为 `0.0`。

只有对应开关开启时，才会添加对应位置的法向量属性。

## 调用方式

```cpp
#include <SurfaceNormals/iGameSurfaceNormalsFilter.h>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include <iGameSurfaceMesh.h>

auto input = iGame::FileIO::ReadFile("surface.vtk");
auto inputMesh = iGame::DynamicCast<iGame::SurfaceMesh>(input);
if (inputMesh == nullptr) {
    return;
}

auto filter = iGame::SurfaceNormalsFilter::New();
filter->SetInput(inputMesh);

// 可选参数，均提供默认值
filter->SetComputePointNormals(true);
filter->SetComputeCellNormals(true);
filter->SetSplitting(true);
filter->SetFeatureAngle(30.0);
filter->SetFlipNormals(false);
filter->SetConsistency(true);

if (!filter->Execute()) {
    return;
}

auto output = iGame::DynamicCast<iGame::SurfaceMesh>(filter->GetOutput(0));
iGame::Scene::Pointer scene = iGame::Scene::New();
scene->AddModel(output);
```

## 使用示例

参考示例程序：

```
Examples/Filter/SurfaceNormals/TestSurfaceNormalsFilter.cpp
```

示例程序读取表面网格，执行法向量计算，并在控制台输出点法向量和面法向量。

常用测试模型：

- `Examples/Models/SurfaceNormalsFilter_pyramid_roof.vtk`（示例程序默认模型）
- `Examples/Models/SurfaceNormalsFilter_test.vtk`

## Qt 端操作
- 菜单入口：`面/点法向量计算 (Surface Normals)`。
- 弹窗提供六个参数：`计算点法向量`、`计算面法向量`、`锐边分裂 (Splitting)`、`特征角（度）`、`一致性 (Consistency)`、`翻转法向量 (Flip Normals)`。
- 特征角必须是 `0~180` 之间的数值，否则提示参数错误。
- 输入不是 `SurfaceMesh` 时，执行失败并提示“仅支持多边形表面网格（Poly Data）”。

## 输出说明
- 输出节点名称为 `<输入名称>_normals`。
- 面数量保持不变。
- `Splitting=false` 时点数量不变；`Splitting=true` 时，锐边分裂点会追加到原始点之后，输出点数量可能增加。
- 退化面（零面积）使用原始点序输出，法向量与模长为 `0`。

## 注意事项
- 输入必须是 `SurfaceMesh`，否则 `Execute()` 返回 `false`。
- filter 不修改输入数据对象，输出为独立副本。
- 法向量已单位化，`Normals_Magnitude` 只用于区分有效 / 退化状态（`1.0` / `0.0`）。
