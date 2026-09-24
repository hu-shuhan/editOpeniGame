# CountCellVerticesFilter 使用说明

对应任务：简单任务（#5）——统计网格中每个单元的顶点数。

## 1. 功能

`CountCellVerticesFilter` 遍历输入网格的全部单元，统计**每个单元拥有的顶点（节点）数**，
并把结果作为 **Cell Data** 写入一个**独立输出的非结构化网格**：

| 输出属性名 | 类型 | 挂载位置 | 含义 |
| --- | --- | --- | --- |
| `cell_vertex_count` | DoubleArray（一维） | 单元（cell） | 每个单元的顶点数，长度 = 单元数 |

- 对三角形输出 3、四边形输出 4、四面体输出 4、六面体输出 8，其余类型以单元实际存储的节点数为准。
- **独立结果节点**：Filter 不修改输入数据，而是新建一个 `UnstructuredMesh` 作为输出
  （默认命名 `<输入名>_VertexCount`）。点与单元只读共享输入，属性集为新建，
  因此原模型的属性不会被改动，模型树里可以把输出作为一个独立节点查看、按 `cell_vertex_count` 着色。
- **重复执行**：写结果前会先删除输出属性集中同名的旧数组，重复执行只保留一份且始终是最新结果。
- **空的模型**：输入没有任何单元时仍然算执行成功，并产出一个长度为 0 的数组，
  保证"执行成功 ⇒ 数组一定存在"，界面不会出现"找不到数组"的矛盾状态。

## 2. 支持的数据类型

公开支持以下类型（其余类型返回失败并给出原因，不静默处理）：

| 类型 | 说明 |
| --- | --- |
| `UnstructuredMesh` | 直接使用其单元数组与单元类型 |
| `SurfaceMesh` | 面单元，输出单元类型按点数推断（3→三角形、4→四边形、其他→多边形） |
| `VolumeMesh` | 体单元，输出单元类型按点数推断（4→四面体、5→金字塔、6→三棱柱、8→六面体） |

输入为空、无单元数组、类型不支持等情况会返回 `false`，原因见 `GetMessage()`。

## 3. 调用方式

### 3.1 GUI 方式

1. 打开/导入一个网格模型；
2. 主菜单 **算法处理 → 统计单元顶点数**，在左侧面板点击 **执行**；
3. 结果网格会作为一个独立节点加入模型树（`<原名>_VertexCount`），
   可选中它并按 `cell_vertex_count` 着色；同时面板表格显示每个单元的顶点数。
   对标 ParaView 的管线行为：执行后**原模型自动隐藏、结果节点替换显示**（场景始终只渲染一份网格）。

> 表格采用**分页**展示（对标框架 igQtSearchInfoWidget）：每页最多 1000 个单元，
> 用「上一页 / 下一页」翻页，大模型不会因为一次性塞满 QTableWidget 而卡界面；
> 完整数据仍可「导出CSV」查看。空模型会显示"该模型没有单元（0 个）"，不弹错误框。

### 3.2 代码方式

```cpp
#include <CountCellVertices/iGameCountCellVerticesFilter.h>

auto filter = iGame::CountCellVerticesFilter::New();
filter->SetInput(mesh);
if (filter->Execute()) {
    // 结果在独立输出节点里，原模型 mesh 没有被改动
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    // out 的 Cell Data 里包含名为 "cell_vertex_count" 的数组
} else {
    std::cerr << filter->GetMessage() << std::endl;  // 失败原因
}
```

### 3.3 命令行测试

```bash
cd Examples
./testCountCellVertices
```

自动读取相对路径模型，覆盖三个场景并全部通过才输出 PASS：
1. 混合单元网格 `CountCellVertices_mixed_cells.vtk`（独立输出 + 值正确 + 原模型不被改）；
2. 重复执行（同名数组只保留一份）；
3. 空网格 `CountCellVertices_empty.vtk`（0 单元也产出空数组）。

## 4. 使用示例

使用 AI 生成的测试模型 `Examples/Models/CountCellVertices_mixed_cells.vtk`
（一个包含多种单元类型的网格：六面体、三棱柱、金字塔、四面体、四边形、三角形、线段）：

| 单元类型 | 顶点数 |
| --- | ---: |
| 六面体 Hexahedron | 8 |
| 三棱柱 Prism (Wedge) | 6 |
| 金字塔 Pyramid | 5 |
| 四面体 Tetrahedron | 4 |
| 四边形 Quad | 4 |
| 三角形 Triangle | 3 |
| 线段 Line | 2 |

执行后输出网格的 `cell_vertex_count` 依次为 `8, 6, 5, 4, 4, 4, 3, 3, 2`，
正好对应各单元的顶点数。

## 5. 注意事项

1. **结果在输出节点、不在原模型**：`cell_vertex_count` 只写在输出网格上，输入模型保持不变；
   需要"未统计的原始网格"时，原模型一直都在。
2. **重复执行不堆积**：同名数组写入前会先删除旧的，属性列表里永远只有一份 `cell_vertex_count`。
3. **单点单元（VERTEX）不入统计**：iGame 的 VTK 读取器不会为 `VERTEX`（1 点）单元建立 Cell，
   因此 0 维点单元不会出现；线段（LINE，2 点）正常统计为 2。
4. **统计口径**：顶点数取自单元连接表实际存储的节点数，与几何坐标是否重复无关，也与单元类型枚举的固定节点数无关。
5. **不支持的类型会明确失败**：输入不是 UnstructuredMesh / SurfaceMesh / VolumeMesh 时返回 `false`
   并通过 `GetMessage()` 给出原因，不会静默跳过。
6. **0 单元也是成功**：空网格执行成功并产出长度为 0 的数组，界面据此显示"没有单元"而不是报错。
