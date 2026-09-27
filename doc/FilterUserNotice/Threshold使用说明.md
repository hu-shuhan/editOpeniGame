# Threshold 滤波器使用说明

> - 头文件:`iGameCore/Filters/Threshold/iGameThresholdFilter.h`
> - 实现文件:`iGameCore/Filters/Threshold/iGameThresholdFilter.cpp`
> - 自动测试示例:`Examples/Filter/TestThresholdAndGenerateIds.cpp`
> - 配套测试模型:`Examples/Models/ThresholdScalarField.vtk`、`Examples/Models/ThresholdVolumeData.vtk`

---

## 一、功能

按标量值的区间筛选单元:保留标量值落在 `[lower, upper]` 区间内的单元,输出一张**全新的非结构化网格**。

- 支持**点关联(Point)**与**单元关联(Cell)**两类标量数据
- 支持四种边界语义:闭区间、开区间、下界包含、上界包含
- 点关联时支持两种判定方式:
  - `AllScalars`:单元的全部顶点都落在区间内才保留(结果更严格)
  - `AnyScalar`:单元只要有任意顶点落在区间内就保留(结果更宽松)
- 输出网格只包含保留的单元及其顶点,顶点自动重编号压缩,不残留孤立点
- 输入网格的所有属性(点属性、单元属性)会按**原始数组类型无损复制**到输出网格,64 位整型 Id 不会因 `double` 中转而丢失精度

**适用场景**:提取感兴趣数值范围的数据区域,例如压力/温度/曲率的区间提取、异常值区域定位、Id 数组的区间筛选等。

---

## 二、调用方式

### 2.1 主要接口

| 接口 | 说明 |
| --- | --- |
| `static Pointer New()` | 创建滤波器实例 |
| `SetInput(DataObject::Pointer)` | 设置输入数据(基类接口) |
| `SetScalarData(ArrayObject::Pointer array, Association association = Point, int dimension = 0)` | 指定用于判定的标量数组、关联类型与分量下标 |
| `SetThreshold(double lower, double upper)` | 一次性设置上下阈值(默认 `0.0 ~ 1.0`) |
| `SetLowerThreshold(double)` / `SetUpperThreshold(double)` | 分别设置下/上阈值 |
| `SetBoundaryMode(BoundaryMode)` | 设置边界语义,默认 `Closed` |
| `SetPointEvaluation(PointEvaluation)` | 设置点关联判定方式,默认 `AllScalars` |
| `bool Execute()` | 执行筛选,失败返回 `false` |
| `DataObject::Pointer GetOutput()` | 获取输出(基类接口) |
| `UnstructuredMesh::Pointer GetThresholdMesh()` | 以非结构化网格形式获取输出 |
| `GetLowerThreshold()` / `GetUpperThreshold()` / `GetBoundaryMode()` / `GetPointEvaluation()` / `GetAssociation()` / `GetScalarData()` | 读取当前配置 |

### 2.2 枚举定义

```cpp
enum class Association { Point, Cell };                                  // 标量关联类型
enum class PointEvaluation { AllScalars, AnyScalar };                    // 点关联判定方式
enum class BoundaryMode { Closed, Open, LowerInclusive, UpperInclusive }; // 边界语义
```

`BoundaryMode` 语义对照:

| 模式 | 区间 | 含义 |
| --- | --- | --- |
| `Closed` | `[lower, upper]` | 上下界都包含(默认) |
| `Open` | `(lower, upper)` | 上下界都不包含 |
| `LowerInclusive` | `[lower, upper)` | 只包含下界 |
| `UpperInclusive` | `(lower, upper]` | 只包含上界 |

### 2.3 典型调用步骤

1. `ThresholdFilter::New()` 创建滤波器
2. `SetInput(dataObject)` 设置输入网格
3. `SetScalarData(array, Association::Point, 0)` 指定判定用的标量数组
4. `SetThreshold(lower, upper)` 设置阈值区间
5. (可选)`SetBoundaryMode(...)`、`SetPointEvaluation(...)`
6. `Execute()` 执行,返回 `true` 表示成功
7. `GetOutput()` / `GetThresholdMesh()` 获取筛选结果

---

## 三、使用示例

### 3.1 点关联筛选(AllScalars)

```cpp
#include "Threshold/iGameThresholdFilter.h"
#include "iGameFileIO.h"

// 1. 读取数据
auto input = iGame::FileIO::ReadFile("./Models/ThresholdScalarField.vtk");

// 2. 取用于判定的标量数组(此处为点关联的 Pressure)
auto attrs = input->GetAttributeSet();
auto& pressure = attrs->GetAttribute("Pressure");
const double lower = -1.0;
const double upper = 0.5;

// 3. 配置并执行
auto filter = iGame::ThresholdFilter::New();
filter->SetInput(input);
filter->SetScalarData(pressure.pointer, iGame::ThresholdFilter::Association::Point, 0);
filter->SetThreshold(lower, upper);
filter->SetBoundaryMode(iGame::ThresholdFilter::BoundaryMode::Closed);
filter->SetPointEvaluation(iGame::ThresholdFilter::PointEvaluation::AllScalars);
if (!filter->Execute()) { /* 处理失败 */ }

// 4. 输出为全新网格
auto mesh = filter->GetThresholdMesh();
```

### 3.2 单元关联筛选(按单元标量)

```cpp
auto& cellQuality = attrs->GetAttribute("CellQuality"); // 单元关联标量

auto filter = iGame::ThresholdFilter::New();
filter->SetInput(input);
filter->SetScalarData(cellQuality.pointer,
                      iGame::ThresholdFilter::Association::Cell, 0);
filter->SetThreshold(0.232, 0.808);
filter->Execute();
```

> 单元关联时 `SetPointEvaluation` 不起作用(每个单元只看自身一个标量值)。

### 3.3 与 GenerateIds 配合(先打 Id 再筛选)

```cpp
// 先给点/单元打上稳定 Id,再按标量筛选,筛选结果里 Id 数组会无损保留
auto idFilter = iGame::iGameGenerateIdsFilter::New(IG_POINT);
idFilter->SetInput(input);
idFilter->SetArrayName("PointIds");
idFilter->SetStartId(0);
idFilter->Execute();

auto threshold = iGame::ThresholdFilter::New();
threshold->SetInput(idFilter->GetOutput());
threshold->SetScalarData(pressure.pointer, iGame::ThresholdFilter::Association::Point, 0);
threshold->SetThreshold(lower, upper);
threshold->Execute();
// 输出网格中仍能找到 "PointIds"、"CellIds",且数量与新网格点数/单元数一致
```

### 3.4 运行内置自动测试

```powershell
# 构建 Examples 后(需 EXAMPLE_COMPILE=ON),直接运行,无需任何手动输入
cmake --build build --target testThresholdAndGenerateIds --config Debug
cd build/Examples/Debug
./testThresholdAndGenerateIds.exe
```

测试会自动读取 `./Models/` 下的内置模型,依次验证点关联/单元关联筛选、Id 数组无损传递与边界模式语义,全部通过时输出 `[PASS]`。

---

## 四、注意事项

1. **标量长度必须匹配关联对象数**:点关联要求标量元素数 ≥ 输入点数,单元关联要求 ≥ 输入单元数,否则 `Execute()` 返回 `false`。
2. **分量下标要合法**:`dimension` 必须小于标量数组的维度,否则 `Execute()` 返回 `false`。
3. **阈值区间要有效**:`lower > upper` 时 `Execute()` 返回 `false`,不会产生空结果(空结果只可能由区间过窄导致)。
4. **非有限值一律排除**:`NaN` / `Inf` 视为不满足条件,不会被保留。
5. **点关联 + AllScalars 更严格**:要求单元所有顶点都落在区间内,区间取 20%~80% 分位时通常能保留中间区域单元;区间过窄可能得到空网格,此时 `[FAIL] 筛选结果包含单元` 之类断言会失败。
6. **输出是全新网格**:滤波器不会修改输入数据,`GetOutput()` 返回重编号、压缩后的新网格;若需要追溯原网格单元,应先用 GenerateIds 打 Id。
7. **属性复制保持类型**:输出属性按源数组实际存储类型逐元素复制,`LongLongArray` / `UnsignedLongLongArray` 等 64 位整型不会退化为 `double`,大整数 Id 安全。
8. **依赖输入可转换**:输入若无法转换为非结构化网格,`Execute()` 返回 `false`;输入类型为 `IG_NONE` 时直接返回 `true` 且不产生输出。
9. **执行进度**:`Execute()` 成功后会通过 `UpdateProgress(1.0)` 更新进度,可在界面进度条中直接反映。

---

## 五、配套测试模型

| 模型 | 规模 | 字段 | 用途 |
| --- | --- | --- | --- |
| `Examples/Models/ThresholdScalarField.vtk` | 441 点 / 400 四边形 | `Pressure`(标量,范围约 -1.5~1)、`Temperature`(标量)、`Velocity`(向量) | 点关联 + `AllScalars` 筛选、边界模式对比 |
| `Examples/Models/ThresholdVolumeData.vtk` | 216 点 / 125 六面体 | `Density`(点标量)、`CellQuality`(单元标量,范围 0.04~1) | 六面体体网格筛选、**单元关联**筛选 |
| `Examples/Models/ThresholdTestData.vtk` | 64 点 / 27 六面体 | `Position`(向量)、`Curvature`(点标量) | 早期最小用例,保留兼容 |

运行时这些模型由 `iGameCopyExampleAssets` 自动拷贝到 Examples 构建目录下的 `Models/`,示例代码直接写死相对路径 `./Models/xxx.vtk`,**无需手动输入**。
