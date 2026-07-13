# Metric 10.2: Feature Extraction Module

## Purpose

Extracts key physical feature scalars from CAE simulation field data, providing a foundation for subsequent cloud map visualization and intelligent analysis. Supports gradient, curvature, Laplacian operator, and vortex feature extraction algorithms.

Key capabilities:

- Scalar field gradient extraction (`GradientFilter`)
- Surface curvature extraction (`CurvatureFilter`)
- Laplacian operator feature extraction (`LaplacianFilter`)
- Vortex feature extraction (`VortexFilter`)
- Machine-learning-based vortex detection (`VortexDetectionFilter`)

## Core Implementation in This Directory

| File | Class | Description |
|------|-------|-------------|
| `iGameGradientFilter.h/.cpp` | `GradientFilter` | Scalar field gradient computation |
| `iGameCurvatureFilter.h/.cpp` | `CurvatureFilter` | Surface curvature computation |
| `iGameLaplacianFilter.h/.cpp` | `LaplacianFilter` | Laplacian operator computation |
| `iGameVortexFilter.h/.cpp` | `VortexFilter` | Vortex feature extraction |
| `iGameVortexDetectionFilter.h/.cpp` | `VortexDetectionFilter` | Vortex detection |

## How It Is Called

### API: Common Feature Extraction Pattern

```cpp
// 1. Load data
auto dataObj = iGame::FileIO::ReadFile(fileName);
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);

// 2. Select scalar and execute extraction
drawObj->ViewCloudPicture(scene, 0);
auto filter = iGame::GradientFilter::New();
filter->SetInput(drawObj);
filter->Execute();

// 3. Display extracted feature as new scalar attribute
int newIndex = drawObj->GetAttributeSet()->GetNumberOfAttributes() - 1;
drawObj->ViewCloudPicture(scene, newIndex);
```

All filters inherit from the framework `Filter` base class with the unified pattern `New()` → `SetInput()` → `Execute()`. Results are automatically appended to the input object's `AttributeSet`.

### GUI Invocation

Extracted features are displayed via the scalar cloud map panel (`igQtScalarViewWidget` / `dockWidget_ScalarField`).

## Related Examples

| Example Target | Description |
|----------------|-------------|
| `testGradientExtraction` | Gradient feature extraction |
| `testCurvatureExtraction` | Curvature feature extraction |
| `testLaplacianExtraction` | Laplacian feature extraction |
| `testVortexExtraction` | Vortex feature extraction |
| `testVortexDetection` | Vortex detection |