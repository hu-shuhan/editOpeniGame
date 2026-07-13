# 指标 10.2：特征提取模块

## 模块作用

从 CAE 仿真场数据中提取关键物理特征标量，为后续云图可视化和智能分析提供特征数据基础。支持梯度、曲率、Laplacian 算子以及涡旋特征等多种提取算法。

主要能力包括：

- 标量场梯度提取（`GradientFilter`）
- 曲面曲率提取（`CurvatureFilter`）
- Laplacian 算子特征提取（`LaplacianFilter`）
- 涡旋特征提取（`VortexFilter`）
- 基于机器学习的涡旋检测（`VortexDetectionFilter`）

## 本目录核心实现

| 文件 | 类 | 说明 |
|------|-----|------|
| `iGameGradientFilter.h/.cpp` | `GradientFilter` | 标量场梯度计算 |
| `iGameCurvatureFilter.h/.cpp` | `CurvatureFilter` | 曲面曲率计算 |
| `iGameLaplacianFilter.h/.cpp` | `LaplacianFilter` | Laplacian 算子计算 |
| `iGameVortexFilter.h/.cpp` | `VortexFilter` | 涡旋特征提取 |
| `iGameVortexDetectionFilter.h/.cpp` | `VortexDetectionFilter` | 涡旋检测 |

## 调用方式

### 编程接口：特征提取通用模式

```cpp
// 1. 读入数据
auto dataObj = iGame::FileIO::ReadFile(fileName);
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);

// 2. 选择标量并执行特征提取
drawObj->ViewCloudPicture(scene, 0);
auto filter = iGame::GradientFilter::New();
filter->SetInput(drawObj);
filter->Execute();

// 3. 提取结果作为新标量属性，切换云图显示
int newIndex = drawObj->GetAttributeSet()->GetNumberOfAttributes() - 1;
drawObj->ViewCloudPicture(scene, newIndex);
```

各 Filter 均继承自框架 `Filter` 基类，调用模式统一为 `New()` → `SetInput()` → `Execute()`，提取结果自动追加到输入对象的 `AttributeSet` 中。

### GUI 调用

特征提取结果通过标量云图面板（`igQtScalarViewWidget` / `dockWidget_ScalarField`）切换显示。

## 相关示例

| 示例 Target | 说明 |
|-------------|------|
| `testGradientExtraction` | 梯度特征提取 |
| `testCurvatureExtraction` | 曲率特征提取 |
| `testLaplacianExtraction` | Laplacian 特征提取 |
| `testVortexExtraction` | 涡旋特征提取 |
| `testVortexDetection` | 涡旋检测 |