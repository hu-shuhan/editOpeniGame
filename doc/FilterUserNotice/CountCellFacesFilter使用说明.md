# CountCellFacesFilter 使用说明

## 功能

统计网格中每个单元的三维面数，计算结果为单元标量属性 `cellFaceCounts`，可从 GetOutput() 返回的数据中读取。

| 单元 | 面数 |
| --- | --- |
| 四面体 | 4 |
| 六面体 | 6 |
| 金字塔 | 5 |
| 棱柱 | 5 |
| 有效多面体 | 根据面连接信息计数 |
| 点、线、三角形、四边形等低维单元 | 0 |

支持 UnstructuredMesh、VolumeMesh、SurfaceMesh、二维及三维 StructuredMesh，以及可识别的 LagrangeUnstructuredMesh。已识别的二次和 Lagrange 单元按对应基础拓扑计数。

## 调用方式

```cpp
#include "FeatureExtraction/iGameCountCellFacesFilter.h"
#include "iGameFileIO.h"

auto input = iGame::FileIO::ReadFile("Models/CountCellFaces_MixedCells.vtk");
if (!input) return 1;
auto filter = iGame::CountCellFacesFilter::New();
filter->SetInput(input);
if (!filter->Execute()) return 1;
auto output = filter->GetOutput(); // 结果数据，含 cellFaceCounts
auto counts = filter->GetResult(); // 按单元顺序排列的 unsigned int 数组
```

Execute 返回成功与否；仅在成功后使用输出。调用者应通过 GetOutput() 获取结果，不要假定其与输入的对象关系。

## 使用示例

源码：`Examples/Filter/FeatureExtraction/CountCellFaces.cpp`。

在已配置 Examples 的构建目录中构建：

```powershell
cmake --build build --target testCountCellFaces
cd build/Examples
./testCountCellFaces.exe
```

Linux 下运行 `./testCountCellFaces`。示例无需参数，无交互输入，使用固定相对路径 `Models/...`。现有 CMake 资源复制步骤会将测试模型复制到 example 运行目录。手动运行时须在包含 Models 子目录的 example 目录执行；CTest 已设置该工作目录：

```powershell
ctest --test-dir build/Examples -R "^testCountCellFaces$" --output-on-failure
```

自动加载的模型：

| 文件（位于 Examples/Models） | 内容与预期 |
| --- | --- |
| CountCellFaces_MixedCells.vtk | 四面体、六面体、金字塔、棱柱、三角形、四边形、线，依次输出 4、6、5、5、0、0、0；附带整数材料属性 |
| CountCellFaces_QuadTensor.vtk | 两个四边形，面数均为 0；附带点张量和单元整数属性 |

这两个小型合成模型的预期值由明确拓扑确定，不依赖 Tet_Plane.vtk。示例逐项核对结果长度、各单元面数以及 cellFaceCounts 的单元属性关联。终端输出各单元数值和 PASS，全部通过返回 0，失败返回非零。

## 注意事项

- 这里统计的是单元的三维面数，不是网格整体的外表面数，也不是二维单元的边数。
- 未支持的单元类型输出 0 并记录日志；不支持的数据对象类型或无输入时 Execute 返回 false。
- 多面体必须提供有效连接信息；本 filter 不修复损坏的输入拓扑。
- 大模型的执行时间和结果数组内存开销随单元数量增长。
- 错误使用项目现有 igError 记录，提示使用 igDebug；可通过 GetMessage() 获取此次执行说明。日志由项目日志配置管理。
