# CellSizeFilter 使用说明

> Filter 名称：**CellSizeFilter（计算单元尺寸 / cell_size）**
> 代码位置：`iGameCore/Filters/CellSize/iGameCellSizeFilter.{h,cpp}`

---

## 一、功能

`CellSizeFilter` 遍历输入网格的每一个单元（cell），解析计算其几何尺寸，并将结果作为 **`IG_CELL` 标量属性** 附加到输出模型的属性集上，并可在"查找数据"面板查询。

对任何支持的网格，Filter 统一输出 **三个** cell 标量属性：

| 属性名 | 含义 | 何时有值 |
|--------|------|----------|
| `Length` | 单元的线长度 | 仅 1D 线单元（点数 ≥ 2） |
| `Area` | 单元的面积 | 仅 2D 面单元 |
| `Volume` | 单元的体积 | 仅 3D 体单元 |

### 数值语义

| 值 | 含义 |
|----|------|
| 数值（含 `0`） | 真实计算结果（退化单元的面积/体积就是真实的 0） |
| `NaN` | 该属性对当前单元**不适用**（如四面体单元的 `Length`/`Area`），或单元类型无法计算 |

每个 cell 只在其维度对应的属性中填入计算值，其余两个维度写入 `NaN`。因此 **NaN 与真实的 0 可以区分**。

### 独立输出节点

Filter **不修改输入模型**。执行成功后输出一个**深拷贝的独立模型节点**：

- 深拷贝内容：顶点坐标、单元连接性、（非结构网格的）单元类型数组、原模型的属性集
- 输出节点命名：`原模型名 + "_CellSize"`
- 新属性附加在输出节点的属性集末尾，原模型保持原样

---

## 二、调用方式


### 方式 1：UI 菜单调用

1. 加载并选中一个网格模型
2. 菜单：**算法处理 → 计算单元尺寸 (ComputeCellSize)**
3. 执行成功后：
   - 模型树新增节点 `模型名_CellSize`（含原属性 + Length/Area/Volume 子项，自动展开）
   - "查找数据"面板可逐 cell 查询
   - 算法执行完成弹窗提示：`Cell size computation complete: N cells computed.`

### 方式 2：命令行测试程序

测试程序：`Examples/Filter/FeatureExtraction/CellSizeExtraction.cpp`，目标名 `testCellSizeExtraction`。

```bash
# 工作目录需包含 Models/（构建时由 CMake 自动拷贝）
testCellSizeExtraction.exe
```

- **无需任何输入**：自动对两个内置测试模型执行计算并打印每个属性的前 10 个值
- 返回码：`0` = 全部成功；`1` = 读文件/计算失败

---

## 三、使用示例

### 内置测试模型 1：
`Examples/Models/CellSize_TetraCube.vtk`

单位立方体 [0,1]³ 按标准体对角线分解为 **6 个四面体**（8 点，VTK_TETRA）。
每个四面体体积 = 1/6，总体积 = 1.0。

实际运行输出：

```text
===== ./Models/CellSize_TetraCube.vtk =====
[CellSize] attribute="Length" first 6 values: nan nan nan nan nan nan
[CellSize] attribute="Area"   first 6 values: nan nan nan nan nan nan
[CellSize] attribute="Volume" first 6 values: 0.166667 0.166667 0.166667 0.166667 0.166667 0.166667
[CellSize] done: ./Models/CellSize_TetraCube.vtk
```

### 内置测试模型 2：
`Examples/Models/CellSize_MixedTypes.vtk`

**全部为体单元、类型不一**（23 点，4 个体单元），用于覆盖不同体单元的体积公式：

| cell | 单元类型 | 手算体积 |
|------|----------|----------|
| 0 | 四面体 (VTK_TETRA) | 1/6 ≈ 0.166667 |
| 1 | 六面体 (VTK_HEXAHEDRON) | 1.0 |
| 2 | 金字塔 (VTK_PYRAMID) | 1/3 ≈ 0.333333 |
| 3 | 棱柱 (VTK_WEDGE) | 0.5 |

总体积 = **2.0**。实际运行输出：

```text
===== ./Models/CellSize_MixedTypes.vtk =====
[CellSize] attribute="Length" first 4 values: nan nan nan nan
[CellSize] attribute="Area"   first 4 values: nan nan nan nan
[CellSize] attribute="Volume" first 4 values: 0.166667 1 0.333333 0.5
[CellSize] done: ./Models/CellSize_MixedTypes.vtk
```

两个模型的输出均与手算值一致，可直接在 ParaView 中打开对比验证。

---

## 四、注意事项

1. **NaN 与 0 的区别**
   - `NaN` = 该属性不适用（维度不符）或单元类型无法计算
   - `0` = 真实计算结果（如三点共线的退化三角形面积为 0）
   - 渲染云图时 NaN 单元不参与色标映射

2. **支持的网格类型**
   - `SurfaceMesh`（面）、`VolumeMesh`（体）、`UnstructuredMesh`（非结构）、`StructuredMesh`（结构化）
   - 其他类型（点集、样条等）执行失败，`GetMessage()` 返回：`Unsupported data type, only ...`

3. **不支持的单元（结果为 NaN）**
   - 变长单元：多面体（IG_POLYHEDRON）
   - 二次单元、拉格朗日单元
   - 点数与维度公式不匹配的异常单元（如 3D 网格中出现 5/7 点的非法体）

4. **不同网格类型的分发方式**
   - `UnstructuredMesh`：按**单元类型**（cellType）精确分发（支持混合维度单元）
   - `SurfaceMesh / VolumeMesh / StructuredMesh`：按**维度 + 顶点数**分发
   - `StructuredMesh` 会在计算前自动生成单元连接性（`GenStructuredCellConnectivities()`）

5. **输出节点为深拷贝**
   - 重复执行会在模型树生成多个 `_CellSize` 节点（每次一份独立快照），**不会**修改原模型
   - 不需要旧结果时，请在模型树中手动删除对应节点

---
