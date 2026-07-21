# Metric 10.1: Intelligent Visual Analysis for CAE Simulation Data

## Composition

For multi-dimensional CAE physical-field data, this metric provides interaction-driven, data-feature-guided intelligent visual analysis. It comprises four sub-features:

| # | Sub-feature | Status |
|---|-------------|--------|
| 1 | Point-driven local-region chart analysis: pick a point, auto-determine a bounding box from the current attribute, and run chart analysis on that local region | ✅ Implemented |
| 2 | Entropy-based automatic streamline seed placement for a region | ✅ Implemented |
| 3 | Intelligent streamline filtering | ✅ Implemented |
| 4 | Semantic-segmentation-based table attributes | ⏳ Planned |

> This document covers the completed sub-features **1**, **2**, **3**. Sub-feature 4 will be added once implemented.
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

## Related Examples (chart analysis)

| Example Target | Description |
|----------------|-------------|
| `testParallelCoordinatesData` | Parallel coordinates |
| `testVariableCorrelationData` | Variable correlation |
| `testVariableDensityData` | Variable density |
| `testPlotLineData` | Plot-line / local-region data |
