# GenerateProcessIds 使用说明

适用范围：iGameCore 生成进程号 Filter（类名 GenerateProcessIdsFilter）；Qt 界面入口为菜单「算法处理 → 生成进程ID (GenerateProcessIds)」。

## 功能

为网格生成进程号数组，用于观察或模拟并行计算的分区：点进程号写入 PointProcessIds，单元进程号写入 CellProcessIds，类型均为 long long。

语义对齐 VTK 的 `vtkGenerateProcessIds`：

- 所有点 / 单元的进程号都写**当前进程号**，不读取输入上已有的任何数组；
- 点进程号默认生成（`GeneratePointData` 默认开），单元进程号默认不生成（`GenerateCellData` 默认关），与 VTK 的默认值一致。

结果是一个新的数据对象——几何（点、单元）与输入共享，属性集为输入的拷贝加上新生成的进程号数组，输入对象本身不被修改。

进程号来源：

1. iGame 目前没有多进程 / 分区机制，进程号由 `SetProcessId()` 给出，默认 0；
2. 也可以继承本 Filter，重写受保护的 `GetPointProcessId(IGsize)` / `GetCellProcessId(IGsize)` 自定义分区策略（iGame 扩展，VTK 没有对应接口）。

支持的输入：非结构化网格、表面网格、体网格、结构化网格、Lagrange 网格等 PointSet 系网格。纯点云（PointSet）没有单元，勾选单元数据时该部分会被跳过。每次执行都返回独立的结果对象，输入始终不被修改，也不会在输入上累积重复属性。

单元计数：表面网格按面数，体网格按体数，非结构化 / 结构化 / Lagrange 网格按单元数。

## 与 VTK / ParaView 的差异

| 项目 | VTK vtkGenerateProcessIds / ParaView | iGameVis |
| --- | --- | --- |
| 进程号来源 | vtkMultiProcessController 的 local process id（串行时 0） | `SetProcessId()`（默认 0），无多进程机制 |
| 属性槽位 | 写入 PointData / CellData 的 ProcessIds 特殊属性槽 | 写入普通标量属性 PointProcessIds / CellProcessIds |
| 着色列表 | ProcessIds 不出现在 ParaView 的着色下拉列表中 | 会出现在模型树属性列表里，可按其着色（值全为同一常数，显示为单色） |
| 纯点云上的单元数据 | 单元数为 0，写出长度为 0 的 CellProcessIds | 跳过单元数据并给出提示，执行仍成功 |
| 单元数据默认值 | GenerateCellData 默认关 | 同样默认关 |

以上差异在制作 ParaView 对照录屏时需要说明。

## 调用方式

### C++ 接口

```cpp
#include <ProcessGet/iGameGenerateProcessIdsFilter.h>
```

```cpp
auto filter = iGame::GenerateProcessIdsFilter::New();
filter->SetInput(mesh);
filter->SetGeneratePointData(true);   // 生成点进程号，默认开
filter->SetGenerateCellData(true);    // 生成单元进程号，默认关
filter->SetProcessId(7);              // 当前进程号，默认 0
bool ok = filter->Execute();          // 失败时 GetMessage() 返回错误说明
// 结果为新的数据对象：几何与输入共享，GetOutput() 上带 PointProcessIds / CellProcessIds
```

自定义分区时继承并重写两个虚函数（iGame 扩展）：

```cpp
class MyPartitionFilter : public iGame::GenerateProcessIdsFilter {
public:
    I_OBJECT(MyPartitionFilter)
    static Pointer New() { return new MyPartitionFilter; }
protected:
    long long GetPointProcessId(IGsize index) override { return index % 2; }
    long long GetCellProcessId(IGsize index) override { return index % 2; }
};
```

### GUI 操作

1. 打开模型；
2. 菜单「算法处理 → 生成进程ID (GenerateProcessIds)」打开左侧面板；
3. 勾选「生成点数据」「生成单元数据」（默认只勾「生成点数据」，与 `GenerateCellData` 默认关一致），点击应用：首次应用在模型树新增结果节点（命名 原模型名_ProcessIds_序号），再次应用更新该节点、不新增；在模型树中选择 PointProcessIds / CellProcessIds 着色即可查看分区。

注意：

- 面板上没有进程号输入项，直接应用得到的是常数 0；需要其它常数请改用 C++ 接口。
- 结果属性是普通标量：进程号全为同一常数时着色为单色，这是预期现象。
- 纯点云上勾选「生成单元数据」会跳过单元部分并仍然成功；该提示通过 C++ 侧的 `GetMessage()` 读取，GUI 只在执行失败时弹框，因此面板上看不到这条提示。

## 使用示例

测试用例在 Examples/Filter/TestGenerateProcessIds.cpp（ctest 名 testGenerateProcessIds）。用例通过相对路径读取 AI 生成的测试模型，不需要任何参数即可完整运行：

```bash
# Windows：在 Examples 构建目录下
testGenerateProcessIds.exe
# 或
ctest -R testGenerateProcessIds --output-on-failure
```

全部通过时逐行打印 PASS 并以退出码 0 结束，任一失败打印 FAIL、退出码为 1。用例在两个 AI 测试模型（三节变径管、文丘里缩放喷管）上执行常数、分区、重复执行三类校验，另校验：结果独立成节点（几何共享、输入未被修改、结果中同名数组唯一）、输入自带的 process_id 数组被忽略（结果仍是当前进程号）、表面网格与体网格的单元进程号、纯点云跳过单元数据、两个开关都关时报错。第一个模型路径可以用第一个命令行参数覆盖，便于换模型验证。

代码片段（读取 Examples/Models/GenerateProcessIds_SteppedPipe.vtk，生成常数 7 的点/单元进程号）：

```cpp
#include <iostream>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>
#include <ProcessGet/iGameGenerateProcessIdsFilter.h>

int main() {
    auto data = iGame::FileIO::ReadFile("./Models/GenerateProcessIds_SteppedPipe.vtk");
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(data);
    if (mesh == nullptr) return 1;

    auto filter = iGame::GenerateProcessIdsFilter::New();
    filter->SetInput(mesh);
    filter->SetGeneratePointData(true);
    filter->SetGenerateCellData(true);
    filter->SetProcessId(7);
    if (!filter->Execute()) { std::cout << filter->GetMessage() << "\n"; return 1; }

    auto output = filter->GetOutput();
    auto pt = output->GetAttributeSet()->GetScalar("PointProcessIds");
    auto cell = output->GetAttributeSet()->GetScalar("CellProcessIds");
    std::cout << pt.pointer->GetNumberOfElements() << " points, value "
              << pt.pointer->GetValue(0) << "\n";
    std::cout << cell.pointer->GetNumberOfElements() << " cells, value "
              << cell.pointer->GetValue(0) << "\n";
    return 0;
}
```

## 注意事项

1. 结果为新的数据对象：几何与输入共享，输入对象不被修改；GUI 会把结果作为独立节点加入模型树（首次应用新增、再次应用更新同一节点；删除该节点后再次应用会重新生成）。
2. 结果数组名固定为 PointProcessIds / CellProcessIds，类型 long long；读取时先取 `GetOutput()`，再用 `GetScalar("PointProcessIds")` 等接口。
3. 进程号只来自 `SetProcessId()`（默认 0）或派生类重写的 `GetPointProcessId` / `GetCellProcessId`；输入上即使存在名为 process_id 的数组也不会被读取（该数组会随属性集原样拷入结果，但不影响生成的进程号）。
4. 点数据与单元数据两个开关都关时，执行失败并给出提示 `Neither point nor cell process ids were requested.`。
5. 纯点云（PointSet）勾选单元数据时：点进程号照常生成，单元部分跳过并在 `GetMessage()` 中给出提示，执行返回成功。
6. 单元计数：表面网格按面数，体网格按体数，非结构化 / 结构化 / Lagrange 网格按单元数。
7. 示例按相对路径 ./Models/... 读取模型，需在 Examples 构建目录下运行；在其它目录运行请改用完整路径。

## 测试模型

两个模型放在 Examples/Models，由脚本程序化生成，几何均可复算：

| 文件 | 内容 |
| --- | --- |
| GenerateProcessIds_SteppedPipe.vtk | 三节变径管道（含两处锥形过渡）：432 点 / 264 六面体。带点标量 layer、单元标量 zone（0/1/2 三段） |
| GenerateProcessIds_VenturiTube.vtk | 文丘里缩放喷管（光滑喉道）：270 点 / 960 四面体。带点标量 layer、单元标量 zone |

两个模型自带的 layer/zone 标量把几何分成三段，直观对应多进程分区的场景；模型本身不带 process_id 数组，直接应用即生成常数进程号。用例会在模型上临时附加一个 process_id 数组，用来校验它不会影响生成的进程号。
