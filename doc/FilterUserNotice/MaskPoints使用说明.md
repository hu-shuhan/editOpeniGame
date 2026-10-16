# Mask Points 使用说明

## 1. 功能说明

Mask Points 用于从输入数据的点集中按照指定规则抽取部分点，并生成新的点集数据。

该 Filter 可用于降低点数量、抽取代表性采样点以及为后续可视化或数据处理提供简化后的点集。抽样过程中会同步保留被选中点对应的点属性数据。

Mask Points 支持规则抽样和随机抽样两类方式，并提供多种随机采样模式。

## 2. 调用方式

### 2.1 GUI 调用

在 iGameVis 中加载模型后，在菜单栏选择：

`算法处理 -> 点抽样（Mask Points）`

打开参数设置窗口后，可配置抽样参数并执行 Filter。


### 2.2 C++ 调用

使用 Filter 时包含头文件：

```cpp
#include <MaskPoints/iGameMaskPointsFilter.h>
```

创建 Filter 并设置输入：

```cpp
auto filter = iGame::MaskPointsFilter::New();

filter->SetInput(0, input);
filter->SetOnRatio(2);
filter->SetOffset(0);
filter->SetMaximumNumberOfPoints(0);
filter->SetRandomMode(false);

if (!filter->Execute()) {
    return;
}

auto output =
        iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
```

## 3. 使用示例

以下示例使用规则抽样方式，每隔2个点选择一个点：

```cpp
#include <MaskPoints/iGameMaskPointsFilter.h>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>

int main() {
    const std::string fileName =
            "./Examples/Models/MaskPoints_AI_Test.vtk";

    auto obj = iGame::FileIO::ReadFile(fileName);

    if (obj.IsNull()) {
        return 1;
    }

    auto input =
            iGame::DynamicCast<iGame::UnstructuredMesh>(obj);

    if (input.IsNull()) {
        return 1;
    }

    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, input);
    filter->SetOnRatio(2);
    filter->SetOffset(0);
    filter->SetMaximumNumberOfPoints(0);
    filter->SetRandomMode(false);
    filter->SetGenerateVertices(false);

    if (!filter->Execute()) {
        return 1;
    }

    auto output =
            iGame::DynamicCast<iGame::UnstructuredMesh>(
                    filter->GetOutput());

    if (output.IsNull()) {
        return 1;
    }

    return 0;
}
```

测试模型 `MaskPoints_AI_Test.vtk` 包含27个点和8个六面体单元。当 `OnRatio = 2`、`Offset = 0` 时，输出选择点 ID：

```text
0, 2, 4, 6, ..., 26
```

因此最终输出14个点。

## 4. 随机采样模式

启用随机采样：

```cpp
filter->SetRandomMode(true);
```

之后可通过 `SetRandomModeType()` 设置具体采样模式。

当前支持以下模式：

 随机化ID步长：  `RANDOMIZED_ID_STRIDES`  在基于点 ID 的采样过程中引入随机性 
 随机采样：  `RANDOM_SAMPLING`  从输入点集中随机选择指定数量的点 
 空间分层随机采样：  `SPATIALLY_STRATIFIED`  根据空间位置进行分层后随机选点 
 均匀空间分布（边界）：  `UNIFORM_SPATIAL_BOUNDS`  根据模型空间包围范围进行均匀空间采样 
 均匀空间分布（表面）：  `UNIFORM_SPATIAL_SURFACE`  根据有效表面单元进行空间采样 
 均匀空间分布（体积）：  `UNIFORM_SPATIAL_VOLUME`  根据有效体积单元进行空间采样 

例如使用普通随机采样：

```cpp
filter->SetMaximumNumberOfPoints(100);
filter->SetRandomMode(true);
filter->SetRandomModeType(
        iGame::MaskPointsFilter::RANDOM_SAMPLING);
filter->SetRandomSeed(42);
```

设置相同的随机种子可以获得可重复的随机采样结果。

## 5. 顶点生成

默认情况下，Mask Points 可以只输出采样后的点，而不生成单元：

```cpp
filter->SetGenerateVertices(false);
```

此时输出中只包含采样点：

```text
NumberOfPoints > 0
NumberOfCells = 0
```

如果需要为每个采样点创建顶点单元，可以设置：

```cpp
filter->SetGenerateVertices(true);
filter->SetSingleVertexPerCell(true);
```

此时每个采样点对应一个 `IG_VERTEX` 单元。

当前实现不支持：

```cpp
filter->SetGenerateVertices(true);
filter->SetSingleVertexPerCell(false);
```

即暂不支持将多个采样点组织成单个 PolyVertex 单元。

## 6. 点属性

Mask Points 在抽取点坐标的同时，会同步复制所选点对应的点属性。

例如输入模型包含：

```text
PointValue
```

抽样前属性值为：

```text
0, 1, 2, 3, ..., 26
```

使用：

```text
OnRatio = 2
Offset = 0
```

抽样后对应属性为：

```text
0, 2, 4, 6, ..., 26
```

因此输出点属性与抽样后的点保持一一对应关系。

## 7. 注意事项

`OnRatio` 必须大于0。如果设置为0等非法值，Filter 执行失败。

当 `Offset` 超出输入点范围时，Filter 可以得到空输出。

`MaximumNumberOfPoints` 用于限制最终输出点数量。当其设置为0时，不额外限制规则抽样产生的点数。

使用随机采样时，如果需要重复获得相同结果，应设置相同的 `RandomSeed`。

`GenerateVertices = false` 时，输出只包含点，不包含单元。

`GenerateVertices = true` 时，当前应配合 `SingleVertexPerCell = true` 使用。

空间表面采样和空间体积采样依赖输入模型中有效的表面或体积单元。

Filter 主要对点进行抽样，因此输出不会保留原输入模型的原始单元拓扑关系。

## 8. 自动测试

核心功能测试文件：

```text
Examples/Filter/MaskPoints/TestMaskPoints.cpp
```

AI生成模型测试文件：

```text
Examples/Filter/MaskPoints/TestMaskPointsModel.cpp
```

测试模型：

```text
Examples/Models/MaskPoints_AI_Test.vtk
```

测试程序使用写死的相对路径读取模型，不需要手动输入文件路径。

从项目根目录运行：

```bat
build\Examples\Release\testMaskPointsModel.exe
```

正常情况下最终输出：

```text
All Mask Points model tests passed.
```