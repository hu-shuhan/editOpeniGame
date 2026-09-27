# MeshQuality 网格质量评估使用说明

## 1. 功能

`MeshQualityFilter` 用于对输入网格进行综合质量评估。

该 Filter 根据网格中 Cell 的类型，调用对应的 `SurfaceMeshMetricsFilter` 或 `VolumeMeshMetricsFilter` 计算质量指标，并统计整个网格的质量范围。

目前支持以下 Cell 类型：

- 三角形（Triangle）
- 四边形（Quad）
- 四面体（Tetrahedron）
- 六面体（Hexahedron）

对于不同类型的网格，可以分别指定对应的质量评价指标：

- Triangle 质量指标
- Quad 质量指标
- Tetra 质量指标
- Hexahedron 质量指标

Filter 执行后：

1. 为每个可计算的 Cell 计算一个质量值；
2. 将所有 Cell 的质量值保存为名为 `Quality` 的 Cell 标量属性；
3. 统计可计算 Cell 的质量最小值 `Minimum`；
4. 统计可计算 Cell 的质量最大值 `Maximum`；
5. 统计可计算 Cell 的平均值 `Average`。

其中，在 Qt 界面中最终显示的主要结果为：

```text
Quality: [Minimum, Maximum]
```

即整个网格的质量值范围。Filter 同时保留每个 Cell 的具体 `Quality` 值以及平均质量值。

------

## 2. 调用方式

### 2.1 创建 Filter

首先创建 `MeshQualityFilter`：

```cpp
auto filter = iGame::MeshQualityFilter::New();
```

### 2.2 设置输入网格

使用 `SetInput()` 设置待计算的网格：

```cpp
filter->SetInput(obj);
```

其中 `obj` 为 `iGame::DataObject::Pointer` 类型的数据对象。

### 2.3 设置质量指标

可以根据网格 Cell 类型分别设置质量指标：

```cpp
filter->SetTriangleMetric(...);
filter->SetQuadMetric(...);
filter->SetTetMetric(...);
filter->SetHexMetric(...);
```

例如：

```cpp
filter->SetTriangleMetric(SurfaceMeshMetricsFilter::SurfaceMetric::ASPECT_RATIO);
filter->SetQuadMetric(SurfaceMeshMetricsFilter::SurfaceMetric::ASPECT_RATIO);
filter->SetTetMetric(VolumeMeshMetricsFilter::VolumeMetric::TET_ASPECT_RATIO);
filter->SetHexMetric(VolumeMeshMetricsFilter::VolumeMetric::HEX_JACOBIAN);
```

当前 Qt 界面提供的指标选项包括：

### Triangle

```text
FACE_AREA
MAX_ANGLE
MIN_ANGLE
JACOBIAN
ASPECT_RATIO
EDGE_RATIO
ANGLE_QUALITY
FACE_MIN_ANGLE
FACE_MAX_ANGLE
FACE_MIN_ANGLE_QUALITY
```

### Quad

```text
FACE_AREA
MAX_ANGLE
MIN_ANGLE
JACOBIAN
ASPECT_RATIO
EDGE_RATIO
WARPAGE
TAPER
SKEW
ANGLE_QUALITY
FACE_MIN_ANGLE
FACE_MAX_ANGLE
FACE_MIN_ANGLE_QUALITY
```

### Tetrahedron

```text
TET_EDGE_RATIO
TET_VOLUME
TET_ASPECT_RATIO
TET_JACOBIAN
TET_COLLAPSE_RATIO
TET_VOL_SKEW
TET_MIN_ANGLE
TET_EQUIANGLE_SKEWNESS
TET_INRADIUS
TET_CIRCUMRADIUS
TET_VOL_ASPECT_RATIO
TET_ASPECT_RATIO_ALT
TET_VOLUME_ALT
```

### Hexahedron

```text
HEX_VOLUME
HEX_TAPER
HEX_JACOBIAN
HEX_EDGE_RATIO
HEX_MAX_EDGE_RATIO
HEX_SKEW
HEX_STRETCH
HEX_DIAGONAL
HEX_RELATIVE_SIZE_SQUARED
HEX_MIN_SCALED_JACOBIAN
HEX_AVG_SCALED_JACOBIAN
HEX_VOLUME_ALT
```

Qt 界面中的这些指标分别对应 `SurfaceMeshMetricsFilter::SurfaceMetric` 和 `VolumeMeshMetricsFilter::VolumeMetric` 枚举值。

### 2.4 执行 Filter

设置完成后调用：

```cpp
if (!filter->Execute()) {
    // 执行失败
}
```

`Execute()` 会首先检查输入数据，然后根据输入数据类型获取对应的 Cell 和 Points，并进行质量计算。

------

## 3. 使用示例

下面给出一个完整的调用示例。

```cpp
#include <MeshQuality/iGameMeshQualityFilter.h>
#include <iGameFileIO.h>

#include <iostream>
#include <iomanip>

int main()
{
    // 读取测试模型
    const std::string fileName = "./Models/MeshQuality_Complex.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);

    if (obj == nullptr) {
        std::cout << "Read ERROR!" << std::endl;
        return 1;
    }

    // 创建 MeshQualityFilter
    auto filter = iGame::MeshQualityFilter::New();

    // 设置输入
    filter->SetInput(obj);

    // 执行质量评估
    if (!filter->Execute()) {
        std::cout << "MeshQualityFilter Execute ERROR!" << std::endl;
        return 1;
    }

    // 输出计算结果
    std::cout << std::fixed << std::setprecision(15);

    std::cout << "Mesh Quality Result" << std::endl;
    std::cout << "-------------------" << std::endl;

    std::cout << "NumberOfCells: "
              << filter->GetNumberOfCells()
              << std::endl;

    std::cout << "Minimum: "
              << filter->GetMinimum()
              << std::endl;

    std::cout << "Maximum: "
              << filter->GetMaximum()
              << std::endl;

    std::cout << "Average: "
              << filter->GetAverage()
              << std::endl;

    return 0;
}
```

该示例读取：

```text
./Models/MeshQuality_Complex.vtk
```

然后创建 `MeshQualityFilter`，设置输入数据并执行质量计算，最后输出 Cell 数量、质量最小值、最大值和平均值。

------

## 4. Qt 界面调用示例

在 iGameVis Qt 界面中，可以通过：

```text
Filters
└── 网格质量评估 (MeshQuality)
```

打开 MeshQuality 参数窗口。

界面分别提供：

```text
Triangle 质量指标
Quad 质量指标
Tetra 质量指标
Hexahedron 质量指标
```

选择指标后，创建 `MeshQualityFilter` 并设置对应指标：

```cpp
MeshQualityFilter::Pointer filter = MeshQualityFilter::New();

filter->SetInput(data);

filter->SetTriangleMetric(triangleMetricValues[triangleIndex]);
filter->SetQuadMetric(quadMetricValues[quadIndex]);
filter->SetTetMetric(tetMetricValues[tetIndex]);
filter->SetHexMetric(hexMetricValues[hexIndex]);

if (!filter->Execute()) {
    // 执行失败
}
```

执行成功后获取：

```cpp
double minQuality = filter->GetMinimum();
double maxQuality = filter->GetMaximum();
```

最终在界面显示：

```text
Quality: [Minimum, Maximum]
```

Qt 调用代码实际使用了四类指标选择框，并将用户选择的指标转换为对应的枚举值后传给 `MeshQualityFilter`。

------

## 5. 输出结果

### 5.1 Quality 属性

Filter 会创建名为：

```text
Quality
```

的标量数组，并将其作为 Cell 属性加入输入网格。

每一个支持计算的 Cell 对应一个质量值：

```text
Cell 0 → Quality 0
Cell 1 → Quality 1
Cell 2 → Quality 2
...
```

对于不支持的 Cell 类型或者无法得到有限质量值的 Cell，当前实现会将对应的 Quality 设置为 `NaN`。

### 5.2 Minimum

表示所有成功计算的 Cell 中的最小质量值：

```cpp
filter->GetMinimum();
```

### 5.3 Maximum

表示所有成功计算的 Cell 中的最大质量值：

```cpp
filter->GetMaximum();
```

### 5.4 Average

表示所有成功计算的 Cell 的平均质量值：

```cpp
filter->GetAverage();
```

当前实现虽然计算并保存了 `Average`，但 Qt 界面最终主要显示的是：

```text
Quality: [Minimum, Maximum]
```

平均值可以通过 Filter 接口获取。

------

## 6. 注意事项

### 6.1 输入数据要求

MeshQualityFilter 要求输入数据包含有效的：

- Cell
- Points

如果输入为空、没有 Cell 或没有 Points，Filter 会执行失败。

### 6.2 支持的网格类型

当前实现支持：

```text
SurfaceMesh
VolumeMesh
UnstructuredMesh
```

对于 SurfaceMesh：

```text
3 个顶点 → Triangle
4 个顶点 → Quad
```

对于 VolumeMesh：

```text
4 个顶点 → Tetrahedron
8 个顶点 → Hexahedron
```

UnstructuredMesh 则直接读取 Cell 的实际类型。

### 6.3 不支持的 Cell 类型

当前 MeshQualityFilter 只对：

```text
Triangle
Quad
Tetrahedron
Hexahedron
```

执行质量计算。

其他 Cell 类型暂时不会进行质量计算，对应 `Quality` 值设置为 `NaN`。

### 6.4 质量指标需要与 Cell 类型对应

不同类型的 Cell 使用不同的质量指标。

例如：

```cpp
SetTriangleMetric(...)
```

用于 Triangle；

```cpp
SetQuadMetric(...)
```

用于 Quad；

```cpp
SetTetMetric(...)
```

用于 Tetrahedron；

```cpp
SetHexMetric(...)
```

用于 Hexahedron。

因此在选择指标时，应根据实际网格 Cell 类型选择对应指标。

### 6.5 Quality 范围的含义

MeshQualityFilter 最终统计的是所有成功计算 Cell 的质量值范围：

```text
Quality: [Minimum, Maximum]
```

其中：

```text
Minimum = 所有有效 Cell Quality 中的最小值
Maximum = 所有有效 Cell Quality 中的最大值
```

因此该结果用于反映当前网格中质量指标的整体取值范围，而不是单独某一个 Cell 的质量。

### 6.6 输入数据会添加 Quality 属性

Filter 执行后会将 `Quality` 作为 Cell 标量属性加入输入数据对象，并通过 `SetOutput()` 设置输出对象。

在 iGameVis 中执行完成后，可以在模型属性/属性数组中查看 `Quality` 数据。

### 6.7 测试模型路径

示例程序当前使用：

```text
./Models/MeshQuality_Complex.vtk
```

因此运行示例程序时，需要保证测试模型位于示例程序所使用的相对路径下。

------

## 7. 总结

`MeshQualityFilter` 用于对网格进行综合质量评估。

其基本使用流程为：

```text
读取网格
   ↓
创建 MeshQualityFilter
   ↓
设置输入网格
   ↓
选择对应 Cell 类型的质量指标
   ↓
Execute()
   ↓
计算每个 Cell 的 Quality
   ↓
统计 Minimum / Maximum / Average
   ↓
输出 Quality 属性
```

在 iGameVis 图形界面中，可以分别选择 Triangle、Quad、Tetrahedron 和 Hexahedron 的质量指标，并执行网格质量评估。最终界面显示整个网格的：

```text
Quality: [Minimum, Maximum]
```

同时，网格中会增加 `Quality` Cell 属性，可进一步查看每个 Cell 的质量值。