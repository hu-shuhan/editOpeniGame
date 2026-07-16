# Metric 10.2: Key Feature Extraction from Simulation Data

## Composition

For CAE physical-field data, this metric provides key feature-field extraction, neural-network vortex detection with comparison against manual annotations, and support for key-region interaction plus temporal evolution of key events. The technical target is: **key feature extraction Precision / Recall ≥ 90%**.

| # | Sub-feature | Status |
|---|-------------|--------|
| 1 | Classical physical features: gradient / curvature / Laplacian / vorticity | ✅ Implemented |
| 2 | NN-based vortex extraction vs. manual labels; Accuracy / Precision / Recall (target ≥ 90%) | ✅ Implemented (metrics); GUI overlay pending restore |
| 3 | Key-region click / selection (points, cells, box) | ✅ Implemented (via Selection / 10.3; extractors run on the current attribute field) |
| 4 | Temporal evolution of key events; deformation applied only to the selected region | ⏳ Partial (time series / deformation in 11.3; region-limited deformation TBD) |

> This document covers sub-features **1** and **2** in full, and how **3** / **4** connect to existing interaction and visualization modules.
> Difference from **10.1**: 10.1 focuses on **analysis data generation**; 10.2 focuses on **feature-field extraction and vortex detection evaluation**.
> Difference from **10.3**: 10.3 focuses on **brush ↔ 3D linking**; 10.2 feature scalars can feed cloud maps and selection analysis.
> Difference from **11.3**: 11.3 provides generic **time switching, structural deformation, and animation export**; 10.2 key-event temporal views rely on those capabilities.

---

## Sub-feature 1: Classical physical feature extraction (other key features)

### Description

From the currently selected physical attribute (scalar / vector), compute classical differential-geometry and fluid-mechanics feature scalars on the mesh, write them into `AttributeSet`, and display them as cloud maps for downstream vortex detection and region analysis.

| Feature | Output attribute | Notes |
|---------|------------------|-------|
| Gradient | `gradient` | Spatial gradient of scalar / vector fields |
| Curvature | `curvatures` | Surface curvature (cotangent-style discrete) |
| Laplacian | `laplacians` | Discrete Laplacian |
| Vorticity | `vorticities` | Classical \(\omega = \nabla \times v\) (not neural) |

### Source Paths

| Path | Class | Notes |
|------|-------|-------|
| `iGameCore/Filters/FeatureExtraction/iGameGradientFilter.*` | `GradientFilter` | Gradient |
| `iGameCore/Filters/FeatureExtraction/iGameCurvatureFilter.*` | `CurvatureFilter` | Curvature |
| `iGameCore/Filters/FeatureExtraction/iGameLaplacianFilter.*` | `LaplacianFilter` | Laplacian |
| `iGameCore/Filters/FeatureExtraction/iGameVortexFilter.*` | `VortexFilter` | Classical vorticity |
| `iGameCore/Filters/iGameFilterIncludes.h` | — | Shared includes |

### How It Is Called

```cpp
auto dataObj = iGame::FileIO::ReadFile(fileName);
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);

auto filter = iGame::GradientFilter::New();  // or CurvatureFilter / LaplacianFilter / VortexFilter
filter->SetInput(drawObj);
filter->SetAttributeByIndex(attrIndex);      // or SetAttributeByName(name)
filter->Execute();

int newIndex = drawObj->GetAttributeSet()->GetNumberOfAttributes() - 1;
drawObj->ViewCloudPicture(scene, newIndex);
```

Common pattern: `Filter::New()` → `SetInput()` → (optional) `SetAttributeByIndex/Name` → `Execute()`; results are appended to `AttributeSet`.

### GUI

Menu **Filters → Feature Extraction**:

| Menu item | Filter |
|-----------|--------|
| ComputeGradient | `GradientFilter` |
| Compute Laplacian | `LaplacianFilter` |
| Compute Curvature | `CurvatureFilter` |
| Compute Vorticity | `VortexFilter` |

Results appear in the model-tree attribute list and can be shown via `dockWidget_ScalarField` / `igQtScalarViewWidget`.

### Related Examples

| Target | Notes |
|--------|-------|
| `testGradientExtraction` | Gradient |
| `testCurvatureExtraction` | Curvature |
| `testLaplacianExtraction` | Laplacian |
| `testVortexExtraction` | Classical vorticity |

Test data: `test/Feature Extraction Test/`.

---

## Sub-feature 2: NN vortex extraction vs. manual annotation

### Description

Run a **LibTorch TorchScript** 3D patch CNN on volume velocity fields to predict vortex structures, writing the point scalar `vortexPredict`. If a manual annotation attribute named **`PredictedLabel`** is present, compare prediction to labels point-wise and compute:

| Metric | Definition |
|--------|------------|
| Accuracy | \((TP+TN) / (TP+FP+TN+FN)\) |
| Precision | \(TP / (TP+FP)\) |
| Recall | \(TP / (TP+FN)\) |

**Technical target**: on annotated simulation data, key feature extraction **Precision / Recall ≥ 90%** (read via `GetPrecision()` / `GetRecall()`; pass/fail depends on data and model; the code provides full metric computation).

Thresholds: ground truth `> 0` is positive; prediction `> 0.5` is positive.

### Source Paths

| Path | Class / API | Notes |
|------|-------------|-------|
| `iGameCore/Filters/FeatureExtraction/iGameVortexDetectionFilter.*` | `VortexDetection` | NN vortex detection core |
| same | `EvaluatePredictMetrics` | TP/FP/TN/FN → Accuracy / Precision / Recall |
| same | `GetAccuracy` / `GetPrecision` / `GetRecall` | Metric getters |
| `Qt/src/IQCore/igQtMainWindow.cpp` | menu PredictVortex | GUI trigger; metrics overlay reserved (currently commented) |

### Algorithm Outline

1. **Input**: volume mesh + velocity vector attribute; non-uniform grids may be resampled / blocked (`process_blocks`, `split`, …).
2. **Inference**: load TorchScript model (default `./Resources/AI/model_1x64x64x64_1108_cuda.pt`), slide **64³** patches, fuse with Hann weights.
3. **Post-process**: Q-criterion helper `ComputePointQ`, KNN label smoothing `knn_smooth_labels`.
4. **Output**: point scalar `vortexPredict` in `AttributeSet`.
5. **Evaluation**: if attribute `PredictedLabel` exists, call `EvaluatePredictMetrics` and store `m_Accuracy` / `m_Precision` / `m_Recall` (also printed to console).

### Build / Dependencies

```text
-DENABLE_LIBTORCH_MODULE=ON
```

Defines `LibTorch_ENABLE`. Requires local LibTorch (+ CUDA as configured) and the `.pt` model under `Resources/AI/`.

### How It Is Called

```cpp
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
// Optional GT: point scalar named "PredictedLabel"

auto filter = iGame::VortexDetection::New();
filter->SetInput(drawObj);
filter->SetAttributeByIndex(velocityAttrIndex);
if (filter->Execute()) {
    double accuracy  = filter->GetAccuracy();   // -1 if no GT
    double precision = filter->GetPrecision();
    double recall    = filter->GetRecall();
    // Acceptance example: precision >= 0.90 && recall >= 0.90

    int newIndex = drawObj->GetAttributeSet()->GetNumberOfAttributes() - 1;
    drawObj->ViewCloudPicture(scene, newIndex);  // vortexPredict
}
```

### GUI

Menu **Filters → Feature Extraction → PredictVortex**: runs `VortexDetection` on the current model/attribute and attaches the result to the model tree.

> `vortexMetricsLabel` and Precision/Recall overlay logic are reserved in the main window but currently commented out; metrics remain available via API / console.

### Related Examples

| Target | Notes | Condition |
|--------|-------|-----------|
| `testVortexDetection` | NN vortex detection + cloud map | `ENABLE_LIBTORCH_MODULE=ON` |

Reference data: `./Models/pipedcylinder2d_gt.vtk` (annotated scenario).

---

## Sub-feature 3: Key-region click / selection

### Description

Users can **click points / select cells / box-select regions** on the 3D model to obtain key-region IDs or a bounding box, used to:

- limit downstream analysis scope (aligned with 10.1 local charts and 10.3 brushing);
- focus cloud-map inspection near key structures;
- feed region input for temporal evolution / deformation (sub-feature 4).

Feature-extraction filters themselves operate on the **full current attribute field**; region semantics come from the **Selection interaction layer**, then link to feature results for display.

### Source Paths

| Path | Notes |
|------|-------|
| `iGameCore/Core/Common/iGameSelection.*` | Selection data model |
| `iGameCore/Rendering/Core/Interactor/iGameSelectionStyle.*` | Point / cell selection |
| `iGameCore/Rendering/Core/Interactor/iGameBoxStyle.*` | Box selection |
| `doc/modules/README_10.3.md` | Brush ↔ 3D linking |

### Usage Notes

1. Enable a selection style; click or box-select to obtain point / cell IDs.
2. Run feature extraction (sub-features 1 / 2) on the full field, then inspect attributes such as `vortexPredict`.
3. For local analysis, pass the selection bounding box into 10.1 charts or 10.3 brushing.

---

## Sub-feature 4: Temporal evolution of key events & region-scoped deformation

### Description

**Intended capabilities**:

1. **Temporal evolution of key events**: on multi-timestep data (e.g. PVD), play vortex-predict / vorticity feature fields along the time axis to observe structure evolution.
2. **Deformation on the selected region only**: apply displacement deformation only to user-selected points / cells to highlight local structural response.

### Current Status & Integration

| Capability | Status | Entry |
|------------|--------|-------|
| Time-step switching / animation | ✅ Generic capability | `DataObject::UpdateAnimation(keyframeIdx)`; PVD; animation dock / FFMPEG (**11.3**) |
| Feature refresh over time | ✅ Switch cloud maps per frame; re-run or precompute vortex predict | `ViewCloudPicture` + `UpdateAnimation` |
| Whole-mesh structural deformation | ✅ Implemented | `StressDeformationFilter` + `igQtDeformationWidget` (**11.3**) |
| **Deformation limited to selection** | ⏳ TBD | Selection (sub-feature 3) not yet bound to “offset selected points only” in `DeformationData` |

Recommended workflow (available now):

```text
Load time series → feature extraction / vortex predict → cloud-map key fields
    → animation panel for timesteps (key-event temporal evolution)
    → (optional) deformation panel for whole-mesh displacement
```

After region-scoped deformation is enhanced, the expected flow is:

```text
Click / box-select key region → write deformation offsets only for that set → play over time
```

### Source Paths (cross-module)

| Path | Notes |
|------|-------|
| `iGameCore/Core/DataModel/iGameDataObject.*` | `UpdateAnimation` |
| `iGameCore/IO/VTK XML/iGamePVDReader.*` | Time-series PVD |
| `iGameCore/Filters/Deformation/iGameStressDeformationFilter.*` | Structural deformation |
| `Qt/src/IQWidgets/igQtDeformationWidget.*` | Deformation dock |
| `doc/modules/README_11.3.md` | Full time / deformation / animation notes |

### GUI

| Panel | Notes |
|-------|-------|
| Animation / time-series docks | Play key feature fields over time (11.3) |
| Deformation dock / `igQtDeformationWidget` | Displacement vector, scale factors, enable deformation |

---

## Acceptance Notes (Accuracy Target)

| Item | Notes |
|------|-------|
| Target | Precision ≥ 90% **and** Recall ≥ 90% |
| GT attribute name | `PredictedLabel` (point scalar, same length as points) |
| Thresholds | GT `> 0`; prediction `> 0.5` |
| API | `VortexDetection::GetPrecision()` / `GetRecall()` / `GetAccuracy()` |
| Without GT | getters stay `-1`; only `vortexPredict` cloud map is produced |

---

## Related Examples (summary)

| Target | Notes | Condition |
|--------|-------|-----------|
| `testGradientExtraction` | Gradient | default |
| `testCurvatureExtraction` | Curvature | default |
| `testLaplacianExtraction` | Laplacian | default |
| `testVortexExtraction` | Classical vorticity | default |
| `testVortexDetection` | NN vortex detection | `ENABLE_LIBTORCH_MODULE=ON` |
