# Metric 10.1: Intelligent Visual Analysis Module

## Purpose

Provides intelligent visual analytics for CAE simulation data through multi-variable parallel coordinates, variable correlation analysis, variable density distribution, and plot-line probing. Helps users quickly discover relationships and distribution patterns across multi-dimensional physical field data, with support for linked interactive selection.

Key capabilities:

- Multi-variable parallel coordinates data generation and visualization
- Variable correlation matrix analysis
- Variable density distribution statistics
- Plot-line data extraction
- Linked interaction with selection callbacks

## Core Implementation in This Directory

| File | Class | Description |
|------|-------|-------------|
| `iGameGenerateParallelCoordinatesData.h/.cpp` | `GenerateParallelCoordinatesDataFilter` | Generates parallel coordinates analysis data from mesh attributes |

## Related Source Paths

- Variable correlation: [`../VariableCorrelation/iGameGenerateVariableCorrelationDataFilter.h`](../VariableCorrelation/iGameGenerateVariableCorrelationDataFilter.h)
- Variable density: [`../VariableDensity/iGameGenerateVariableDensityDataFilter.h`](../VariableDensity/iGameGenerateVariableDensityDataFilter.h)
- Plot line: [`../PlotLine/iGameGeneratePlotLineDataFilter.h`](../PlotLine/iGameGeneratePlotLineDataFilter.h)
- Interactive selection: [`../Selection/`](../Selection/) (`SetSelectionCallBackFuncFilter`, etc.)
- Analysis data types: [`../../Core/Common/iGameParallelCoordinatesData.h`](../../Core/Common/iGameParallelCoordinatesData.h), etc.
- Qt widgets: [`../../../Qt/src/IQWidgets/igQtParallelCoordinatesWidget.cpp`](../../../Qt/src/IQWidgets/igQtParallelCoordinatesWidget.cpp)

## How It Is Called

### API: Generate Parallel Coordinates Data

```cpp
auto filter = iGame::GenerateParallelCoordinatesDataFilter::New(IG_POINT);
filter->SetInput(0, mesh);
filter->Execute();
auto data = iGame::DynamicCast<iGame::ParallelCoordinatesData>(filter->GetOutput(0));
```

### API: Variable Correlation Analysis

```cpp
auto filter = iGame::GenerateVariableCorrelationDataFilter::New(IG_POINT);
filter->SetInput(0, mesh);
filter->Execute();
```

### GUI Invocation

The Qt main window loads corresponding Dock panels:

- `dockWidget_ParallelCoordinatesField` → `igQtParallelCoordinatesWidget`
- `dockWidget_VariableCorrelationField` → `igQtVariableCorrelationWidget`
- `dockWidget_VariableDensityField` → `igQtVariableDensityWidget`

Selection operations link to visualization panels via callback functions in the `Selection` filter family.

## Related Examples

| Example Target | Description |
|----------------|-------------|
| `testParallelCoordinatesData` | Parallel coordinates data generation |
| `testVariableCorrelationData` | Variable correlation analysis |
| `testVariableDensityData` | Variable density distribution |
| `testPlotLineData` | Plot-line data extraction |
| `testMultiscaleInteraction` | Multiscale linked interaction analysis |