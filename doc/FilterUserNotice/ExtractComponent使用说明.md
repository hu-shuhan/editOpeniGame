# ExtractComponent 使用说明

适用范围：iGameCore 提取分量 Filter（类名 ExtractComponentFilter）；Qt 界面入口为菜单「算法处理 → 提取分量 (Extract Component)」，弹出独立面板（不占用左侧工具面板）。

## 功能

从输入网格的点数据或单元数据里取出一个多维数组的 1~3 个分量，生成新的数组，并返回一个新的数据对象：

- 输出数组维度 = 提取的分量个数（1~3），与 VTK `vtkImageExtractComponents` 的 `SetComponents` 语义一致；
- 输出数组的具体类型与输入数组一致（float、int、long long 等各自保持），避免类型转换与精度丢失；
- 输出对象与输入共享几何，输入对象本身不被修改。

数组选择规则：

- 不指定输入数组名时，取属性集中第一个匹配挂载限制的数组（顺序由属性集决定，一般点数据在前）；
- 可按数组名指定；点、单元存在同名数组时，用挂载类型（IG_POINT / IG_CELL）进一步区分；
- 分量按 0 起始索引，0/1/2 即 X/Y/Z；索引为负或超出数组维度时执行失败，原因通过 GetMessage() 读取。

属性集拷贝策略：

- 默认**深拷贝**输入属性（结果与输入完全解耦，之后改动输入不会影响结果）。VTK 的类文档同样写明 "It does involve a copy of the data"；
- 大模型上更在意内存与耗时，可显式调用 `SetShallowCopyAttributes(true)` 改为只读共享输入数组（iGame 扩展：结果只新增自己的数组，从不修改已有数组）；
- 两种模式下，提取出来的结果数组都是新建的，与输入数组无关。

输出数组默认名 Result，可自定义；名字为空时执行失败；与输入已有数组重名时执行覆盖，结果属性集中该名字唯一。目前支持非结构化网格和表面网格。

## 与 VTK / ParaView 的差异

| 项目 | VTK vtkImageExtractComponents / ParaView | iGameVis |
| --- | --- | --- |
| 输入 | vtkImageData，只有一个标量数组，分量即像素通道 | 通用网格，属性集可能有多个数组、点/单元两类 → 必须按「名字 + 挂载类型」选数组（iGame 特有接口） |
| 分量数 | 1~3（`SetComponents(c1)` / `(c1,c2)` / `(c1,c2,c3)`） | 相同（已对齐） |
| 输出类型 | 与输入一致 | 与输入一致 |
| 数据拷贝 | 类文档明确 "involves a copy of the data" | 默认深拷贝；可用 `SetShallowCopyAttributes(true)` 改为只读共享（iGame 扩展） |
| 向量场拆分 | 官方示例 `ExtractComponents.cxx` 的用法：同一输入建多个 filter，分别 `SetComponents(0)`/`(1)`/`(2)` | 同样做法：连跑 `SetComponent(0/1/2)`，或用 `SetComponents` 一次取多个分量 |
| GUI | 面板里选择分量 | 面板含「输入数组（点/单元标注）+ 输出数组名 + 分量」；GUI 目前只提供**单分量**，多分量提取走 C++ 接口；1 维数组时分量下拉禁用 |

## 调用方式

### C++ 接口

```cpp
#include <Attribute/iGameExtractComponentFilter.h>
```

```cpp
auto filter = iGame::ExtractComponentFilter::New();
filter->SetInput(mesh);                    // 输入 DataObject
filter->SetInputArrayName("V");            // 数组名；留空 = 取第一个匹配挂载限制的数组
filter->SetInputAttachmentType(IG_POINT);  // 可选：IG_POINT / IG_CELL，默认不限定
filter->SetOutputArrayName("ResultY");     // 输出数组名，默认 Result；不能为空
filter->SetComponent(1);                   // 单分量：0/1/2 = X/Y/Z
bool ok = filter->Execute();               // 失败时 GetMessage() 返回错误说明
auto out = filter->GetOutput();            // 结果数据对象（几何与输入共享）
```

一次提取多个分量（输出维度 = 分量个数，分量顺序按参数保留）：

```cpp
auto filter = iGame::ExtractComponentFilter::New();
filter->SetInput(mesh);
filter->SetInputArrayName("V");
filter->SetOutputArrayName("Vxy");
filter->SetComponents(0, 1);        // 2 个分量；也可 SetComponents(0, 1, 2)
filter->Execute();

int n = filter->GetNumberOfComponents();       // 2
const int* comps = filter->GetComponents();    // comps[0] == 0, comps[1] == 1
auto arr = filter->GetOutput()->GetAttributeSet()->GetScalar("Vxy").pointer;
// arr->GetDimension() == 2；取第 i 个元素的第 c 个分量：arr->GetElementValue(i, c)
```

属性集拷贝策略（默认深拷贝）：

```cpp
filter->SetShallowCopyAttributes(true);   // 只读共享输入数组，省内存；默认 false = 深拷贝
```

### GUI 操作

1. 打开模型（在模型树中选中它）；
2. 菜单「算法处理 → 提取分量 (Extract Component)」→ 弹出**独立面板**：非模态、始终置顶，不点右上角 X 不会消失，关闭后再次打开复用同一面板；
3. 在面板里选择输入数组（下拉项以 (Point)/(Cell) 标注点/单元数组）、输出数组名和分量；
4. 点「应用」：首次执行在模型树新增结果节点（命名 原模型名_ExtractComponent_序号），对同一输入再次执行则更新该节点，不重复新建。结果节点可继续作为其它 Filter 的输入。

## 使用示例

测试用例在 Examples/Filter/TestExtractComponent.cpp（ctest 名 testExtractComponent）。用例通过相对路径读取 AI 生成的测试模型，不需要任何参数即可完整运行：

```bash
# Windows：在 Examples 构建目录下
testExtractComponent.exe
# 或
ctest -R testExtractComponent --output-on-failure
```

全部通过时逐行打印 PASS 并以退出码 0 结束，任一失败打印 FAIL、退出码为 1。用例覆盖：

- 两个 AI 测试模型的真实数据校验（空名取首向量、显式名取指定分量、按 VTK 官方示例的做法把 0/1/2 三个分量分别提取后逐路核对）；
- 程序化网格上的分量边界（1 维 / 2 维数组取不存在的分量要失败、负分量要失败）、多分量提取（`SetComponents` 的维度与顺序）、属性集拷贝策略（默认深拷贝解耦、开关打开后共享）、输出名为空要失败、重名覆盖、点/单元挂载区分、输出类型保持（Int / LongLong）、对结果再次提取的回归。

代码片段（读取 Examples/Models/ExtractComponent_FlowPipe.vtk，提取点向量 V 的 Y 分量）：

```cpp
#include <iostream>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>
#include <Attribute/iGameExtractComponentFilter.h>

int main() {
    auto data = iGame::FileIO::ReadFile("./Models/ExtractComponent_FlowPipe.vtk");
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(data);
    if (mesh == nullptr) return 1;

    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    filter->SetInputArrayName("V");
    filter->SetOutputArrayName("ResultY");
    filter->SetComponent(1);
    if (!filter->Execute()) { std::cout << filter->GetMessage() << "\n"; return 1; }

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    auto arr = out->GetAttributeSet()->GetScalar("ResultY").pointer;
    for (IGsize i = 0; i < arr->GetNumberOfElements(); ++i)
        std::cout << arr->GetValue(i) << "\n";
    return 0;
}
```

## 注意事项

1. 分量索引从 0 起；1 维数组只能取 X（GUI 会禁用分量下拉），多分量请用 `SetComponents`。
2. 不填输入数组名 = 取第一个匹配挂载限制的数组，顺序由属性集决定（一般点数据在前）；同名点/单元数组请显式指定挂载类型。
3. 输出是新对象，输入对象不被修改；几何（点、单元）为共享引用，不要假定输出持有独立几何。
4. 结果属性集默认深拷贝输入属性，结果与输入解耦；`SetShallowCopyAttributes(true)` 改为只读共享，省内存但输入被修改时结果会同步变化。
5. 输出数组名为空 → 执行失败；与输入已有数组重名 → 覆盖，结果集中的该名字唯一（不会残留两个同名属性）。
6. 输入仅支持非结构化网格与表面网格。
7. 面板是置顶独立窗口，点 X 才关闭；关闭后再打开复用同一面板与同一结果节点。
8. 示例按相对路径 ./Models/... 读取模型，需在 Examples 构建目录下运行；在其它目录运行请改用完整路径。

## 测试模型

两个模型放在 Examples/Models，由脚本程序化生成，几何与流场分布均可复算：

| 文件 | 内容 |
| --- | --- |
| ExtractComponent_FlowPipe.vtk | 直管道（环形流道）：576 点 / 384 六面体。点向量 V（切向涡 + 轴向流）、点标量 Pressure（沿程递减）；单元向量 cellV、单元标量 cellStress |
| ExtractComponent_BendPipe.vtk | 90° 弯管段：210 点 / 720 四面体。点向量 V（随弯转方向旋转）、点标量 Pressure；单元向量 cellV、单元标量 cellStress |

两个模型均以点向量 V（float、3 分量）作为第一个向量属性，并同时带点、单元两类数组，便于验证点/单元两条提取路径与弯道中分量方向的真实变化。
