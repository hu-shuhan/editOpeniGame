# Metric 10.2: Key Feature Extraction from Simulation Data

## Composition

For CAE physical-field data, this metric provides key feature-field extraction, neural-network vortex detection with comparison against manual annotations, and support for key-region interaction plus temporal evolution of key events. The technical target is: **key feature extraction Precision / Recall ≥ 90%**.

| # | Sub-feature | Status |
|---|-------------|--------|
| 1 | Classical physical features: gradient / curvature / Laplacian / vorticity / isolines and isosurfaces | ✅ Implemented |
| 2 | NN-based vortex extraction vs. manual labels; Accuracy / Precision / Recall (target ≥ 90%) | ✅ Implemented (metrics); GUI overlay pending restore |
| 3 | Per-data-type program interfaces enabling feature extraction at different precision levels | ✅ Implemented (isolines / isosurfaces × surface / volume × original / simplified meshes) |
| 4 | Temporal evolution of key events; deformation applied only to the selected region | ⏳ Partial (Phase 1: surface color / opacity mapping ✅; time series / deformation in 11.3; region-limited deformation TBD) |

> This document covers sub-features **1**, **2** and **3** in full, and how **4** connects to existing interaction and visualization modules.
> Key-region click / box selection itself is covered by **10.3** and the `Selection` module and is not repeated here.
> Difference from **10.1**: 10.1 focuses on **analysis data generation**; 10.2 focuses on **feature-field extraction and vortex detection evaluation**.
> Difference from **11.3**: 11.3 provides generic **time switching, structural deformation, and animation export**; 10.2 key-event temporal views rely on those capabilities.

---

## Sub-feature 1: Classical physical feature extraction (other key features)

### Description

From the currently selected physical attribute (scalar / vector), extract classical differential-geometry and fluid-mechanics features on the mesh for downstream vortex detection and region analysis. They fall into two groups by **output form**:

**A. Feature scalar fields**: written into the source model's `AttributeSet` and displayable as cloud maps.

| Feature | Output attribute | Notes |
|---------|------------------|-------|
| Gradient | `gradient_<source attribute>` | Scalar input → 3-component gradient vector; vector input → 9-component gradient tensor |
| Curvature | `curvatures` | Surface curvature (cotangent-style discrete) |
| Laplacian | `laplacians` | Discrete Laplacian |
| Vorticity | `vorticities` | Classical \(\omega = \nabla \times v\) (not neural) |

> The gradient output name now carries the source attribute name, e.g. `gradient_V` for a source attribute `V`, so computing gradients of different attributes no longer overwrites each other.

**B. Feature geometry**: the result is a **new mesh object** added to the model tree as its own model, rather than an appended attribute.

| Feature | Output | Notes |
|---------|--------|-------|
| Isolines / isosurfaces | New `UnstructuredMesh` named `<name>_Contour` | Contours of a scalar field at given iso values; several iso values may be passed at once |

Isolines and isosurfaces are the **same algorithm** (`ContourFilter`) dispatching on the dimension of each input cell:

| Input cell | Output cell | Result |
|------------|-------------|--------|
| 2D cells (triangle / quad / polygon and their quadratic forms) | `IG_LINE` | **Isolines** (marching-squares style case tables) |
| 3D cells (tet / hex / prism / pyramid / polyhedron and their quadratic forms) | `IG_TRIANGLE` | **Isosurfaces** (marching-tetrahedra style case tables) |

Polygons are fan-triangulated first and then treated as triangles; polyhedra go through `clipCelltoTetra()` and are then treated as tetrahedra. A mixed mesh yields both line segments and triangles in the same output `UnstructuredMesh`. Point / cell attributes of the source model are remapped onto the contour via the interpolated edges and originating cells, so the contour object itself can be colored.

### Supported Mesh Types (important)

Input requirements differ per feature:

| Feature | Required input | What to do with a volume mesh (3D cells) |
|---------|----------------|------------------------------------------|
| Gradient | Surface, volume, structured, mixed and polyhedral meshes | **Compute directly**; no surface extraction needed |
| Curvature / Laplacian | **Surface mesh** (all 2D cells) | Run **Surface Extraction** first, then compute on the extracted surface |
| Vorticity | **3D volume cells** | Compute directly on the volume mesh; a surface-only mesh is not supported |
| Isolines / isosurfaces | Surface, volume, unstructured and structured meshes — **all accepted** | Run it directly: 2D cells give isolines, 3D cells give isosurfaces; no surface extraction needed |

A **surface mesh** (`IG_SURFACE_MESH`, or an `IG_UNSTRUCTURED_MESH` made entirely of 2D cells) takes gradient / curvature / Laplacian directly.

A **volume mesh** (`IG_VOLUME_MESH`, or an `IG_UNSTRUCTURED_MESH` containing tets, hexes, … ) can compute gradient directly. For **curvature / Laplacian**, it needs one extra step first:

> Menu **Filters → Data Processing → Surface Extraction**

This extracts the model's boundary faces into a standalone surface-mesh object named `<name>_surface` and adds it to the model tree. Select that `_surface` object in the tree, then run curvature / Laplacian on it.

Two caveats:

- **The "shell" you see while rendering is not a surface mesh.** The shell lives in `DrawObject`'s `m_RenderableMesh.SurfaceMesh` and exists only for the renderer; the data object itself is still a volume mesh, and that is what filters read. Surface Extraction must be run explicitly so the surface becomes its own object in the model tree.
- **After extraction you are computing on the boundary.** Curvature / Laplacian on a surface mesh are surface quantities, not the same as the 3D gradient of the volumetric field — keep that in mind when reading the result.

Running curvature / Laplacian directly on a volume mesh raises `Not Surface Mesh !`. That is the filter's default message and simply means "this input is not a surface mesh" (the volume-mesh branch is not wired up). **Gradient is no longer limited by this** — it can be run on volume meshes directly.

### Source Paths

| Path | Class | Notes |
|------|-------|-------|
| `iGameCore/Filters/FeatureExtraction/iGameAdvancedGradientFilter.*` | `AdvancedGradientFilter` | Gradient (surface / volume / structured / mixed / polyhedral) |
| `iGameCore/Filters/FeatureExtraction/iGameCurvatureFilter.*` | `CurvatureFilter` | Curvature |
| `iGameCore/Filters/FeatureExtraction/iGameLaplacianFilter.*` | `LaplacianFilter` | Laplacian |
| `iGameCore/Filters/FeatureExtraction/iGameVortexFilter.*` | `VortexFilter` | Classical vorticity |
| `iGameCore/Filters/Contour/iGameContourFilter.*` | `ContourFilter` | Isolines / isosurfaces (dispatched by cell dimension) |
| `iGameCore/Filters/Contour/iGameCellContour.h` | `CellContour::Contour` | Per-cell case tables and edge interpolation |
| `Qt/src/IQWidgets/igQtContourExtractWidget.*` | `igQtContourExtractWidget` | Contour-extraction dock |
| `iGameCore/Filters/iGameFilterIncludes.h` | — | Shared includes |

### How It Is Called

From `Examples/Filter/FeatureExtraction/GradientExtraction.cpp`:

```cpp
auto dataObj = iGame::FileIO::ReadFile("./Models/pipedcylinder2d_gt.vtk");
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);

auto filter = iGame::AdvancedGradientFilter::New();  // new gradient filter
filter->SetInput(drawObj);
filter->SetAttributeByIndex(attrIndex);             // or SetAttributeByName(name)
filter->SetComputeGradientTensor(true);             // vector input → 9-component tensor
filter->SetOutputToPointData(true);                 // output point data
filter->Execute();

int newIndex = drawObj->GetAttributeSet()->GetNumberOfAttributes() - 1;
drawObj->ViewCloudPicture(scene, newIndex);
```

Common pattern: `Filter::New()` → `SetInput()` → (optional) `SetAttributeByIndex/Name` → configure output options → `Execute()`; results are appended to `AttributeSet`.

> The new gradient output name is `gradient_<source attribute>`, e.g. `gradient_pressure` for a source attribute `pressure`. To look it up by name:
> ```cpp
> int idx = dataObj->GetAttributeSet()->GetAttributeIndex("gradient_pressure");
> ```

**Isolines / isosurfaces** use a different interface — driven by values rather than an attribute index, and producing a new mesh. From `Examples/Filter/TestContourLine.cpp`:

```cpp
auto obj = iGame::FileIO::ReadFile("./Models/Tet_Plane.vtk");

auto pointAttributes = obj->GetAttributeSet()->GetAllPointAttributes();  // point scalars only
auto& attr = pointAttributes->GetElement(index);
auto array = attr.pointer;
auto range = attr.GetDataRange();
int dimension = 0;                      // which component of a multi-component attribute

// Several iso values may be passed at once; results are merged into one output mesh
std::vector<double> values;
values.push_back(range->GetValue(dimension * 2 + 2) * 2 / 3 + range->GetValue(dimension * 2 + 3) / 3);
values.push_back(range->GetValue(dimension * 2 + 2) / 3 + range->GetValue(dimension * 2 + 3) * 2 / 3);

auto filter = iGame::ContourFilter::New();
filter->SetInput(obj);
filter->SetIsoScalarData(array, values, dimension);   // single value: SetIsoScalarData(array, value, dimension)
filter->Execute();

auto res = filter->GetContourMesh();    // or GetOutput(); an UnstructuredMesh
scene->AddModel(res);
auto draw = iGame::DynamicCast<iGame::DrawObject>(res);
draw->SetViewStyle(IG_SURFACE | IG_WIREFRAME);        // isolines are line cells — IG_WIREFRAME is required
draw->ViewCloudPicture(scene, index, dimension);
```

Three caveats:

- **Point scalars only**: the GUI's attribute combo box comes from `GetAllPointAttributes()`; cell attributes are not offered.
- **View style**: `DrawObject` defaults to `IG_SURFACE` only, while an isoline output is all line cells with no triangles — without `IG_WIREFRAME` you get the right point/cell counts and an empty viewport. The GUI sets this automatically from the output cell dimensions.
- **Iso values outside the data range** intersect no cell, so the output is empty; the GUI reports this instead of producing an empty model.

### GUI

| Menu item | Filter | Required input |
|-----------|--------|----------------|
| Filters → Data Processing → Surface Extraction | → `<name>_surface` surface object | **Prerequisite** for curvature / Laplacian on a volume mesh; gradient does not need it |
| Filters → Feature Extraction → ComputeGradient | `AdvancedGradientFilter` | Surface / volume / structured / mixed / polyhedral mesh |
| Filters → Feature Extraction → Compute Laplacian | `LaplacianFilter` | Surface mesh |
| Filters → Feature Extraction → Compute Curvature | `CurvatureFilter` | Surface mesh |
| Filters → Feature Extraction → Compute Vorticity | `VortexFilter` | 3D volume cells |
| Tool panel → Contour Extraction (isolines/isosurfaces) / `action_ContourExtract` | `dockWidget_ContourExtract`: pick a point scalar, a component, and an iso value | Any mesh |

Typical order of operations:

- **Surface mesh**: select the model → select the attribute → Feature Extraction → Gradient / Laplacian / Curvature
- **Volume mesh (gradient)**: select the model → select the attribute → Feature Extraction → Gradient; **no surface extraction needed**
- **Volume mesh (curvature / Laplacian)**: select the model → Data Processing → Surface Extraction → select the new `<name>_surface` node → select the attribute → Feature Extraction → Curvature / Laplacian
- **Vorticity**: no extraction needed — select the velocity vector attribute on the volume mesh and run it
- **Isolines / isosurfaces**: select the model → open the Contour Extraction panel → pick a point scalar and component (the panel shows that component's value range) → enter an iso value → run. The result joins the model tree as its own model `<name>_Contour`, can be shown / hidden / colored independently, and re-running with a new value updates the same result object in place

Results appear in the model-tree attribute list and can be shown via `dockWidget_ScalarField` / `igQtScalarViewWidget`.

### Related Examples

| Target | Notes |
|--------|-------|
| `testGradientExtraction` | Gradient |
| `testAdvancedGradientFilter` | Gradient regression test (`./Models/StreamTest.vtk`) |
| `testCurvatureExtraction` | Curvature |
| `testLaplacianExtraction` | Laplacian |
| `testVortexExtraction` | Classical vorticity |
| `testContourLine` | Isolines / isosurfaces (`./Models/Tet_Plane.vtk`, three iso values at once) |
| `testContourExtraction` | Per-data-type dispatch: `./Models/driver_1.vtk` + `./Models/streamTet.vtk` (bring your own) |

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

## Sub-feature 3: Per-data-type interfaces for extraction at different precision levels

### Description

Dedicated program interfaces are provided per mesh data type, so that one and the same extraction capability works across data of **different precision (mesh density)**. Two orthogonal dimensions are involved:

- **Data type**: surface / volume / unstructured / structured meshes. The filter dispatches internally on `GetDataObjectType()`; callers need not distinguish them.
- **Mesh precision**: original vs. simplified meshes. Both surface and volume simplification interfaces are provided, and both carry the attribute field along, so a simplified result feeds straight back into extraction.

Taking **isolines / isosurfaces** as the example: a surface mesh yields isolines, a volume mesh yields isosurfaces; running the same iso value on the original and on the simplified mesh produces contours of different fineness, which is how precision is traded against cost.

### Interface dispatch: data type → execution path

`ContourFilter::Execute()` dispatches on the data-object type; each of the four input kinds has its own path:

| Input data type | Execution path | Internal handling | Output |
|-----------------|----------------|-------------------|--------|
| `IG_SURFACE_MESH` | `ExecuteWithSurfaceMesh` | `GenerateFromSurfaceMesh` converts to `UnstructuredMesh`, then shared handling | **Isolines** (`IG_LINE`) |
| `IG_VOLUME_MESH` | `ExecuteWithVolumeMesh` | Ordinary volume meshes go through `GenerateFromVolumeMesh`; polyhedral ones divert to `ExecuteWithVolumeMeshWithPolyhedronType` | **Isosurfaces** (`IG_TRIANGLE`) |
| `IG_UNSTRUCTURED_MESH` | `ExecuteWithUnstructuredMesh` | Per-cell dispatch by dimension: 2D cells give segments, 3D cells give triangles | Isolines / isosurfaces / both |
| `IG_STRUCTURED_MESH` | Reuses `ExecuteWithVolumeMesh` | Same as volume | **Isosurfaces** |

### Precision levels: original vs. simplified meshes

Simplification comes in a surface set and a volume set, both attribute-preserving:

| Target | Menu (Filters → Data Processing) | Class | Main parameters |
|--------|----------------------------------|-------|-----------------|
| Surface | Surface Simplification | `MeshSimplificationFilter` | reduction ratio (0..1), preserve boundary, check all scalars, geometric-similarity metric |
| Surface | Fast Surface Simplification | `MeshSimplificationFilterPro` | target reduction (0..1), target face count, preserve boundary |
| Surface | Surface Simplification | `MeshSimplifierWithAttributes` + `MeshSaliency` | Reduction, Target Face Count, attribute weights (saliency-guided) |
| Volume | Tetra Edge-Collapse Simplification | `TetraEdgeSimplification` | Reduction (0..1), Target Tetra Count, Boundary Penalty, Lambda, Preserve Boundary, Use All Point Attributes, Stretch Factor, Max Aspect Ratio |

Simplification produces a **new mesh object**; once in the model tree it can be used directly as contour input. Because attributes travel with it (the surface side's "check all scalars", the volume side's `Use All Point Attributes`), the same named scalar remains selectable on the simplified mesh.

### The four combinations

| Combination | Input | Result | Notes |
|-------------|-------|--------|-------|
| ① Surface · original | Surface mesh / all-2D unstructured mesh | Isolines | Baseline precision; densest segments, closest to the original geometry |
| ② Surface · simplified | Output of surface simplification | Isolines | Segment count drops with the reduction ratio; contour shape coarsens |
| ③ Volume · original | Volume / 3D-cell unstructured / structured mesh | Isosurfaces | Baseline precision; densest triangles |
| ④ Volume · simplified | Output of tetra edge-collapse simplification | Isosurfaces | Fewer triangles; isosurface detail is smoothed away |

Contour fineness is dictated directly by the **cell density of the input mesh** — every contour vertex lies on an **edge** of a cut cell, so denser cells mean more cut edges and more intersection points. Running the same iso value on meshes of different precision is therefore the most direct way to control contour precision, with no algorithm parameter to tune.

### How It Is Called

```cpp
// (1) Extract directly on the original mesh — surface / volume / unstructured / structured all dispatch internally
auto obj = iGame::FileIO::ReadFile(fileName);
auto contour = iGame::ContourFilter::New();
contour->SetInput(obj);
contour->SetIsoScalarData(array, value, dimension);
contour->Execute();
auto res0 = contour->GetContourMesh();

// (2) Simplify first, then extract (surface; for volumes swap in TetraEdgeSimplification)
auto simp = iGame::MeshSimplificationFilterPro::New();
simp->SetInput(obj);
simp->SetTargetReduction(0.5f);      // or SetTargetFaceCount(n)
simp->SetPreserveBoundary(true);
simp->Execute();
auto simplified = simp->GetOutput();

auto contour2 = iGame::ContourFilter::New();
contour2->SetInput(simplified);
contour2->SetIsoScalarData(array2, value, dimension);   // array2 taken from the simplified output
contour2->Execute();
auto res1 = contour2->GetContourMesh();
```

After simplification the attribute array **must be re-fetched from the simplified output**: simplification rebuilds the `AttributeSet`, so the original model's `ArrayObject` pointer no longer matches the point count.

### GUI

| Step | Entry |
|------|-------|
| 1. (optional) lower the precision | Filters → Data Processing → Surface Simplification / Fast Surface Simplification / Surface Simplification / Tetra Edge-Collapse Simplification |
| 2. select the target model | Pick the original model, or the new model produced by simplification |
| 3. extract the contour | Tool panel → Contour Extraction (isolines/isosurfaces) → point scalar, component, iso value → run |
| 4. compare | Each run adds its own `<name>_Contour` model, so several precision levels can be displayed side by side |

### Usage Notes

1. The higher the reduction ratio (the fewer cells kept), the coarser the contour; run the same iso value at several reduction ratios to compare.
2. Surface simplification only affects isolines and volume simplification only isosurfaces — they are not interchangeable. To get boundary isolines from a volume mesh, run Surface Extraction first, then simplify.
3. Simplification changes cell counts and point numbering, so contour vertex counts shift with it; for quantitative comparison use geometric measures (isoline length / isosurface area) rather than vertex counts.

### Test Cases

| Target | Source | Default data | Notes |
|--------|--------|--------------|-------|
| `testContourExtraction` | `Examples/Filter/TestContourExtraction.cpp` | `./Models/driver_1.vtk` (surface mesh) + `./Models/streamTet.vtk` (tetrahedral volume mesh), both bring-your-own | One interface dispatching over two data kinds: the surface mesh yields isolines, the volume mesh yields isosurfaces |

The case runs both models: it takes point scalar 0, derives three iso values from the data range and passes them in one call, then counts the line segments and triangles in the output per cell and asserts

- the surface mesh (`driver_1.vtk`) must produce `IG_LINE`, otherwise it reports `expected iso-lines but got no line segment`;
- the volume mesh (`streamTet.vtk`) must produce `IG_TRIANGLE`, otherwise it reports `expected iso-surfaces but got no triangle`.

That is a direct check of `ContourFilter`'s dispatch-by-cell-dimension logic. Both contours are added to the same scene for display, with shell rendering turned off and `IG_SURFACE | IG_WIREFRAME` set — without the latter, a pure line-cell isoline would be invisible.

> Neither model ships with the repository; place them under `./Models/` in the run directory. The program prints `exists=0/1` at startup so a missing file is obvious.

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
| Scalar → color / opacity mapping (surface rendering) | ✅ Phase 1 implemented | `ScalarsToColors::SetOpacityMappingEnabled` + `TransparencyLink` transparency pipeline; "Opacity Mapping" toggle in the Scalar View dock |
| Whole-mesh structural deformation | ✅ Implemented | `StressDeformationFilter` + `igQtDeformationWidget` (**11.3**) |
| **Deformation limited to selection** | ⏳ TBD | Selection (interaction layer, see 10.3) not yet bound to “offset selected points only” in `DeformationData` |

### Phased implementation: scalar-to-color / opacity mapping (Phase 1)

As the first phase of **temporal evolution of key events**, the platform now maps the selected scalar field to both color and opacity **in surface rendering** (points, wireframe and volume rendering share the same mapping chain):

- **Color mapping**: reuses the `ScalarsToColors` color bar — attribute value → RGB;
- **Opacity mapping**: when enabled, each vertex gets an alpha derived from its attribute value (currently a linear transfer function `opacity = normalized value`, see `ColorMap::MapOpacity`), multiplied by the object's overall transparency before entering the transparency pipeline;
- **Render path**: in `TransparencyLink.frag`, both `colorMode==0` (surface + lighting) and `colorMode==1` (unlit) output `in_Color.a * objectData.transparent`; `DrawWithTransparency` enters this path as soon as opacity mapping is enabled (no need to lower the overall transparency first), and per-pixel OIT sorting keeps blending correct;
- **Entry points**: GUI — tick "Opacity Mapping" in the Scalar View dock (`dockWidget_ScalarField` / `igQtScalarViewWidget`); API — `Scene::SetOpacityMappingEnabled` / `DrawObject::SetOpacityMappingEnabled` (recursive over sub-data-objects, covering PVD frame blocks); the volume-rendering example is `Examples/Rendering/SetVolumeRendering.cpp`.

Usage for key-event temporal evolution: on a single frame, "color highlight + semi-transparent low-value regions" already isolates key event regions; combined with the animation playback in **11.3**, switching attributes frame by frame shows how key regions evolve over time.

**Roadmap** (implementation path of this sub-feature):

| Phase | Content | Status |
|-------|---------|--------|
| Phase 1 | Scalar → color / opacity mapping (surface rendering) | ✅ Implemented (this section) |
| Phase 2 | Threshold-based point filtering with separate opacity for selected / unselected points (`AttributeOpacityFilter`) | ⏳ Planned |
| Phase 3 | Per-frame attribute difference (`TimeDifferenceFilter`) as the filter condition, highlighting rapidly changing regions during playback | ⏳ Planned |

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
| `iGameCore/Core/Common/iGameScalarsToColors.*` / `iGameColorMap.*` | Scalar → RGBA color / opacity mapping |
| `iGameCore/Rendering/Shaders/GLSL/TransparencyLink.frag` | Per-vertex alpha in the surface transparency pipeline (OIT sorting) |
| `Qt/src/IQWidgets/igQtScalarViewWidget.*` | "Opacity Mapping" toggle |
| `iGameCore/Filters/Deformation/iGameStressDeformationFilter.*` | Structural deformation |
| `Qt/src/IQWidgets/igQtDeformationWidget.*` | Deformation dock |
| `doc/modules/README_11.3.md` | Full time / deformation / animation notes |

### GUI

| Panel | Notes |
|-------|-------|
| Animation / time-series docks | Play key feature fields over time (11.3) |
| Deformation dock / `igQtDeformationWidget` | Displacement vector, scale factors, enable deformation |
| Scalar View dock (`dockWidget_ScalarField` / `igQtScalarViewWidget`) | Tick "Opacity Mapping" to map the current scalar to color plus opacity |

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
| `testContourLine` | Isolines / isosurfaces | default |
| `testContourExtraction` | Surface → isolines, volume → isosurfaces (dispatch by data type) | default |
| `testVortexDetection` | NN vortex detection | `ENABLE_LIBTORCH_MODULE=ON` |
