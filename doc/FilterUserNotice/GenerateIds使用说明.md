# GenerateIds 滤波器使用说明

> - 头文件:`iGameCore/Filters/GenerateIds/iGameGenerateIdsFilter.h`
> - 实现文件:`iGameCore/Filters/GenerateIds/iGameGenerateIdsFilter.cpp`
> - 自动测试示例:`Examples/Filter/TestThresholdAndGenerateIds.cpp`
> - 配套测试模型:`Examples/Models/GenerateIdsTestData.vtk`、`Examples/Models/GenerateIdsMixedCells.vtk`

---

## 一、功能

为数据对象生成一份**自增编号(Id)标量数组**,挂载到点数据或单元数据上,用于稳定标识、追溯与后续筛选。

- 一次只处理一种关联类型:构造时通过 `New(IG_POINT)` 或 `New(IG_CELL)` 指定
- 数组名与起始编号可配置(默认名 `Ids`,默认起始 `0`)
- Id 以 **`LongLongArray`(64 位整型)** 存储,并通过类型化访问器直写,超过 2^53 的大整数**不会因 `double` 中转丢失精度**
- 同名属性按 **"数组名 + attachmentType" 双键匹配**处理:点与单元可安全持有同名数组(例如 `IG_POINT` 的 `Ids` 与 `IG_CELL` 的 `Ids` 共存),重复执行不会重复添加,也不会覆盖到另一种关联的数组
- 输入与输出为同一数据对象(在原对象上增加属性),不改变网格拓扑;若对象支持绘制,会自动触发重建绘制数据以刷新显示

**适用场景**:为点/单元编号以便追溯原始数据、在阈值/裁剪等拓扑会改变的滤波器之前打 Id、为属性面板或图表提供稳定的 X 轴编号等。

---

## 二、调用方式

### 2.1 主要接口

| 接口 | 说明 |
| --- | --- |
| `static Pointer New(IGenum dataType)` | 创建滤波器,`dataType` 只能为 `IG_POINT` 或 `IG_CELL` |
| `SetInput(DataObject::Pointer)` | 设置输入数据(基类接口) |
| `SetArrayName(const std::string& name)` | 设置生成的数组名,默认 `"Ids"` |
| `SetStartId(long long start)` | 设置起始编号,默认 `0`,实际 Id = `start + i` |
| `bool Execute()` | 执行,失败返回 `false`(基类接口) |
| `DataObject::Pointer GetOutput()` | 获取输出(与输入为同一对象) |

### 2.2 典型调用步骤

1. `iGameGenerateIdsFilter::New(IG_POINT)` 或 `New(IG_CELL)` 创建滤波器
2. `SetInput(dataObject)` 设置输入
3. `SetArrayName("PointIds")` 指定数组名(可选)
4. `SetStartId(0)` 指定起始编号(可选)
5. `Execute()` 执行
6. `GetOutput()` 取回结果(与输入同一对象,可直接继续串接其它滤波器)

---

## 三、使用示例

### 3.1 生成点 Id 与单元 Id

```cpp
#include "GenerateIds/iGameGenerateIdsFilter.h"
#include "iGameFileIO.h"

auto input = iGame::FileIO::ReadFile("./Models/GenerateIdsTestData.vtk");

// 点 Id
auto pointIdFilter = iGame::iGameGenerateIdsFilter::New(IG_POINT);
pointIdFilter->SetInput(input);
pointIdFilter->SetArrayName("PointIds");
pointIdFilter->SetStartId(0);
if (!pointIdFilter->Execute()) { /* 处理失败 */ }

// 单元 Id(可继续在同一对象上追加)
auto cellIdFilter = iGame::iGameGenerateIdsFilter::New(IG_CELL);
cellIdFilter->SetInput(pointIdFilter->GetOutput());
cellIdFilter->SetArrayName("CellIds");
cellIdFilter->SetStartId(0);
cellIdFilter->Execute();

auto result = cellIdFilter->GetOutput(); // 同时含 PointIds 与 CellIds
```

### 3.2 同名点/单元数组共存

```cpp
// 点、单元各生成一份名为 "Ids" 的数组,互不覆盖
auto p = iGame::iGameGenerateIdsFilter::New(IG_POINT);
p->SetInput(input);
p->SetArrayName("Ids");
p->Execute();

auto c = iGame::iGameGenerateIdsFilter::New(IG_CELL);
c->SetInput(p->GetOutput());
c->SetArrayName("Ids");
c->Execute();

// 结果:点关联 Ids 一份 + 单元关联 Ids 一份,重复执行不会继续新增
```

### 3.3 大整数 Id(超过 2^53)不丢精度

```cpp
// 2^53 + 1 无法用 double 精确表示,Id 用 LongLongArray 直写因此保持精确
auto filter = iGame::iGameGenerateIdsFilter::New(IG_POINT);
filter->SetInput(input);
filter->SetArrayName("BigIds");
filter->SetStartId(9007199254740993LL); // 2^53 + 1
filter->Execute();

auto& attr = filter->GetOutput()->GetAttributeSet()->GetAttribute("BigIds");
// attr.pointer->GetArrayType() == IG_LongLongArray
auto typed = iGame::DynamicCast<iGame::LongLongArray>(attr.pointer);
const long long first = typed->ValueAt(0); // == 9007199254740993,精确无损
```

### 3.4 与 Threshold 配合(先打 Id 再筛选)

```cpp
// 先给点/单元打 Id,再按标量阈值筛选;筛选结果中 Id 数组按类型无损保留
auto threshold = iGame::ThresholdFilter::New();
threshold->SetInput(result);
threshold->SetScalarData(scalar.pointer, iGame::ThresholdFilter::Association::Point, 0);
threshold->SetThreshold(lower, upper);
threshold->Execute();
// 输出网格中 PointIds / CellIds 数量与新网格点数 / 单元数一致
```

### 3.5 运行内置自动测试

```powershell
# 构建 Examples 后(需 EXAMPLE_COMPILE=ON),直接运行,无需任何手动输入
cmake --build build --target testThresholdAndGenerateIds --config Debug
cd build/Examples/Debug
./testThresholdAndGenerateIds.exe
```

测试会自动读取 `./Models/` 下的内置模型,验证点/单元 Id 生成、同名数组共存、大整数精度、混合单元类型网格以及 Id 穿过滤波器后的完整性,全部通过时输出 `[PASS]`。

---

## 四、注意事项

1. **关联类型只能二选一**:`New(dataType)` 的 `dataType` 必须是 `IG_POINT` 或 `IG_CELL`;其它取值时 `Execute()` 直接返回 `false`。
2. **输入不能为空**:输入为 `nullptr` 时 `Execute()` 返回 `false`;若点数或单元数为 0,则直接返回 `true` 且不生成数组。
3. **属性集缺失会自动创建**:数据对象没有 `AttributeSet` 时会自动创建一个再挂载。
4. **同名覆盖为双键匹配**:只有"数组名相同 **且** attachmentType 相同"才会被视为同一属性并覆盖;点与单元的同名数组互不影响,也不会重复添加。
5. **Id 类型为 `LongLongArray`**:写入使用类型化访问器(`ValueAt`),不经 `double`,因此 2^53 以上的大编号仍然精确;读取端若用 `GetValue()`/`GetElementValue()`(返回 `double`)会重新引入精度损失,建议按需使用类型化接口。
6. **输出即输入**:滤波器在原对象上追加属性,`GetOutput()` 与输入是同一对象;若需要保留未加 Id 的原始数据,请先做深拷贝。
7. **重复执行是幂等的**:同名同关联的数组只保留一份,重复执行会刷新数值而不是不断新增。
8. **绘制刷新**:输出对象若为 `DrawObject`,内部会调用 `ForceReConvertToDrawableData()` 触发重建,界面可直接看到新数组;非绘制对象无此副作用。
9. **执行日志**:每次执行会向标准输出打印 `[INFO] Added array '...' with N elements to Point/Cell data.` 与当前属性总数,便于排查。

---

## 五、配套测试模型

| 模型 | 规模 | 字段 | 用途 |
| --- | --- | --- | --- |
| `Examples/Models/GenerateIdsTestData.vtk` | 25 点 / 16 四边形 | `Height`(标量) | 点/单元 Id 生成、同名点单元数组共存、大整数 Id 精度校验 |
| `Examples/Models/GenerateIdsMixedCells.vtk` | 9 点 / 5 单元(3 个四边形 + 2 个三角形) | `Height`(标量) | 混合单元类型网格上的 Id 生成 |

运行时这些模型由 `iGameCopyExampleAssets` 自动拷贝到 Examples 构建目录下的 `Models/`,示例代码直接写死相对路径 `./Models/xxx.vtk`,**无需手动输入**。
