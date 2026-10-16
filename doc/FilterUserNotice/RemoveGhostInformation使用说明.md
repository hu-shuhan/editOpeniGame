# Remove Ghost Information 使用说明

## 1. 功能说明

Remove Ghost Information 用于移除网格中的 Ghost 信息。

对于带有 Cell 级 `vtkGhostType` 的非结构网格，Filter 会根据 Ghost 标记判断需要删除的 Cell，并重新构建输出网格。删除 Ghost Cell 后，不再被保留 Cell 使用的 Point 也会被清理，同时重新建立 Point ID 和 Cell ID 的映射关系。

输出结果中不会继续保留 `vtkGhostType` 属性，与保留网格对应的普通 Point Data 和 Cell Data 会同步复制到新的输出网格中。

当前实现已验证以下典型 Cell Ghost 类型：

```text
0  = Normal Cell
1  = DUPLICATECELL
32 = HIDDENCELL
```

其中 `DUPLICATECELL` 和 `HIDDENCELL` 会被移除。

核心测试已经覆盖 `DUPLICATECELL`、`HIDDENCELL`、Point Remapping、Point Attribute、Cell Attribute、Attribute Types、Point Ghost Only、Int64 Precision 和无 Ghost 属性等情况。

## 2. 调用方式

### 2.1 GUI 调用

在 iGameVis 中打开包含 Ghost 信息的模型，并在模型树中选中该模型。

然后通过菜单执行：

```text
算法处理 -> Remove Ghost Information
```

执行成功后，新的结果对象会加入模型树。

如果输入模型不存在 Point 或 Cell 级 `vtkGhostType`，则不会创建无意义的重复结果节点。

### 2.2 C++ 调用

使用 Filter 时包含头文件：

```cpp
#include <RemoveGhostInformation/iGameRemoveGhostInformationFilter.h>
```

创建并执行 Filter：

```cpp
auto filter = iGame::RemoveGhostInformationFilter::New();

filter->SetInput(0, input);

if (!filter->Execute()) {
    return;
}

auto output =
        iGame::DynamicCast<iGame::UnstructuredMesh>(
                filter->GetOutput());

if (output.IsNull()) {
    return;
}
```

对于 Legacy VTK 文件中的 Ghost 数据，应使用项目提供的 Ghost VTK Reader 读取：

```cpp
#include <VTK/iGameGhostVTKReader.h>

const std::string fileName =
        "./Examples/Models/RemoveGhostInformation_AI_Test.vtk";

auto obj = iGame::GhostVTKReader::ReadFile(fileName);
```

该 Reader 会读取 Legacy ASCII VTK 中 Cell Data 的 `vtkGhostType`，并以适合 Remove Ghost Information Filter 使用的 Ghost 数组形式保存到数据对象中。

## 3. 使用示例

测试模型：

```text
Examples/Models/RemoveGhostInformation_AI_Test.vtk
```

模型包含两个相互独立的四面体，共：

```text
Points = 8
Cells = 2
```

两个 Cell 的 Ghost 信息分别为：

```text
Cell 0:
vtkGhostType = 0

Cell 1:
vtkGhostType = 1
```

其中 Cell 0 为正常单元，Cell 1 为 `DUPLICATECELL`。

测试模型同时包含普通属性：

```text
PointValue:
100, 101, 102, 103, 104, 105, 106, 107

CellValue:
10, 20
```

执行 Remove Ghost Information 后，第二个四面体被删除，因此输出结果为：

```text
Points = 4
Cells = 1
```

保留的普通属性为：

```text
PointValue:
100, 101, 102, 103

CellValue:
10
```

同时：

```text
vtkGhostType
```

不会继续保留在输出数据中。

完整调用示例：

```cpp
#include <RemoveGhostInformation/iGameRemoveGhostInformationFilter.h>

#include <VTK/iGameGhostVTKReader.h>
#include <iGameUnstructuredMesh.h>

#include <string>

int main() {
    const std::string fileName =
            "./Examples/Models/RemoveGhostInformation_AI_Test.vtk";

    auto obj = iGame::GhostVTKReader::ReadFile(fileName);

    if (obj.IsNull()) {
        return 1;
    }

    auto input =
            iGame::DynamicCast<iGame::UnstructuredMesh>(obj);

    if (input.IsNull()) {
        return 1;
    }

    auto filter =
            iGame::RemoveGhostInformationFilter::New();

    filter->SetInput(0, input);

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

## 4. Ghost Cell 处理

`vtkGhostType` 是 Ghost 状态的标记数组。

当前核心测试已验证以下两种典型 Ghost Cell：

### DUPLICATECELL

```text
vtkGhostType = 1
```

该 Cell 会从输出网格中删除。

例如输入：

```text
2 Cells
8 Points
```

其中一个 Cell 为 `DUPLICATECELL`，输出为：

```text
1 Cell
4 Points
```

### HIDDENCELL

```text
vtkGhostType = 32
```

该 Cell 同样会从输出网格中删除。

核心测试已经分别验证了 `vtkGhostType = 1` 和 `vtkGhostType = 32` 的删除行为。:contentReference[oaicite:0]{index=0}

## 5. Point 重映射

删除 Ghost Cell 后，Filter 会检查剩余 Cell 实际使用的 Point。

未被任何保留 Cell 使用的 Point 不会进入输出网格。

因此输出网格会重新建立 Point ID 映射：

```text
旧 Point ID
    ↓
筛选仍然被有效 Cell 使用的 Point
    ↓
生成连续的新 Point ID
```

与这些 Point 对应的 Point Attribute 也会按照新的 Point ID 同步重映射。

对于本说明中的测试模型：

```text
原 Point 0,1,2,3 -> 保留
原 Point 4,5,6,7 -> 删除
```

最终形成4个新的有效 Point。

## 6. 属性处理

Remove Ghost Information 在重新构建网格时，会同步处理属性数据。

对于普通 Point Data：

```text
Point Attribute
```

会根据保留下来的 Point ID 重新复制。

对于普通 Cell Data：

```text
Cell Attribute
```

会根据保留下来的 Cell ID 重新复制。

测试已验证普通 Point Attribute 和 Cell Attribute 能够在 Ghost Cell 删除后正确保持对应关系，同时属性类型不会被统一转换为 `double`。

例如：

```text
FloatArray
IntArray
Int64 类型数据
```

均有相应测试覆盖。

输出结果中 Ghost 属性：

```text
vtkGhostType
```

不会继续保留。

## 7. Point Ghost

Filter 会同时检查 Point 和 Cell 上的 `vtkGhostType`。

当前测试包含 Point Ghost Only 情况，用于确认只存在 Point 级 Ghost 信息时仍能够正确识别和处理 Ghost 属性。

Cell 的实际删除主要依据 Cell 级 `vtkGhostType` 完成。

## 8. 无 Ghost 信息

如果输入中不存在 Point 或 Cell 级：

```text
vtkGhostType
```

则 Filter 不应修改原有几何、拓扑和普通属性数据。

对应测试确认：

```text
Cells 数量不变
Points 数量不变
普通 Point Attribute 保持
普通 Cell Attribute 保持
```

原核心测试对无 Ghost 输入也有单独验证。:contentReference[oaicite:1]{index=1}

GUI 中遇到这种情况时，不创建新的重复结果节点，并提示当前模型未发现 Ghost 信息。

## 9. 注意事项

当前 Remove Ghost Information 主要针对 `UnstructuredMesh` 数据进行处理。

Cell Ghost 删除依据 `vtkGhostType` 中的 Ghost bit 判断，而不是简单判断数值是否非零。

Legacy `.vtk` 文件中的 Ghost 信息建议使用：

```cpp
iGame::GhostVTKReader::ReadFile()
```

读取，以确保 Cell 级 `vtkGhostType` 按正确的数据类型和数值进入 AttributeSet。

`vtkGhostType` 名称判断不区分大小写，因此：

```text
vtkGhostType
vtkghosttype
```

均可被识别。

当输入中存在 Ghost Cell 时，输出 Point 数量可能同时减少，因为删除 Ghost Cell 后未被剩余 Cell 使用的 Point 会被清理。

输出网格中的普通 Point Data 和 Cell Data 会根据新的网格映射重新构建，而不是简单复制原数组。

## 10. 自动测试

核心功能测试文件：

```text
Examples/Filter/RemoveGhostInformation/TestRemoveGhostInformation.cpp
```

该测试目前覆盖：

```text
DUPLICATECELL
HIDDENCELL
Point Remapping
Point Attribute
Cell Attribute
Attribute Types
Point Ghost Only
Int64 Precision
No Ghost Attribute
```

运行：

```bat
build\Examples\Release\testRemoveGhostInformation.exe
```

正常情况下最终输出：

```text
All RemoveGhostInformation core tests passed.
```

AI生成模型测试文件：

```text
Examples/Filter/RemoveGhostInformation/TestRemoveGhostInformationModel.cpp
```

测试模型：

```text
Examples/Models/RemoveGhostInformation_AI_Test.vtk
```

程序使用写死的相对路径自动读取模型，无需手动输入文件路径。

运行：

```bat
build\Examples\Release\testRemoveGhostInformationModel.exe
```

该测试验证：

```text
vtkGhostType = [0, 1]

8 Points -> 4 Points
2 Cells  -> 1 Cell

Ghost Cell 删除
未使用 Point 删除
vtkGhostType 删除
普通 Point Attribute 保留
普通 Cell Attribute 保留
```

正常情况下最终输出：

```text
All Remove Ghost Information model tests passed.
```

## 11. 与 ParaView 的结果验证

Remove Ghost Information 的结果可使用同一测试模型在 ParaView 中进行对照。

对于：

```text
RemoveGhostInformation_AI_Test.vtk
```

输入包含2个四面体，其中：

```text
Cell 0: vtkGhostType = 0
Cell 1: vtkGhostType = 1
```

按照 Ghost Cell 删除逻辑，执行后应只保留正常四面体。

因此主要验证结果为：

```text
输入：
8 Points
2 Cells

输出：
4 Points
1 Cell
```

同时应确认：

```text
vtkGhostType 消失
正常 Point Data 保留
正常 Cell Data 保留
```

已有 ParaView 验证结果也表明，Remove Ghost Information 会依据 Cell Data 中的 `vtkGhostType` 删除对应 Ghost Cell，并清理不再使用的 Point，同时保留剩余数据对应的普通属性。:contentReference[oaicite:2]{index=2}