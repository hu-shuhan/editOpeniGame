# FeatureEdgesFilter 使用说明

## 1. 功能概述

`FeatureEdgesFilter` 用于从表面网格（`SurfaceMesh`）中提取特征边，并输出由线段组成的 `UnstructuredMesh`。Filter 支持以下四类边：

- 边界边：只属于一个面的边；
- 特征边：属于两个面的边，且相邻面法向夹角不小于设定的特征角度；
- 非流形边：同时属于三个或更多面的边；
- 普通流形边：属于两个面且法向夹角小于特征角度的边。

输出结果包含两个单元属性：

- `Edge Types`：边类型编号，`0` 为边界边，`1` 为特征边，`2` 为非流形边，`3` 为普通流形边；
- `Edge Ids`：输出线段对应的原始表面网格边编号。

## 2. 调用方式

```cpp
#include <FeatureExtraction/iGameFeatureEdgesFilter.h>

auto filter = iGame::FeatureEdgesFilter::New();
filter->SetInput(surfaceMesh);
filter->SetFeatureAngle(30.0);
filter->SetBoundaryEdges(true);
filter->SetFeatureEdges(true);
filter->SetNonManifoldEdges(true);
filter->SetManifoldEdges(false);

if (!filter->Execute()) {
    // 处理输入网格为空或没有符合条件的边等错误
}

auto output = filter->GetOutput();
```

输入必须是 `SurfaceMesh`。如果手头是体网格或其他非表面网格，应先执行“表面提取”，再将表面网格传入本 Filter。

## 3. 使用示例

项目提供了两个自动测试示例：

- `testFeatureEdges`：自动读取以下两个模型并验证输出边数和属性；
- `testFeatureEdgesVisualization`：自动读取立方体模型并打开窗口显示输入表面和提取出的特征边。

测试模型通过相对路径自动读取，无需输入命令行参数：

```text
./Models/FeatureEdges_Cube.vtk
./Models/FeatureEdges_NonManifold.vtk
```

其中：

- `FeatureEdges_Cube.vtk` 为立方体表面模型，默认提取出 12 条特征边；
- `FeatureEdges_NonManifold.vtk` 为三个三角面共用一条边的非流形模型，默认提取出 6 条边界边和 1 条非流形边，共 7 条边。

编译并运行：

```powershell
cmake --build build --config Release --target testFeatureEdges
build\Examples\Release\testFeatureEdges.exe
```

运行成功时会输出：

```text
ALL FEATURE EDGES TESTS PASSED
```

## 4. 注意事项

1. Filter 不会自动将体网格转换为表面网格；体网格应先执行表面提取。
2. 特征角度的有效范围为 `0` 到 `180` 度，默认值为 `30` 度。
3. 如果所有边类型选项均关闭，或输入网格没有符合条件的边，`Execute()` 会返回失败。
4. 输出为线网格，渲染时可使用 `Edge Types` 属性进行分类着色。
5. 示例程序默认从可执行文件所在目录下的 `Models` 子目录读取模型，运行时请确保模型已部署到该目录。
