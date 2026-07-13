# 指标 10.2：特征提取模块

## 模块作用

从 CAE 仿真场数据中提取关键物理特征标量，为云图显示与后续分析提供特征场。

## 源码路径

| 路径 | 类 | 说明 |
|------|-----|------|
| `iGameCore/Filters/FeatureExtraction/iGameGradientFilter.*` | `GradientFilter` | 标量梯度 |
| `iGameCore/Filters/FeatureExtraction/iGameCurvatureFilter.*` | `CurvatureFilter` | 曲面曲率 |
| `iGameCore/Filters/FeatureExtraction/iGameLaplacianFilter.*` | `LaplacianFilter` | Laplacian 算子 |
| `iGameCore/Filters/FeatureExtraction/iGameVortexFilter.*` | `VortexFilter` | 涡旋特征 |
| `iGameCore/Filters/FeatureExtraction/iGameVortexDetectionFilter.*` | `VortexDetectionFilter` | ML 涡旋检测（可选） |

## 调用方式

```cpp
auto dataObj = iGame::FileIO::ReadFile(fileName);
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
auto filter = iGame::GradientFilter::New();
filter->SetInput(drawObj);
filter->Execute();
int newIndex = drawObj->GetAttributeSet()->GetNumberOfAttributes() - 1;
drawObj->ViewCloudPicture(scene, newIndex);
```

统一模式：`Filter::New()` → `SetInput()` → `Execute()`，结果追加到 `AttributeSet`。

### GUI

提取结果通过 `dockWidget_ScalarField` / `igQtScalarViewWidget` 切换云图显示。

## 相关示例

| 示例 Target | 说明 | 条件 |
|-------------|------|------|
| `testGradientExtraction` | 梯度 | 默认 |
| `testCurvatureExtraction` | 曲率 | 默认 |
| `testLaplacianExtraction` | Laplacian | 默认 |
| `testVortexExtraction` | 涡旋特征 | 默认 |
| `testVortexDetection` | ML 涡旋检测 | `ENABLE_LIBTORCH_MODULE=ON` |
