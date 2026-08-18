# Metric 10.1: Intelligent Visual Analysis for CAE Simulation Data

## Composition

For multi-dimensional CAE physical-field data, this metric provides interaction-driven, data-feature-guided intelligent visual analysis. It comprises four sub-features:

| # | Sub-feature | Status |
|---|-------------|--------|
| 1 | Point-driven local-region chart analysis: pick a point, auto-determine a bounding box from the current attribute, and run chart analysis on that local region | ✅ Implemented |
| 2 | Entropy-based automatic streamline seed placement for a region | ✅ Implemented |
| 3 | Intelligent streamline filtering | ✅ Implemented |
| 4 | Semantic-segmentation-based table attributes | ✅ Implemented |

> This document covers the completed sub-features **1**, **2**, **3**, and **4**.

---

## Sub-feature 1: Point-driven local-region chart analysis

### Description

The user picks a point (or cell / box selection) on the 3D model. The system automatically computes the bounding box of that selection from the current attribute, narrows the analysis scope to this local region, and generates charts over it (plot-line / radial, parallel coordinates, variable correlation, variable density) to reveal local variable relationships and distributions.

Flow: **pick point / box → auto bounding box → local-region chart analysis**.

### Source Paths

| Path | Description |
|------|-------------|
| `Qt/src/IQWidgets/igQtDataChangeWidget.cpp` / `Qt/include/IQWidgets/igQtDataChangeWidget.h` | Local-region chart panel (selection, radial chart, chart drawing) |
| `iGameCore/Filters/PlotLine/iGameGeneratePlotLineDataFilter.*` | Plot-line / region data generation |
| `iGameCore/Filters/ParallelCoordinates/iGameGenerateParallelCoordinatesData.*` | Parallel coordinates |
| `iGameCore/Filters/VariableCorrelation/iGameGenerateVariableCorrelationDataFilter.*` | Correlation matrix |
| `iGameCore/Filters/VariableDensity/iGameGenerateVariableDensityDataFilter.*` | Density distribution |
| `iGameCore/Rendering/Core/Interactor/iGameBoxStyle.*` | Box-selection interactor, supplies bounding-box extreme points |

### Key Implementation

1. **Selection → bounding box**: take the extreme points of the selection (`SelectBox`'s `BoxStyle`) and build the bounding box:

   ```cpp
   auto boxStyle = DynamicCast<iGame::BoxStyle>(interactor->GetSpecialInteractor("SelectBox"));
   auto minMaxP  = boxStyle->GetBox()->GetExtremePoint();
   auto boundingBox = BoundingBox(minMaxP.first, minMaxP.second);
   SetRadialPoint(boundingBox);   // bind the analysis scope to this local region
   ```

   Without an explicit box selection, it falls back to the whole-model bounding box `m_Mesh->GetBoundingBox()`.

2. **Local-region charts**: generate `PlotLineData` over the selected region by attribute, and build layers based on each variable's hue / saturation:

   ```cpp
   auto Data = PlotLineData::New(attrs, dataType, MIN_H, MAX_H, MIN_S, MAX_S);
   ```

3. **Selection linkage**: selection changes are delivered through the `SelectionCallbackEvent(itemType, ids, ope)` callback, refreshing the images of the selected variables in real time (`GenerateChoosedVariableImage`).

### GUI

| Dock panel | Widget | Description |
|------------|--------|-------------|
| `dockWidget_DataChangeField` | `igQtDataChangeWidget` | Pick point / box → local-region radial / plot-line charts |
| `dockWidget_ParallelCoordinatesField` | `igQtParallelCoordinatesWidget` | Parallel coordinates |
| `dockWidget_VariableCorrelationField` | `igQtVariableCorrelationWidget` | Variable correlation |
| `dockWidget_VariableDensityField` | `igQtVariableDensityWidget` | Variable density |

---

## Sub-feature 2: Entropy-based automatic streamline seed placement

### Description

Automatically computes streamline seed points for a region's vector field: the region is divided into a uniform box grid, and **directional information entropy** measures how complex the flow direction is inside each box. Seeds are preferentially placed in the highest-entropy boxes (where flow directions are most disordered and structure is richest), concentrating a limited number of seeds on the most worthwhile regions and avoiding the waste and omissions of uniform seeding.

### Source Paths

| Path | Class / Function | Description |
|------|------------------|-------------|
| `iGameCore/Filters/StreamView/iGameStreamTracer.*` | `StreamTracer::getEntropySeeding` | Entropy-based seed computation (core) |
| `iGameCore/Filters/StreamView/iGameStreamTracer.*` | `StreamTracer::getModelSelect` / `getAllSubBlockCenters` / `computeSubBlockCenters` | Derive the focus bounding box from the selection and split it into sub-block centers |
| `iGameCore/Core/Common/iGamePointFinder.h` | `PointFinder` (`GetNumberOfBoxes` / `GetPointsInBox`) | Uniform box-grid spatial partition |
| `Qt/src/IQWidgets/igQtStreamTracerWidget.cpp` | `generateStreamline` | GUI trigger for seed generation and streamline computation |

### Algorithm

`getEntropySeeding(vectorName, topPercent = 0.1f, ptsPerExtrema = 2)`:

1. **Box partition**: `PointFinder` divides the point set into `GetNumberOfBoxes()` spatial boxes; `GetPointsInBox(i)` yields the points inside box `i`.
2. **Directional entropy** (parallel, per box): each point's vector is binned by direction — using a Lambert cylindrical equal-area projection, azimuth `φ` into 24 bins and polar `cosθ` into 15 bins, for 360 equal-area direction bins total; per-bin probabilities `p` give the Shannon entropy `H = -Σ p·ln(p)`.
3. **Region selection**: sort boxes by descending entropy and take the top `topPercent`.
4. **Extremum seeding** (parallel): within each selected box, sort points by vector magnitude and take `ptsPerExtrema` smallest-magnitude and largest-magnitude points as seeds (covering low-speed stagnation zones and high-speed zones).

The computation is parallelized with `ThreadPool::parallelFor`.

### How It Is Called

```cpp
auto tracer = iGame::StreamTracer::New();
tracer->initStreamTracer(model);        // bind model / mesh
tracer->AddPtFinder(pointFinder);       // supply the box partition
// Entropy seeds: top 2.5% highest-entropy boxes, 8 extremum points per box
auto seeds = tracer->getEntropySeeding(vectorName, 0.025f, 8);

// Optionally combine with uniform sub-block centers, then compute streamlines
tracer->SetInput(seeds, vectorName, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
tracer->Execute();
auto streamlines = tracer->GetOutput();
```

### GUI

In the streamline panel `igQtStreamTracerWidget`, the seed mode `control == 6` is "entropy seeding": it first calls `getEntropySeeding` to obtain seeds in high-entropy regions, then overlays uniform sub-block centers via `computeSubBlockCenters`, and finally generates streamlines.

Region sourcing also supports "auto bounding box from selection": `getModelSelect` derives the bounding box from the selected points / cells, then splits sub-block centers inside the focus region as the seeding scope.

### Related Examples

| Example | Description |
|---------|-------------|
| `Examples/Filter/Vector/TestStreamline.cpp` | Streamlines and seed generation |

---

## Sub-feature 3: Intelligent streamline filtering

### Description

A single seeding pass often produces hundreds or thousands of streamlines; displaying them all causes occlusion and clutter. Intelligent filtering clusters streamlines by **shape features** and picks a representative from each cluster, greatly reducing the count while preserving the structural diversity of the flow field — it removes largely redundant streamlines without losing representative flow patterns. Results are colored by cluster (`ClusterLabel`) so different flow patterns are easy to distinguish.

### Source Paths

| Path | Class / Function | Description |
|------|------------------|-------------|
| `iGameCore/Filters/StreamView/iGameStreamlineSimplifier.*` | `StreamlineSimplifier` | Streamline filtering (core) |
| `Qt/src/IQWidgets/igQtStreamTracerWidget.cpp` | `igQtStreamTracerWidget::Simplifier` | GUI trigger, original-streamline caching, result coloring |

### Algorithm

`StreamlineSimplifier::Execute()` has five stages:

1. **Streamline reconstruction `ExtractStreamlines`**: reassemble `IG_LINE` cells into individual streamlines using `StreamlineId` (cell attribute) and `Velocity` (point attribute); drop streamlines with `< 3` points, and filter by arc length (discard those shorter than 10% of the median arc length).
2. **Curvature histogram `ComputeHistograms`**: along each streamline, compute the angle between consecutive direction vectors as discrete curvature, bin it into `CurvBins` (default 40) bins, and obtain the normalized histogram and its CDF.
3. **Distance matrix `BuildDistanceMatrix`**: extract a 5-D shape feature per streamline (bendRatio, maxTurn, top-K mean turn, mid-region mean turn, curved-segment ratio), normalize, and take a weighted L1 distance (weights `0.10/0.35/0.25/0.20/0.10`); add a curvature-CDF distance to form `0.95·featureDistance + 0.05·curvatureDistance`.
4. **Hierarchical clustering `ClusterAverage`**: average-linkage agglomerative clustering merged down to `NumClusters` clusters.
5. **Representative sampling `SampleRepresentatives`**: allocate quotas across clusters up to `TotalTarget` (small clusters fully kept, surplus reassigned to large clusters), then pick representatives within each cluster at equal (linspace) intervals.

`BuildOutputMesh` rebuilds the output mesh and writes `Velocity` and `ClusterLabel` attributes for coloring.

### Parameters

| Setter | Meaning | Default |
|--------|---------|---------|
| `SetCurvBins(int)` | Curvature histogram bin count | 40 |
| `SetNumClusters(int)` | Target number of clusters | 20 |
| `SetTotalTarget(int)` | Total streamlines to keep after filtering | 50 |

### How It Is Called

```cpp
auto simp = iGame::StreamlineSimplifier::New();
simp->SetInput(streamlineMesh);   // input: streamline mesh with StreamlineId / Velocity
simp->SetCurvBins(40);
simp->SetNumClusters(clusterCount);
simp->SetTotalTarget(keepCount);
if (simp->Execute()) {
    auto simplified = simp->GetOutput();   // output: representative streamlines with ClusterLabel
}
```

### GUI

In the streamline panel `igQtStreamTracerWidget`, first select a generated streamline object in the model tree (name contains `_StreamLine`), then click the **Cluster** button to trigger `Simplifier()`: the cluster count comes from `clusterSpin` and the kept total from `perClusterSpin`. The first filtering caches a snapshot of the original streamlines, so parameters can be re-tuned repeatedly without losing the original data; results are colored by the `ClusterLabel` cloud map.

---

## Sub-feature 4: Semantic-segmentation-based table attributes

### Description

The system runs P3SAM part-semantic segmentation on the current CAE mesh, obtains a per-cell `part_id` from the segmented mesh, and maps the low-resolution result back to the original high-resolution mesh. The mapped result is written into the current original `DataObject::AttributeSet` as an `IG_BLOCK_MAPPING` attribute attached to `IG_CELL`, making it available to subsequent UI components and algorithms. The Search Data panel then reads point and cell attributes dynamically, creates table columns, and supports queries by `part_id`, other physical-field attributes, and the current selection.

Flow: **current model → triangulation / simplification → P3SAM segmentation → read `part_id` → map to original cells → write to `AttributeSet` → table query and part linkage**.

`part_id` is an integer identifier for a segmented region or part; it does not itself contain a human-readable category such as “bolt” or “blade.” A semantic service must return and attach additional attributes such as `class_id` or `class_name` if category names are required.

### Source Paths

| Path | Class / API | Description |
|------|-------------|-------------|
| `iGameCore/Core/Common/P3SAM/iGameP3SAMSegmenter.*` | `P3SAMSegmenter` | Full pipeline for triangulation, simplification, segmentation requests, VTK parsing, and mapping back to the original mesh |
| `iGameCore/Core/Common/P3SAM/iGameP3SAMClient.*` | `P3SAMClient` | Sends OBJ data and the post-processing flag over TCP and receives a VTK segmentation result |
| `iGameCore/Core/Common/iGameBlockMapping.*` | `BlockMapping` | Reads `part_id` from the segmented mesh and maps it spatially to the original surface, unstructured, or volume mesh |
| `iGameCore/Core/DataModel/iGameDataObject.*` | `SetBlockMapping` / `GetBlockMapping` / `HasBlockMapping` | Writes and reads the mapped array as an `IG_BLOCK_MAPPING + IG_CELL` attribute in the current `AttributeSet` |
| `Qt/src/IQWidgets/igQtSearchInfoWidget.*` | `igQtSearchInfoWidget` | Dynamic table columns, numeric queries, selection / part filtering, and pagination |
| `Qt/src/IQWidgets/igQtPartFocusWidget.*` | `igQtPartFocusWidget` | Enumerates `part_id` values, counts cells per part, and links part selection to the table, camera, and selection box |
| `Qt/src/IQCore/igQtMainWindow.cpp` | **Part Segmentation**, **Part Segmentation (From File)**, **Part Focus** | GUI entry points and signal connections between components |

### Algorithm and Data Flow

1. **Mesh preprocessing**: `P3SAMSegmenter::Execute()` converts the input to a drawable surface, triangulates and simplifies it according to `m_simplificationRatio`, and exports OBJ data to reduce network traffic and server-side inference cost.
2. **Semantic segmentation**: `P3SAMClient` sends `[post_process][obj_size][obj_data]` to the configured P3SAM service and receives a VTK result. The returned mesh must contain an integer per-cell attribute named exactly `part_id`.
3. **Mapping the low-resolution result back**: `BlockMapping::GetPartId` retrieves `part_id` from the segmented mesh. The system builds a voxel lookup grid from segmented-cell centroids and their IDs, queries the centroid of every original cell, and writes the nearest segmented region's ID to a new `IntArray`. Centroid collection, voxel assignment, and original-cell queries use `ThreadPool::parallelFor`.
4. **Writing to the original attribute set**: after naming the mapped array `part_id`, the segmenter executes:

   ```cpp
   resultArray->SetName("part_id");
   original->SetBlockMapping(resultArray);
   ```

   `SetBlockMapping` ultimately calls:

   ```cpp
   m_Attributes->AddAttribute(IG_BLOCK_MAPPING, IG_CELL, resultArray);
   ```

   Therefore, the current original model's `AttributeSet` is modified directly; the result is not kept only in the temporary simplified mesh or inside `P3SAMSegmenter`. Re-running segmentation removes the old BlockMapping attribute before writing the new one.
5. **Generating table attributes**: `igQtSearchInfoWidget::refreshProperties` iterates over the `AttributeSet` and filters attributes by `IG_POINT` or `IG_CELL`. A scalar produces one column; a vector or tensor produces a magnitude column and one column per component. When `IG_BLOCK_MAPPING`, `part_id`, or `block_id` is found, cell mode prioritizes that attribute.
6. **Queries and part linkage**: the table supports `=`, `>`, and `<` numeric conditions and restricts rows to selected points or cells when a 3D selection exists. The Part Focus panel counts cells by `part_id`; checked parts are passed to `setSelectedPartIds`, which restricts candidate rows before applying the selected attribute condition.
7. **Pagination for large data**: filtered results store only point or cell IDs. The display stage reads values in pages of 100, 500, 1000, or 5000 rows and supports previous, next, and direct page navigation, avoiding creation of every `QTableWidgetItem` at once.

### How It Is Called

Online P3SAM segmentation:

```cpp
auto segmenter = iGame::P3SAMSegmenter::New();
segmenter->SetServerHost("127.0.0.1");
segmenter->SetServerPort(8765);
segmenter->SetInput(dataObj);               // current original model
segmenter->SetSimplificationRatio(0.1f);
segmenter->SetPostProcess(false);
segmenter->SetTimeout(300000);

if (segmenter->Execute()) {
    auto* partIds = dataObj->GetBlockMapping();
    auto* attrs   = dataObj->GetAttributeSet(); // now contains cell-attached part_id
}
```

When a segmented VTK already exists, read it as an `UnstructuredMesh` and map it directly:

```cpp
auto segmented = iGame::DynamicCast<iGame::UnstructuredMesh>(
    iGame::FileIO::ReadFile("segment_result.vtk"));
auto partIds = iGame::BlockMapping::GetMappingBlockCellsArray(originalMesh, segmented);
if (partIds) {
    partIds->SetName("part_id");
    dataObj->SetBlockMapping(partIds);
}
```

### GUI

| Entry | Description |
|-------|-------------|
| **Algorithms → Part Segmentation** | Configure the P3SAM service IP / port, segment the current model online, and write back `part_id` |
| **Algorithms → Part Segmentation (From File)** | Read an existing VTK containing `part_id` and map the result to the current model |
| **Algorithms → Part Focus** | List `Part <id> (<N> Cells)` entries; focus the camera / set the selection box for checked parts and link them to the table |
| **Search Data** / `action_SearchInfo` | Display all attributes in point / cell tables; when segmentation exists, enter cell mode and prioritize `part_id` |

![Part segmentation](../../Resources/Images/零件分割.png)

### Test and Validation Data

| Item | Entry / Input | Description |
|------|---------------|-------------|
| Offline mapping validation | **Part Segmentation (From File)** | Select a user-provided `UnstructuredMesh` VTK whose `CELL_DATA` contains an attribute named exactly `part_id` |
| Online integration | **Part Segmentation** | Requires an accessible P3SAM TCP service; the default address is `127.0.0.1:8765` |
| Table validation | **Search Data** | Switch to cell data, confirm that `part_id` appears as an attribute column, and query a specific part ID with `=` |

> There is currently no standalone command-line example target and no segmentation-result VTK committed with the repository. Offline validation requires a user-provided VTK containing `part_id`; online validation requires a separately running P3SAM server.

---

## Related Examples (chart analysis)

| Example Target | Description |
|----------------|-------------|
| `testParallelCoordinatesData` | Parallel coordinates |
| `testVariableCorrelationData` | Variable correlation |
| `testVariableDensityData` | Variable density |
| `testPlotLineData` | Plot-line / local-region data |
