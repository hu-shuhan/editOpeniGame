# Metric 10.3: Physics Field Feature Visual Interaction Module

## Purpose

Perception-driven, large-scale multi-scale physics field feature interaction: brush/range selection in 2D analytic views (parallel coordinates, correlation, density, plot-line) drives 3D model highlighting via `Selection` callbacks.

> vs **10.1**: 10.3 = **linked interaction**; 10.1 = analysis data generation.  
> vs **11.3**: 11.3 = field rendering; 10.3 = multi-variate charts ↔ 3D selection.

## Source Paths

`ParallelCoordinates/`, `VariableCorrelation/`, `VariableDensity/`, `PlotLine/`, `Core/Common/iGameCtxPresObjData.*`, `Filters/Selection/`, `Examples/MultiscaleInteraction/`.

## How It Is Called

```cpp
data->SetDefaultSelectionFunc(name, mesh->GetSelection());
auto pointIds = data->FiltInRangeIds(...);
SelectPoints(mesh, pointIds);
```

### GUI

`igQtParallelCoordinatesWidget`, `igQtVariableCorrelationWidget`, `igQtVariableDensityWidget`, `igQtDataChangeWidget`.

## Related Examples

**`testMultiscaleInteraction`** (recommended acceptance demo), plus `testParallelCoordinatesData`, `testVariableCorrelationData`, `testVariableDensityData`, `testPlotLineData`, `testSetSelectionCallBack`.
