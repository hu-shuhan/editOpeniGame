# Metric 11.3: Field Visualization Output Module

## Purpose

Scalar cloud maps, adaptive vector fields, tensor fields, structural deformation, streamlines/time-series flow fields, and animation export.

## Source Paths

- Core: `iGameDrawObject::ViewCloudPicture()`, `DataObject::UpdateAnimation()`, `SetAccelerationOption()`
- Filters: `VectorView/`, `TensorView/`, `Deformation/`, `StreamView/`, `Contour/`
- IO: `PVDReader`, `FFMPEGVideoWriter` (optional)

## How It Is Called

```cpp
drawObj->ViewCloudPicture(scene, attributeIndex);
drawObj->UpdateAnimation(keyframeIdx);
```

Streamlines via `StreamBase` + `StreamTracer`; vectors via `iGameVectorBase::DrawVector()`.

### GUI

`dockWidget_ScalarField`, `dockWidget_VectorField`, `dockWidget_TensorField`, `dockWidget_FlowField`, `dockWidget_Animation`, `DeformationDockWidget`, `dockWidget_ContourExtract`.

## Related Examples

`testSetScalarField`, `testTimeVaryingVector`, `testStreamline`, `testVector*`, `testTensorView`, `testDeformation`, `testContourLine`, `testAnimation`, `testSaveAnimation` (requires `FFMPEG_FOUND`).
