# FeatureEdgeRegionFilter 使用说明

## 1. 功能概述

`FeatureEdgeRegionFilter` 用于根据特征边对表面网格（`SurfaceMesh`）进行区域划分，并输出带有区域编号的 `SurfaceMesh`。

Filter 将特征边作为区域划分的边界：对于不属于特征边、且恰好连接两个面的边，合并其两侧面的区域，最终得到若干连通区域。

输出结果包含一个面单元属性：

- `Region Id`：面所属的区域编号，从 `0` 开始连续编号。同一区域内的面具有相同编号。

本 Filter 不直接提取特征边，通常先使用 `FeatureEdgesFilter` 提取特征边，再将结果作为本 Filter 的第二个输入。

## 2. 调用方式

```cpp
#include <FeatureExtraction/iGameFeatureEdgesFilter.h>
#include <FeatureExtraction/iGameFeatureEdgeRegionFilter.h>

// 先从表面网格中提取特征边。
auto featureFilter = iGame::FeatureEdgesFilter::New();
featureFilter->SetInput(surfaceMesh);
featureFilter->SetFeatureAngle(30.0);
featureFilter->SetBoundaryEdges(true);
featureFilter->SetFeatureEdges(true);
featureFilter->SetNonManifoldEdges(true);
featureFilter->SetManifoldEdges(false);

if (!featureFilter->Execute()) {
    // 特征边提取失败，停止后续处理。
    return 1;
}

// 根据特征边划分表面区域。
auto filter = iGame::FeatureEdgeRegionFilter::New();
filter->SetInput(0, surfaceMesh);
filter->SetInput(1, featureFilter->GetOutput());

if (!filter->Execute()) {
    // 区域划分失败，停止后续处理。
    return 1;
}

auto output = filter->GetOutput();
auto regionAttribute = output->GetAttributeSet()->GetAttribute("Region Id");
```

输入 0 必须是 `SurfaceMesh`；输入 1 必须是与该表面网格对应的特征边 `UnstructuredMesh`，其中包含有效的 `Edge Ids` 属性。如果输入为体网格或其他非表面网格，应先执行表面提取。

## 3. 使用示例

项目提供了窗口示例：

- `testFeatureEdgeRegion`：自动读取模型，完成表面提取、特征边提取和区域划分，并打开窗口显示按区域着色的表面及特征边。

示例源文件：

```text
Examples/Filter/FeatureExtraction/FeatureEdgeRegion.cpp
```

测试模型通过相对路径自动读取，无需输入命令行参数：

```text
./Models/FeatureRegion_MountingPlate.vtk
```

该模型为带中央凸台、四个斜坡和两个方形通孔的机械板件，包含 5,878 个顶点和 11,760 个三角形。

使用示例中的 30° 特征角度及边类型设置时，预期得到 19 个区域：

- 底板顶面和底面，共 2 个区域；
- 底板外侧面，共 4 个区域；
- 两个方形通孔的内壁，共 8 个区域；
- 凸台斜坡，共 4 个区域；
- 凸台顶面，共 1 个区域。

在仓库根目录编译并运行（假设主库已安装、Examples 已配置，使用当前 VS 构建目录）：

```powershell
cmake --build cmake-build-examples-vs --config Release --target testFeatureEdgeRegion
cd cmake-build-examples-vs
.\Release\testFeatureEdgeRegion.exe
```

预期终端输出：

```text
Number of regions:19
```

窗口中显示不同区域的着色结果，并叠加特征边。

## 4. 注意事项

1. Filter 不会自动将体网格转换为表面网格；体网格应先执行表面提取。
2. 两个输入必须对应同一表面网格。特征边的 `Edge Ids` 必须与原网格边编号一致，提取特征边后不要修改原网格拓扑或边编号。
3. 当前实现对 `Edge Ids` 缺失、长度不足等情况的校验不完整，可能导致越界访问。建议直接使用同一网格的 `FeatureEdgesFilter` 输出。
4. 特征角度在上游 `FeatureEdgesFilter` 中设置，本 Filter 没有独立的角度参数。通常保持 `SetManifoldEdges(false)`，避免普通三角形边参与分区。
5. 输出会直接在输入表面网格上添加或更新 `Region Id`，不会创建独立的区域网格。该属性为单分量 `IntArray`；已有同名属性若不是该类型，执行会失败。
6. 区域划分依据拓扑连接关系，不会自动焊接坐标相同的顶点。边界边和连接三个及以上面的非流形边不参与面合并。
