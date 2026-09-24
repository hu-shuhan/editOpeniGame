# GlobalIds 使用说明

## 功能

`iGame::GenerateGlobalIdsFilter` 为输入数据的点和单元生成连续、唯一的全局 ID 属性。默认生成以下两个一维标量数组：

- 全局点属性 `GlobalPointIds`，挂载类型为 `IG_POINT`。
- 全局单元属性 `GlobalCellIds`，挂载类型为 `IG_CELL`。

生成的数组类型为 `DoubleArray`。Filter 会直接修改输入对象的 `AttributeSet`，执行成功后 `GetOutput()` 与输入对象指向同一个 `DataObject`。

该 Filter 支持普通 `PointSet`，以及 `SurfaceMesh`、`VolumeMesh`、`UnstructuredMesh`、`StructuredMesh` 和 `LagrangeUnstructuredMesh`。对于复合数据，会递归处理所有叶子对象；多个叶子共享同一个 `Points` 对象时，共享同一段 Point IDs，不会重复计数。

## 主要接口

头文件：

```cpp
#include <GlobalIds/iGameGenerateGlobalIdsFilter.h>
```

| 接口 | 作用 | 默认值 |
| --- | --- | --- |
| `SetInput(input)` | 设置待处理的 `DataObject` | 无 |
| `SetGeneratePointIds(bool)` | 是否生成点全局 ID | `true` |
| `SetGenerateCellIds(bool)` | 是否生成单元全局 ID | `true` |
| `SetPointOffset(offset)` | 设置点 ID 的起始偏移 | `0` |
| `SetCellOffset(offset)` | 设置单元 ID 的起始偏移 | `0` |
| `SetOffsets(pointOffset, cellOffset)` | 同时设置点和单元起始偏移 | `0, 0` |
| `SetPointArrayName(name)` | 设置点 ID 数组名称 | `GlobalPointIds` |
| `SetCellArrayName(name)` | 设置单元 ID 数组名称 | `GlobalCellIds` |
| `SetExistingIdPolicy(policy)` | 设置已有同名 ID 属性的处理方式 | `Error` |
| `CountEntities(input, points, cells)` | 只统计输入中的点数和单元数，不生成属性 | — |
| `GetMessage()` | 获取最近一次执行失败的原因 | 空字符串 |

已有同名属性的处理策略：

- `ExistingIdPolicy::Error`：发现同名属性时执行失败，输入保持不变。
- `ExistingIdPolicy::KeepExisting`：仅当已有属性是连续且与当前 offset 完全一致的一维 `DoubleArray` 时保留，否则失败。
- `ExistingIdPolicy::Replace`：用新生成的连续 ID 数组替换已有属性。

## 调用方式

基本调用顺序如下：

1. 创建 Filter。
2. 设置输入对象。
3. 根据需要设置生成范围、数组名称和已有属性策略。
4. 调用 `Execute()`。
5. 检查返回值，并通过 `GetOutput()` 或输入对象的 `AttributeSet` 读取结果。

## 使用示例

下面的示例使用仓库内固定测试模型 `Models/GlobalIdsTestModel.vtk`，无需命令行输入模型路径：

```cpp
#include <GlobalIds/iGameGenerateGlobalIdsFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>

#include <iostream>

int main() {
    using namespace iGame;

    auto input = FileIO::ReadFile("Models/GlobalIdsTestModel.vtk");
    if (!input) {
        std::cerr << "读取测试模型失败。\n";
        return 1;
    }

    auto filter = GenerateGlobalIdsFilter::New();
    filter->SetInput(input);
    filter->SetOffsets(10000, 2000);
    filter->SetExistingIdPolicy(
            GenerateGlobalIdsFilter::ExistingIdPolicy::Error);

    if (!filter->Execute()) {
        std::cerr << filter->GetMessage() << '\n';
        return 1;
    }

    auto output = filter->GetOutput();
    auto* attributes = output->GetAttributeSet();
    const int pointIds = attributes->GetAttributeIndex("GlobalPointIds");
    const int cellIds = attributes->GetAttributeIndex("GlobalCellIds");

    std::cout << "Point ID attribute index: " << pointIds << '\n';
    std::cout << "Cell ID attribute index: " << cellIds << '\n';
    return 0;
}
```

完整测试示例位于：

```text
Examples/Filter/GlobalIds/TestGenerateGlobalIds.cpp
```

该示例会自动读取 `Examples/Models/GlobalIdsTestModel.vtk`。模型包含 12 个点和 4 个混合类型单元，用于验证 ID 生成、偏移、已有属性策略、复合数据和精度边界。

## 注意事项

1. Filter 为原地操作。执行成功会修改输入叶子对象的 `AttributeSet`，需要保留原始数据时应在调用前复制输入。
2. Offset 必须由调用者提供。Filter 不负责读取 MPI 进程号，也不会自动计算其他进程的数据前缀。
3. `Execute()` 不会自动推进 `PointOffset` 或 `CellOffset`。连续处理下一块数据时，调用者应使用“当前 offset + 当前实体数量”计算下一组 offset。
4. ID 使用 `DoubleArray` 保存。为了保证每个整数都能精确表示，生成范围的最后一个 ID 不能超过 `2^53`。
5. 点或单元数组名称不能为空；否则 `Execute()` 返回 `false`，具体原因可从 `GetMessage()` 获取。
6. 如果点 ID 和单元 ID 都关闭，`Execute()` 返回成功并原样输出输入，但不会新增属性。
7. 对复合数据执行前会先完成整体校验。任一叶子对象不受支持或已有属性不符合策略时，不会只修改一部分叶子。
8. 共享同一个 `Points` 对象的多个叶子共用 Point ID 范围；Cell ID 仍按各叶子的单元数量顺序分配。
