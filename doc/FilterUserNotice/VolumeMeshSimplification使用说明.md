# VolumeMeshSimplification 使用说明

## 1. 功能概述

体网格简化用于减少四面体体网格的顶点和单元数量，并输出简化后的 `VolumeMesh`。当前示例使用的实现类为 `iGame::TetraSimplification`，头文件为 `iGameVolumeMeshSimplification.h`。

处理流程为：

```text
读取体网格 → MeshTetrahedralize 四面体化 → TetraSimplification 简化
```

**必须先完成四面体化，再将其输出送入简化器。** 本文测试模型为六面体体网格，不能直接传入简化器。

Filter 支持设置简化目标、保护边界顶点，以及在简化过程中考虑点属性。输出包含简化后的顶点坐标、四面体连接关系及参与处理的点属性，例如测试模型中的 `Temperature` 标量场。

## 2. 调用方式

```cpp
#include <DataProcessing/iGameMeshTetrahedralize.h>
#include <DataProcessing/iGameVolumeMeshSimplification.h>

// input 为已读取的有效体网格，先执行四面体化。
auto tetraFilter = iGame::MeshTetrahedralize::New();
tetraFilter->SetInput(input);
if (!tetraFilter->Execute()) {
    // 四面体化失败，停止后续处理。
    return 1;
}

auto tetInput = tetraFilter->GetOutput();
if (tetInput == nullptr) {
    return 1;
}

// 仅将四面体化后的输出传给简化器。
auto filter = iGame::TetraSimplification::New();
filter->SetInput(tetInput);
filter->SetTargetReduction(0.5);
filter->SetTargetTetraCount(0);
filter->SetPreserveBoundary(true);
filter->SetUseAllPointAttributes(true);

if (!filter->Execute()) {
    // 简化失败，停止后续处理。
    return 1;
}

auto output = filter->GetOutput();
if (output == nullptr) {
    return 1;
}
```

常用参数说明（按当前实现）：

| 接口 | 说明 |
| --- | --- |
| `SetTargetReduction(r)` | 目标顶点保留比例；目标数为四面体化后输入顶点数乘以 `r`，向下取整并至少取 4。`0.5` 表示以约一半顶点为目标，不表示四面体数量一定减半 |
| `SetTargetTetraCount(n)` | `n > 0` 时覆盖比例目标。当前实现实际将其作为目标顶点数，虽然接口名含 `TetraCount`；示例设为 `0`，使用比例目标 |
| `SetPreserveBoundary(true)` | 禁止涉及边界顶点的折叠，用于保护外表面及孔壁边界 |
| `SetUseAllPointAttributes(true)` | 将全部有效点属性纳入处理；设为 `false` 时仅选择当前属性（若其为有效点属性） |

## 3. 使用示例

项目提供了窗口示例：

- `testVolumeMeshSimplification`：自动读取模型，依次执行四面体化和简化，然后打开窗口显示简化结果。

示例源文件：

```text
Examples/Filter/TestVolumeMeshSimplification.cpp
```

测试模型通过相对路径自动读取，无需输入命令行参数：

```text
./Models/VolumeSimplification_FlangedTube.vtk
```

该模型为带两端法兰、斜坡过渡和贯通内孔的管件，包含 3,264 个顶点、2,560 个六面体单元，内部具有实际体单元，不是只有外表面的空壳网格。

模型提供 `Temperature` 点标量场，用于观察属性在四面体化及简化过程中的变化。该属性为程序构造的测试数据，不代表物理仿真结果。

按当前 `MeshTetrahedralize` 的单元中心与面三角化实现，每个测试六面体预计生成 12 个四面体，总计 30,720 个四面体。简化器随后处理这些四面体，而非原始六面体。

在仓库根目录编译并运行（假设主库已安装、Examples 已配置，使用当前 VS 构建目录）：

```powershell
cmake --build cmake-build-examples-vs --config Release --target testVolumeMeshSimplification
cd cmake-build-examples-vs
.\Release\testVolumeMeshSimplification.exe
```

运行过程中会打印简化目标、耗时和输出规模，格式如下，尖括号表示实际运行数值：

```text
[TetraSimplification] Simplifying: <输入顶点数> verts -> target <目标顶点数>
[TetraSimplification] Simplification took <耗时> s
[TetraSimplification] Output: <输出顶点数> verts, <输出四面体数> tets
```

程序自动完成处理后显示结果，窗口需手动关闭。

## 4. 注意事项

1. **必须先四面体化，再简化。** 当前简化器读取体单元时会跳过顶点数不为 4 的单元，直接输入六面体可能导致单元被忽略，不能把类型转换当作四面体化。
2. `MeshTetrahedralize` 接收非结构网格或体网格；表面网格不能直接代替内部体网格。任一步骤失败或输出为空时，应停止后续处理。
3. 简化比例基于四面体化后的输入顶点数，建议设在 `(0, 1]` 内；当前 setter 没有自动限制范围。不要将 `SetTargetReduction` 理解为删除比例，也不要将当前 `SetTargetTetraCount` 的行为理解为精确控制四面体数。
4. 目标数量不是输出保证。边界保护、几何质量限制及可折叠候选不足都可能使算法提前停止，结果也可能因单次折叠而略低于目标顶点数。
5. 开启边界保护后，外观变化可能不明显。应结合终端输出检查顶点数和四面体数的变化，不能仅靠表面截图判断是否发生简化。
6. 当前输出重建参与处理的点属性，使用浮点数组保存；单元属性不会在简化输出中按原样复制。对属性精度有要求时，应另行比较简化前后的数据。
