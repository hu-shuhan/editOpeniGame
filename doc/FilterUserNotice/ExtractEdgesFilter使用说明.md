# ExtractEdgesFilter 使用说明

对应任务：中等任务（#28）——提取网格的边（去重），供可视化与导出。

## 1. 功能

`ExtractEdgesFilter` 从输入网格中提取**全部唯一边**（两个端点连成的 1 维线段单元），
输出一个**独立结果节点** `UnstructuredMesh`，其中每个单元都是 `IG_LINE`（2 点线段）：

- 面网格（三角形/四边形）与体网格（四面体/六面体等）按单元的**拓扑边**提取；
- 输入中已有的线单元（`IG_LINE`）原样保留，折线（`IG_POLY_LINE`）逐段拆成多条线段；
- **共享边自动去重**：相邻单元共用的边只保留一条；
- **输出属性正确重建**：
  - Point Data（点数据）**深拷贝保留** —— 点数不变，语义仍然成立；
  - Cell Data（单元数据）**按"每条边的来源单元"重映射**：输入单元数是 N、输出是边数 M，
    不能原样沿用（长度对不上）也不能丢弃，因此输出第 i 条边取它来源单元的对应值；
    共享边取**来源单元 ID 较小**者（与 `vtkExtractEdges` 的 minimum cell id 规则一致），
    保证输出属性长度与边数一致。
  - 不生成"边→来源单元"的映射数组（`vtkExtractEdges` 也不生成）。
  - 固定生成 **`CellType`** 数组（Cell Data，长度 = 边数）：每条边的 **VTK 单元类型编号**。
    提取结果全是 1 维线单元，所以值恒为 `vtkLine = 3`
    （注意是 **VTK 编号 3**，不是 iGame 内部的 `IG_LINE = 2`）。
    语义与 ParaView 中给数据集添加 "Cell Types" 数组（取自 `GetCellTypesArray()`）一致，
    便于在属性面板里以数组形式查看/筛选每条边的单元类型。

典型用途：把体/面网格变成"线框"展示拓扑，或配合 `ExportEdgesFilter` 导出 `.vtk`。

## 2. 支持的数据类型

输入可为 `UnstructuredMesh` / `SurfaceMesh` / `VolumeMesh`（内部统一转为 UnstructuredMesh 表示）。
其余类型返回 `false`（见 `GetMessage()`）。

**失败与空结果的区分**：
- 没有输入、类型不支持、或 0 个单元：返回 `false`，**绝不会把原模型当作结果返回**；
- 输入有效但没有任何边可提取（例如单元全是不支持的类型）：返回 `true`，输出是 0 条边的空线网格，
  界面据此不创建结果节点。

**不支持的单元**：被跳过的单元数量与类型分布记录在 `GetSkippedCellCount()` / `GetSkippedCellTypes()`，
并写入 `GetMessage()`（如 `skipped 3 cell(s): Vertex x3`），供界面显示。

## 3. 调用方式

### 3.1 GUI 方式

1. 打开/导入一个网格模型（体网格或面网格效果更明显）；
2. 主菜单 **算法处理 → 边提取**，点击 **执行**；
3. 视图区显示提取出的边（线框）；点「导出边为 VTK」保存为 `.vtk` 文件。

> 界面会检查 Filter 的返回值：失败时弹出具体原因；没有可提取边时不创建结果节点并提示；
> 部分单元不支持时会提示"跳过了多少个"。

### 3.2 代码方式

```cpp
#include <ExtractEdges/iGameExtractEdgesFilter.h>

auto filter = iGame::ExtractEdgesFilter::New();
filter->SetInput(mesh);
if (!filter->Execute()) {
    std::cerr << filter->GetMessage() << std::endl;   // 失败原因
    return;
}
auto edgesMesh = filter->GetEdgesMesh();               // 全部为 IG_LINE 的边网格
if (edgesMesh->GetNumberOfCells() == 0) {
    // 成功但没提取到边（见 GetMessage() 里的跳过统计）
}
```

### 3.3 命令行测试

```bash
cd Examples
./testExtractEdges
```

自动读取相对路径模型，覆盖多个场景并全部通过才输出 PASS：
1. 带 Point Data / Cell Data 的三角形网格 `ExtractEdges_tri_cell_data.vtu`
   （验证独立输出、Point Data 保留、Cell Data 长度与边数一致、不额外生成辅助数组）；
2. 六面体网格 `ExtractEdges_hexa_grid.vtk`（去重后 33 条唯一边）；
3. 空网格（0 单元 → 返回失败，不返回原模型）；
4. 复测示例 `extract_edges_cell_data_mismatch.vtu`（9 条边 / CellValue 6×10+3×20 / OriginalCellTag 6×100+3×200）；
5. 多分量 + 点/单元同名数组 `ExtractEdges_multicomp_cell_data.vtk`
   （验证多分量数组长度按"元组数"计、同名点数组不被误删）。

## 4. 使用示例

模型 `Examples/Models/ExtractEdges_hexa_grid.vtk`：`2×2×1` 六面体连续网格（18 点 / 4 单元）。

执行后输出 33 条唯一边，恰好等于该 3D 线框网格的全部棱：
- 沿 X：`nx·(ny+1)·(nz+1) = 2·3·2 = 12`
- 沿 Y：`(nx+1)·ny·(nz+1) = 3·2·2 = 12`
- 沿 Z：`(nx+1)·(ny+1)·nz = 3·3·1 = 9`
- 合计 `12 + 12 + 9 = 33`

模型 `Examples/Models/ExtractEdges_tri_cell_data.vtu`：9 点 / 8 个三角形，带 `point_scalar`（点）与
`cell_id`（单元）两个数组。提取后输出 16 条唯一边：`point_scalar` 长度仍为 9（点），
而 `cell_id` 不会原样沿用（长度 8 与边数 16 不符），按来源单元重映射为长度 16 的数组。

模型 `Examples/Models/extract_edges_cell_data_mismatch.vtu`：2 个共享一个面的四面体，
单元数组 `CellValue = [10, 20]`、`OriginalCellTag = [100, 200]`。提取后 9 条边中，
共享边继承来源单元 ID 较小的 Cell0 的数据，因此 `CellValue` 为 6 个 10 + 3 个 20、
`OriginalCellTag` 为 6 个 100 + 3 个 200。

## 5. 注意事项

1. **边是"拓扑边"**：提取依据是单元的拓扑连接（四面体 6 条、六面体 12 条），高次单元按基础单元拓扑提取。
2. **共享边只保留一次**：相邻单元共用的边自动去重，输出边数 ≤ 各单元棱数之和。
3. **Point Data 深拷贝保留、Cell Data 按来源单元重映射**：点数据语义不变故保留；
   单元数据因长度变化不能沿用，按"每条边的来源单元"逐值重映射（共享边取来源单元 ID 较小者），
   与 ParaView / `vtkExtractEdges` 的 minimum cell id 规则一致；不生成"边→来源单元"的映射数组。
4. **`CellType` 用的是 VTK 编号**：值恒为 `3`（`vtkLine`）。别和 iGame 内部的 `IG_LINE = 2` 混；
   导出成 `.vtk` 时，该数组会与 `CELL_TYPES` 块表达同一信息（`CELL_TYPES` 是格式自带块，不是数组）。
5. **单点单元（VERTEX）会跳过**：0 维点单元不产生边，且会被计入跳过统计。
6. **失败与空结果区分**：0 单元/类型不支持 → 返回 `false`；有效但无可提取边 → 返回 `true` + 0 条边的空线网格。
   两者都不会返回原模型。
7. 导出请用 **VTK（.vtk）格式**：面模型类格式（STL/OBJ/PLY/OFF）不支持 1 维线单元。
