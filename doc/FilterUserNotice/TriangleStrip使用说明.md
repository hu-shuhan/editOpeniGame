# TriangleStrip 使用说明

## 功能

`iGame::TriangleStripFilter` 将共享边的相邻三角形组织为三角带，并处理输入中已有的 `IG_LINE`/`IG_POLY_LINE` 单元。开启连续片段合并后，首尾点 ID 相同的输入线段会被连接为 `IG_POLY_LINE` 折线。与 ParaView 的 `vtkStripper` 一致，过滤器不会把三角面边界自动转换成折线。

算法采用逐面访问和贪心延伸方式：从一个未处理三角形出发，按面内点/边顺序选择第一个可用的相邻三角形，再沿带尾继续延伸。该起始方向与 ParaView 5.12 使用的 `vtkStripper` 9.3 一致。

Filter 的输出同时保存两种互补表示：

- `SurfaceMesh::GetFaces()` 保存展开后的普通三角面，供现有渲染和 Filter 继续使用。
- 正式输出对象的 `Metadata` 保存原生三角带点 ID 序列及其输入 face ID 映射。

原生三角带已经属于正式输出对象，不依赖 `TriangleStripFilter` 的生命周期。调用 `TriangleStripFilter::ReadOutputStrips()` 可以从输出中重建两个 `CellArray`。当前渲染器仍读取 `Faces`，保存三角带不要求修改渲染流程，也不需要修改 `SurfaceMesh` 数据结构。

## 输入与输出

核心 Filter 支持以下输入：

- `SurfaceMesh`。
- 包含二维表面单元以及可选 `IG_LINE`/`IG_POLY_LINE` 单元的 `UnstructuredMesh`。

体网格应先提取表面，再进行三角化。非三角形表面不会参与三角带生成，而是通过 `GetPassThroughPolys()` 原样保留。

可读取的结果包括：

| 接口 | 返回内容 |
| --- | --- |
| `GetOutput()` | 正式 `SurfaceMesh`，Faces 为普通三角面，Metadata 保存三角带和源面映射 |
| `TriangleStripFilter::ReadOutputStrips()` | 从正式输出 Metadata 重建三角带及源 face ID 两个 `CellArray` |
| `TriangleStripFilter::GetStrips()` | 执行期间生成的原生三角带 |
| `TriangleStripFilter::GetStripSourceFaceIds()` | 执行期间生成的三角形到输入 face ID 映射 |
| `GetPassThroughPolys()` | 未参与三角带生成的非三角形面 |
| `GetPolyLines()` | 输入中已有的线/折线单元，或合并后的连续折线 |
| `GetNumberOfStrips()` | 生成的三角带数量 |
| `GetNumberOfOutputCells()` | ParaView Cells 口径：三角带 + 透传多边形 + 输出折线 |
| `GetLongestStripLength()` | 最长三角带包含的三角形数量 |

输出表面会创建新的 `AttributeSet`。PointData 保留原数组和点对应关系；CellData 按输出三角面的源 face ID 重新排列。

## 参数说明

### 最长三角带长度

```cpp
filter->SetMaximumLength(1000);
```

`MaximumLength` 表示单条三角带最多包含的三角形数量，不是点 ID 数量。默认值为 `1000`，小于 `1` 的输入会被限制为 `1`。

例如，一条包含 8 个三角形的完整带：

- `MaximumLength = 1000` 时可以输出 1 条长度 8 的带。
- `MaximumLength = 4` 时会被拆分为 2 条长度 4 的带。

### 合并连续片段

```cpp
filter->SetJoinContiguousSegments(true);
```

- `false`：保持输入中已有的 `IG_LINE`/`IG_POLY_LINE` 单元。
- `true`：比较输入线/折线的首尾点 ID，必要时反转方向后拼接为连续折线。

闭合输入线的合并结果会重复起点。例如，由 10 条连续线段组成的闭合环会生成一条包含 11 个点 ID 的折线，且第一个和最后一个点 ID 相同。

该选项只处理输入中已有的线单元，不会把不同三角带连接在一起，也不会提取表面边界。

## 调用方式

已经得到三角化 `SurfaceMesh` 时，可以直接调用：

```cpp
#include <TriangleStrip/iGameTriangleStripFilter.h>

#include <iostream>

void BuildStrips(const iGame::SurfaceMesh::Pointer& triangles) {
    using namespace iGame;

    auto filter = TriangleStripFilter::New();
    filter->SetInput(triangles);
    filter->SetMaximumLength(1000);
    filter->SetJoinContiguousSegments(true);

    if (!filter->Execute()) {
        std::cerr << "三角带转换失败。\n";
        return;
    }

    auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    CellArray::Pointer strips;
    CellArray::Pointer stripSourceFaceIds;
    if (!TriangleStripFilter::ReadOutputStrips(
                output, strips, stripSourceFaceIds)) {
        std::cerr << "无法读取正式输出中的三角带。\n";
        return;
    }
    for (IGsize stripId = 0; stripId < strips->GetNumberOfCells(); ++stripId) {
        const igIndex* pointIds = nullptr;
        const int pointCount = strips->GetCellIds(stripId, pointIds);
        const int triangleCount = pointCount - 2;

        const igIndex* sourceFaceIds = nullptr;
        const int sourceFaceCount = stripSourceFaceIds->GetCellIds(
                stripId, sourceFaceIds);
        if (sourceFaceCount != triangleCount) {
            std::cerr << "三角带与源面映射不一致。\n";
            return;
        }

        std::cout << "strip " << stripId
                  << ": points=" << pointCount
                  << ", triangles=" << triangleCount << '\n';
    }

    auto* inputLines = filter->GetPolyLines();
    std::cout << "input polylines="
              << inputLines->GetNumberOfCells() << '\n';
    std::cout << "ParaView-compatible output cells="
              << filter->GetNumberOfOutputCells() << '\n';
}
```

## 从文件读取并预处理

测试模型是二维 `UnstructuredMesh`。下面展示完整的读取、表面转换、三角化和三角带生成流程：

```cpp
#include <Convert/iGameConvertToSurfaceMeshFilter.h>
#include <DataProcessing/iGameMeshTriangulationFilter.h>
#include <TriangleStrip/iGameTriangleStripFilter.h>
#include <iGameFileIO.h>

int main() {
    using namespace iGame;

    auto input = FileIO::ReadFile("Models/TriangleStripTestModel.vtk");
    if (!input) {
        return 1;
    }

    auto surfaceFilter = ConvertToSurfaceMeshFilter::New();
    surfaceFilter->SetInput(input);
    surfaceFilter->SetConvertMethod(
            ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_MESH);
    if (!surfaceFilter->Execute()) {
        return 1;
    }

    auto triangulation = MeshTriangulationFilter::New();
    triangulation->SetInput(surfaceFilter->GetOutput());
    if (!triangulation->Execute()) {
        return 1;
    }

    auto triangles = DynamicCast<SurfaceMesh>(triangulation->GetOutput());
    auto filter = TriangleStripFilter::New();
    filter->SetInput(triangles);
    filter->SetMaximumLength(1000);
    filter->SetJoinContiguousSegments(true);
    if (!filter->Execute()) {
        return 1;
    }

    CellArray::Pointer strips;
    CellArray::Pointer stripSourceFaceIds;
    if (!TriangleStripFilter::ReadOutputStrips(
                filter->GetOutput(), strips, stripSourceFaceIds)) {
        return 1;
    }

    return 0;
}
```

完整测试示例位于：

```text
Examples/Filter/TriangleStrip/TestTriangleStrip.cpp
Examples/Filter/TriangleStrip/TestTriangleStripWidget.cpp
```

两个程序都固定读取 `Models/TriangleStripTestModel.vtk`。该模型包含 10 个点和 8 个连续三角形，默认生成 1 条长度 8 的三角带。核心测试还会读取较大且已经由三角形组成的 `Models/SurfaceNormalsFilter_test.vtk`，避免两端三角化策略差异干扰对比，并校验 2752 个三角形在 `MaximumLength = 1000` 时与 ParaView 5.12 一样产生 381 个输出 Cells。小模型不含显式线单元，因此即使表面是开放的，`GetPolyLines()` 也应返回 0 个单元，UI 不会额外发布折线模型。测试程序还会在内存中添加 4 条连续的 `IG_LINE`，验证开启合并后得到 1 条包含 5 个点 ID 的开放折线。

## 注意事项

1. 输入表面应先完成三角化。非三角形面只会进入 `GetPassThroughPolys()`，不会形成三角带。
2. 正式输出的 `SurfaceMesh` 在 `Faces` 中保存展开后的三角面，在 `Metadata` 中保存三角带及其源面映射。后续处理应使用 `ReadOutputStrips()`，而不是长期持有 Filter 内部指针。
3. 对第 `i` 条重建三角带，必须满足 `stripSourceFaceIds->GetCellSize(i) == strips->GetCellSize(i) - 2`。映射顺序与该带展开三角形的顺序一致。
4. Metadata 内部使用 `TriangleStripOffsets`、`TriangleStripPointIds`、`TriangleStripSourceFaceOffsets` 和 `TriangleStripSourceFaceIds` 四个扁平 `IntArray`，不要单独修改其中一个数组。
5. `GetPolyLines()` 和 `GetPassThroughPolys()` 仍由 Filter 管理，需要长期保存时应复制相应数据。
6. 三角带按网格返回的第一个边邻面继续延伸。遇到边界、已处理面或第一个邻面不是三角形时，当前条带终止；这一顺序与 `vtkStripper` 一致。
7. 当前算法是依赖输入面顺序的贪心算法，不保证得到全局最少的三角带数量。
8. `SetJoinContiguousSegments(true)` 只按点 ID 连通关系连接输入线单元，不进行几何距离容差合并；坐标相同但点 ID 不同的端点不会连接。
9. 三角面边界不会进入 `GetPolyLines()`；只有输入中明确存在的 `IG_LINE`/`IG_POLY_LINE` 才会进入折线处理。
10. PointData 会保持点对应关系；CellData 会根据三角带展开后的源面映射重排。调用者不应假设输出面顺序与输入面顺序完全一致。
11. UI 会先尝试提取表面并三角化；直接使用核心 Filter 时，调用者需要自行保证输入是表面网格。
12. UI 中“三角带数量”只统计 strips；“输出 Cell 数（ParaView 口径）”还会加上透传多边形和输出折线。纯三角面输入时两者数值相同。
