# PointCoordinatesFilter 使用说明

## 1. 功能说明

`PointCoordinatesFilter` 用于把输入模型的点坐标提取为点属性数组。生成的数组默认名为 `Coordinates`，包含 X、Y、Z 三个分量，可像普通点属性一样用于查看、着色和后续处理。

该 Filter 生成输入数据对象的独立深拷贝作为输出，并在输出上添加坐标属性。原始输入模型不会被修改。

## 2. 输入与输出

- 输入：含有点坐标的 `DataObject`，例如 `UnstructuredMesh`、`SurfaceMesh` 或 `PointSet`。
- 输出：输入数据对象的独立深拷贝，包含原始所有点、单元和属性，以及新增的三分量点关联向量数组。
- 默认数组名：`Coordinates`。
- 数组元组数：与输入模型点数一致。

## 3. C++ 调用方式

```cpp
#include <PointCoordinates/iGamePointCoordinatesFilter.h>
#include <iGameFileIO.h>

auto input = iGame::FileIO::ReadFile(
    "./Models/PointCoordinatesFilter_Test.vtk");

auto filter = iGame::PointCoordinatesFilter::New();
filter->SetInput(input);
filter->SetArrayName("Coordinates"); // 可省略，默认即为 Coordinates

if (!filter->Execute()) {
    return 1;
}

auto output = filter->GetOutput();
auto coordinates = filter->GetCoordinatesArray();
```

执行成功后，可通过输入/输出对象的 `AttributeSet` 按名称取得该数组：

```cpp
auto& attribute = output->GetAttributeSet()->GetAttribute("Coordinates");
auto array = iGame::DynamicCast<iGame::FloatArray>(attribute.GetPointer());
```

## 4. GUI 使用方法

1. 在 iGameVis 中加载一个含点的模型并选中该模型。
2. 打开一级菜单“算法处理”。
3. 点击“提取点坐标 (Extract Point Coordinates)”。
4. 执行后，在模型的点属性信息中查看 `Coordinates` 数组及 X、Y、Z 分量。
5. 若模型点坐标被修改，重新执行该 Filter，数组内容和范围会同步刷新。

## 5. 自动测试示例

- 示例源码：`Examples/Filter/PointCoordinates/TestPointCoordinates.cpp`
- 测试模型：`Examples/Models/PointCoordinatesFilter_Test.vtk`
- 运行时相对路径：`./Models/PointCoordinatesFilter_Test.vtk`
- CMake 目标：`testPointCoordinates`

测试模型是为本 Filter 生成的不规则双四面体模型，共 5 个点、2 个单元，含正负值和小数坐标。示例内部写死相对路径，运行时无需手动输入文件名或参数。CMake 会把 `Examples/Models` 复制到示例构建目录。

示例自动检查：模型读取、坐标值及三分量结构、点关联方式、拓扑不变、独立输出节点、空数据和同名数组冲突处理。

## 6. 注意事项

- 输入对象必须具有点坐标；空点集允许生成空的三分量数组，但完全没有 `Points` 的对象会执行失败。
- 数组名不能为空。调用 `SetArrayName()` 可修改默认名称。
- 若输出中已经存在同名属性，Filter 会失败并保留原属性，避免静默覆盖数据。
- Filter 生成输入的独立深拷贝作为输出，原始输入不会被修改。
- `Coordinates` 是点属性，不是单元属性，数组元组顺序与模型点编号顺序一致。
