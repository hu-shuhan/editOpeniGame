# Metric 11.4: High-Precision Parallel CAE Visualization Platform

## Composition

This metric builds a **high-precision parallel visualization platform** for CAE results: on a unified data model and OpenGL / Meshlet pipeline it integrates high-order (IGA) high-fidelity display, multi-type field visualization, multi-scale interaction, critical-feature extraction, and result evaluation into an open-source deliverable stack.

| # | Sub-feature (vs. assessment text) | Status | Detail doc |
|---|-----------------------------------|--------|------------|
| 1 | High-precision parallel kernel (VTK-class: Meshlet GPU accel, thread pool, render-pressure pacing) | ✅ Implemented | **This document** |
| 2 | IGA / high-order high-fidelity visualization (spectral methods: see Known gaps) | ✅ Partial (Spline / Lagrange) | [README_7.1.en.md](README_7.1.en.md) |
| 3 | Cloud maps / adaptive vectors / tensors and related field visualization | ✅ Implemented | [README_11.3.en.md](README_11.3.en.md) |
| 4 | Large-scale multi-scale physical-field visual interaction | ✅ Implemented | This overview |
| 5 | Multi-level critical-feature intelligent extraction | ✅ Implemented | [README_10.2.en.md](README_10.2.en.md) |
| 6 | Visualization-result intelligent evaluation (expert / annotation fusion) | ✅ Partial (vortex Precision/Recall; mesh metrics pending stronger GUI) | This overview + [README_10.2.en.md](README_10.2.en.md) |

> **Doc style**: 11.4 is a **platform umbrella**. Sub-features 1 and 6 are expanded here; 2–5 are mainly cross-links to avoid duplicating 7.1 / 10.x / 11.3.  
> Differs from **11.3**: 11.3 answers “how to draw fields”; 11.4 answers “how the platform parallelizes / accelerates and integrates those modules”.  

![Architecture](../../Resources/Images/架构图.png)

---

## Platform Overview

`iGameCore` + `Qt` form the iGameVis visualization product:

| Layer | Path | Role |
|-------|------|------|
| Data & parallel | `iGameCore/Core/` | `DataObject` / `DrawObject`, `ThreadPool` |
| Filters | `iGameCore/Filters/` | Features, field viz, deformation, multi-scale analytics |
| IO | `iGameCore/IO/` | VTK / CGNS / PVD / Spline, etc. |
| Rendering | `iGameCore/Rendering/` | Scene, OpenGL, Meshlet, interactor |
| GUI | `Qt/` | Main window, docks, model tree, filter menus |

Typical GUI data path:

```text
main.cpp → igQtMainWindow → igQtFileLoader::OpenFile()
         → FileIO::ReadFile() → Scene::AddModel()
         → Docks / menus call Filters / DrawObject / Meshlet
```

---

## Sub-feature 1: High-precision parallel visualization kernel

### Description

Three parallel / acceleration pillars on the shared `DrawObject` pipeline for large CAE meshes:

1. **Meshlet GPU acceleration** — meshlets + mesh/compute shaders for culling and draw scheduling.  
2. **CPU thread pool** — `ThreadPool::parallelFor` / `Commit` for filters, codecs, time-frame loads.  
3. **Render-pressure pacing** — `Scene` skips frames under target FPS / GPU usage caps.

### Source Paths

| Path | Class / API | Notes |
|------|-------------|-------|
| `iGameCore/Rendering/Core/Meshleter/` | `Meshleter` / `SurfaceMeshMeshleter` | Build & draw meshlets |
| `iGameCore/Rendering/Shaders/GLSL/MeshShaders/` | Mesh / Task / MeshletCull | GPU shaders |
| `iGameCore/Core/DataModel/iGameDrawObject.*` | `SetAccelerationOption` / `SetRenderWithMeshlet` | Enable acceleration |
| `iGameCore/Core/Common/iGameThreadPool.h` | `parallelFor` / `Commit` | CPU parallelism |
| `iGameCore/Rendering/Core/iGameScene.*` | `SetTargetFps` / `SetGpuUsageLimit` / `ShouldRenderThisCall` | FPS / pressure control |
| `Qt/src/IQComponents/igQtModelTreeWidget.*` | Model-tree menus | Toggle acceleration |

### How It Is Called

**Meshlet** (`Examples/Rendering/MeshletRendering.cpp`):

```cpp
auto scene = iGame::Scene::New();
auto dataObj = iGame::FileIO::ReadFile("./Models/Tet_Plane.vtk");
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
drawObj->SetAccelerationOption(true);
scene->AddModel(dataObj);
scene->ResetCameraView();
```

**Render pressure / target FPS** (`Examples/Rendering/SetRenderingPressure.cpp`):

```cpp
auto scene = iGame::Scene::New();
auto dataObj = iGame::FileIO::ReadFile("./Models/Tet_Plane.vtk");
scene->AddModel(dataObj);
scene->ResetCameraView();
// scene->SetGpuUsageLimit(0.1f);  // optional GPU usage cap
scene->SetTargetFps(30);
```

**CPU parallel** (used widely inside filters / IO):

```cpp
iGame::ThreadPool::parallelFor(0, count, [&](int i) {
    // process element i
});
```

### GUI

| Entry | Notes |
|-------|-------|
| Model-tree context / acceleration menus | `SetAccelerationOption`, Meshlet toggle |
| Scene interaction | Large meshes stay interactive via FPS / GPU caps |

### Test Cases

| Target | Source | Default data | Notes |
|--------|--------|--------------|-------|
| `testMeshletRendering` | `Examples/Rendering/MeshletRendering.cpp` | `./Models/Tet_Plane.vtk` | Meshlet GPU path |
| `testSetRenderingPressure` | `Examples/Rendering/SetRenderingPressure.cpp` | `./Models/Tet_Plane.vtk` | Target FPS / pressure |

Build Examples with `EXAMPLE_COMPILE=ON` (or the repo’s current Examples CMake workflow).

---

## Sub-feature 2: IGA / high-order high-fidelity visualization

### Description

Lagrange high-order conversion and NURBS/Spline CPU/GPU readers for IGA-style high-fidelity display.

Full detail, API, GUI, and examples: **[README_7.1.en.md](README_7.1.en.md)**.

### Test Cases (entry points)

| Target | Condition |
|--------|-----------|
| `testConvertToLagrangeUnstructuredMesh` | default |
| `testSplineReaderCPU` | default |
| `testSplineReaderGPU` | `ENABLE_GPSCUDA_MODULE=ON` |

### Known gaps

- No dedicated **spectral-methods** visualization module; high-fidelity is carried mainly by **Spline / Lagrange**.  
- VTK high-order end-to-end visualization still has documented limits (see 7.1 / user notice).

---

## Sub-feature 3: Cloud / adaptive vector / tensor field visualization

### Description

Platform integrates scalar cloud maps, adaptive vector glyphs, tensor glyphs, deformation, time-series streamlines, and animation export.

Full detail, screenshots, call sites, and tests: **[README_11.3.en.md](README_11.3.en.md)** (Chinese: [README_11.3.md](README_11.3.md)).

### Test Cases (entry points)

| Target | Notes |
|--------|-------|
| `testSetScalarField` | Cloud map |
| `testVector` / `testVectorEveryNth` / … | Adaptive vectors |
| `testTensorView` | Tensors |
| `testDeformation` / `testDeformationCode` | Deformation |
| `testStreamline` / `testTimeVaryingVector` | Streamlines / time series |
| `testAnimation` / `testSaveAnimation` | Playback / export |

---

## Sub-feature 4: Large-scale multi-scale visual interaction

### Description

Brush/box selection in parallel coordinates, correlation, density, and plot-line views links to 3D highlight/filter via `Selection`.

### Test Cases (entry points)

| Target | Source |
|--------|--------|
| `testMultiscaleInteraction` | `Examples/MultiscaleInteraction/TestMultiscaleInteraction.cpp` |
| `testParallelCoordinatesData`, etc. | Per-view examples |

---

## Sub-feature 5: Multi-level critical-feature extraction

### Description

Classical features (gradient / curvature / Laplacian / vorticity) plus NN vortex detection; results land in `AttributeSet` for 11.3 cloud maps and selection analysis.

Detail and Precision/Recall ≥ 90% target: **[README_10.2.en.md](README_10.2.en.md)**.

### Test Cases (entry points)

| Target | Condition |
|--------|-----------|
| `testGradientExtraction` / `testCurvatureExtraction` / `testLaplacianExtraction` / `testVortexExtraction` | default |
| `testVortexDetection` | `ENABLE_LIBTORCH_MODULE=ON` |

---

## Sub-feature 6: Visualization-result intelligent evaluation

### Description

The strongest “annotation / expert knowledge” path today is **vortex prediction vs. labeled attributes**, computing Accuracy / Precision / Recall (`VortexDetection::EvaluatePredictMetrics` in 10.2).

`Filters/MeshMetrics/` can support geometric quality scoring but GUI/Examples wiring is still weak; prefer vortex metrics for live demos.

### Source Paths

| Path | API | Notes |
|------|-----|-------|
| `iGameCore/Filters/FeatureExtraction/` (vortex detection) | `EvaluatePredictMetrics` / `GetPrecision` / `GetRecall` | Annotation comparison |
| `iGameCore/Filters/MeshMetrics/` | `SurfaceMeshMetricsFilter` / `VolumeMeshMetricsFilter` | Mesh quality (pending stronger integration) |

### How It Is Called (from the 10.2 path)

```cpp
// After vortex prediction, with a PredictedLabel annotation attribute present:
filter->EvaluatePredictMetrics(/* ... */);
double precision = filter->GetPrecision();
double recall    = filter->GetRecall();
```

### Test Cases

| Target | Notes |
|--------|-------|
| `testVortexDetection` | Prediction + metrics (LibTorch) |

### Known gaps

- No standalone product module for rule-based / expert-scoring **visualization quality** evaluation.  
- Main-window Precision/Recall overlay may still be commented; values remain available via API / console.

---

## Related Examples (summary)

| Target | Sub-feature | Condition |
|--------|-------------|-----------|
| `testMeshletRendering` | 1 Parallel accel | default |
| `testSetRenderingPressure` | 1 Render pressure | default |
| `testSplineReaderCPU` / `GPU` / `testConvertToLagrangeUnstructuredMesh` | 2 High-fidelity | GPU spline needs GPS CUDA |
| `testSetScalarField` / `testVector*` / `testTensorView` / … | 3 Field viz | see 11.3 |
| `testMultiscaleInteraction` | 4 Multi-scale | default |
| `testGradientExtraction` / … / `testVortexDetection` | 5–6 Features & eval | vortex needs LibTorch |

---

## Related Metrics

| Metric | Doc | Relation to 11.4 |
|--------|-----|------------------|
| 7.1 | [README_7.1.en.md](README_7.1.en.md) | High-order / IGA fidelity |
| 10.1 | [README_10.1.en.md](README_10.1.en.md) | Analysis data generation |
| 10.2 | [README_10.2.en.md](README_10.2.en.md) | Critical features & evaluation |
| 11.3 | [README_11.3.en.md](README_11.3.en.md) | Field visualization output |

Index: [README.md](README.md)
