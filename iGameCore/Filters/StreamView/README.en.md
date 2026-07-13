# Metric 10.3: Physics Field Feature Visual Interaction Module

## Purpose

Provides perception-driven, large-scale multi-scale physics field feature visual interaction. Supports streamline tracing, adaptive vector/tensor field rendering, structural deformation display, and isosurface extraction, with linked selection and multi-variable analysis for interactive exploration of physics field features.

Key capabilities:

- Streamline generation and tracing (seed configuration, streamline simplification)
- Adaptive vector field rendering (all cells / in-range / every Nth sampling)
- Tensor field ellipsoid/cuboid rendering
- Structural stress deformation computation and display
- Isosurface/isoline extraction

## Core Implementation in This Directory

| File | Class | Description |
|------|-------|-------------|
| `iGameStreamBase.h/.cpp` | `StreamBase` | Streamline generation base class with seed and tracing parameters |
| `iGameStreamTracer.h/.cpp` | `StreamTracer` | Streamline tracing algorithm |
| `iGameStreamlineSimplifier.h/.cpp` | `StreamlineSimplifier` | Streamline simplification |

## Related Source Paths

- Vector field rendering: [`../VectorView/iGameVectorBase.h`](../VectorView/iGameVectorBase.h) (`DrawVector`, `DrawType::AllCell | CellInRange | EveryNth`)
- Tensor field rendering: [`../TensorView/iGameTensorFilter.h`](../TensorView/iGameTensorFilter.h)
- Structural deformation: [`../Deformation/iGameStressDeformationFilter.h`](../Deformation/iGameStressDeformationFilter.h)
- Isosurface extraction: [`../Contour/iGameContourFilter.h`](../Contour/iGameContourFilter.h)
- Qt widgets: [`../../../Qt/src/IQWidgets/igQtStreamTracerWidget.cpp`](../../../Qt/src/IQWidgets/igQtStreamTracerWidget.cpp), `igQtVectorWidget`, `igQtTensorWidget`, `igQtDeformationWidget`

## How It Is Called

### API: Streamline Tracing

```cpp
auto streamBase = iGame::StreamBase::New();
auto streamtracer = streamBase->streamFilter;
streamtracer->initStreamTracer(dataObj);
// configure seeds and tracing parameters
streamtracer->SetInput(seeds, vectorName, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
streamtracer->Execute();
streamBase->SetUpdate(true);
scene->AddModel(streamBase);
```

### API: Adaptive Vector Field

```cpp
iGame::iGameVectorBase vectorView;
vectorView.SetDrawMode(iGame::iGameVectorBase::AllCell);   // or CellInRange / EveryNth
vectorView.DrawVector("Velocity", dataObj);
```

### API: Structural Deformation

```cpp
auto deformFilter = iGame::StressDeformationFilter::New();
deformFilter->SetInput(drawObj);
deformFilter->Execute();
```

### GUI Invocation

Qt main window Dock panels:

- `dockWidget_FlowField` → `igQtStreamTracerWidget` (streamlines)
- `dockWidget_VectorField` → `igQtVectorWidget` (vector field)
- `dockWidget_TensorField` → `igQtTensorWidget` (tensor field)
- Deformation panel → `igQtDeformationWidget` (structural deformation)
- `dockWidget_ContourExtract` → `igQtContourExtractWidget` (isosurface)

## Related Examples

| Example Target | Description |
|----------------|-------------|
| `testStreamline` | Streamline tracing |
| `testVector` / `testVectorAllCell` / `testVectorCellInRange` / `testVectorEveryNth` | Adaptive vector field |
| `testTensorView` | Tensor field visualization |
| `testDeformation` | Structural deformation |
| `testContourLine` | Isosurface extraction |
| `testMultiscaleInteraction` | Multiscale linked interaction |