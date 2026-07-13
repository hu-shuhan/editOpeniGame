# Metric 10.1: Intelligent Visual Analysis Module

## Purpose

Statistical visual analytics for multi-dimensional CAE field data: parallel coordinates, variable correlation, density distribution, and plot-line probing.

> Difference from **10.3**: 10.1 focuses on **analysis data generation**; 10.3 focuses on **brush/selection interaction linked to the 3D model**.

## Source Paths

| Path | Description |
|------|-------------|
| `iGameCore/Filters/ParallelCoordinates/iGameGenerateParallelCoordinatesData.*` | Parallel coordinates |
| `iGameCore/Filters/VariableCorrelation/iGameGenerateVariableCorrelationDataFilter.*` | Correlation matrix |
| `iGameCore/Filters/VariableDensity/iGameGenerateVariableDensityDataFilter.*` | Density distribution |
| `iGameCore/Filters/PlotLine/iGameGeneratePlotLineDataFilter.*` | Plot-line data |
| `iGameCore/Core/Common/iGameParallelCoordinatesData.h` etc. | Analysis data objects |

## How It Is Called

### Parallel coordinates

```cpp
auto filter = iGame::GenerateParallelCoordinatesDataFilter::New(IG_POINT);
filter->SetInput(0, mesh);
filter->Execute();
auto data = iGame::DynamicCast<iGame::ParallelCoordinatesData>(filter->GetOutput(0));
```

### Variable correlation / density

```cpp
auto corr = iGame::GenerateVariableCorrelationDataFilter::New(IG_POINT);
corr->SetInput(0, mesh);
corr->Execute();
```

### GUI Dock panels

`igQtParallelCoordinatesWidget`, `igQtVariableCorrelationWidget`, `igQtVariableDensityWidget`, `igQtDataChangeWidget`.

## Related Examples

`testParallelCoordinatesData`, `testVariableCorrelationData`, `testVariableDensityData`, `testPlotLineData`.

## Optional

LibTorch vortex detection belongs to **10.2** (`ENABLE_LIBTORCH_MODULE=ON`).
