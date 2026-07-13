# Metric 10.2: Feature Extraction Module

## Purpose

Extract key physical feature scalars from CAE field data for cloud maps and downstream analysis.

## Source Paths

`iGameCore/Filters/FeatureExtraction/` — `GradientFilter`, `CurvatureFilter`, `LaplacianFilter`, `VortexFilter`, `VortexDetectionFilter` (optional LibTorch).

## How It Is Called

```cpp
auto filter = iGame::GradientFilter::New();
filter->SetInput(drawObj);
filter->Execute();
drawObj->ViewCloudPicture(scene, newAttributeIndex);
```

### GUI

`dockWidget_ScalarField` / `igQtScalarViewWidget`.

## Related Examples

| Target | Condition |
|--------|-----------|
| `testGradientExtraction` / `testCurvatureExtraction` / `testLaplacianExtraction` / `testVortexExtraction` | default |
| `testVortexDetection` | `ENABLE_LIBTORCH_MODULE=ON` |
