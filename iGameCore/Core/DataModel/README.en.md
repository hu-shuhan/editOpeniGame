# Metric 11.3: Cloud Map / Adaptive Vector Field / Tensor Field / Structural Deformation / Time-Series Flow Field / Animation Output Visualization Module

## Purpose

Provides multi-type visualization output for CAE simulation results, including scalar cloud maps, adaptive vector fields, tensor fields, structural deformation, time-series flow field switching, and animation video export. Serves as the core visualization layer connecting data models to the rendering pipeline.

Key capabilities:

- Scalar cloud map (color map) switching and rendering
- Time-series flow field frame switching (`UpdateAnimation`)
- Rendering acceleration option control
- Data model binding with vector/tensor/deformation filters

## Core Implementation in This Directory

| File | Class | Key Method | Description |
|------|-------|------------|-------------|
| `iGameDrawObject.h/.cpp` | `DrawObject` | `ViewCloudPicture()` | Scalar cloud map rendering |
| `iGameDataObject.h/.cpp` | `DataObject` | `UpdateAnimation()` | Time-series frame switching |
| `iGameDrawObject.h/.cpp` | `DrawObject` | `SetAccelerationOption()` | Rendering acceleration control |

## Related Source Paths

- Adaptive vector field: [`../../Filters/VectorView/iGameVectorBase.h`](../../Filters/VectorView/iGameVectorBase.h)
- Tensor field: [`../../Filters/TensorView/iGameTensorFilter.h`](../../Filters/TensorView/iGameTensorFilter.h)
- Structural deformation: [`../../Filters/Deformation/iGameStressDeformationFilter.h`](../../Filters/Deformation/iGameStressDeformationFilter.h), [`../../Core/Common/iGameDeformationData.h`](../../Core/Common/iGameDeformationData.h)
- Time-series data reading: [`../../IO/VTK XML/iGamePVDReader.h`](../../IO/VTK%20XML/iGamePVDReader.h)
- Animation video export: [`../../IO/FFMPEG/iGameFFMPEGVideoWriter.h`](../../IO/FFMPEG/iGameFFMPEGVideoWriter.h)
- Qt widgets: [`../../../Qt/src/IQWidgets/igQtScalarViewWidget.cpp`](../../../Qt/src/IQWidgets/igQtScalarViewWidget.cpp), `igQtAnimationWidget`

## How It Is Called

### API: Scalar Cloud Map

```cpp
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
drawObj->ViewCloudPicture(scene, attributeIndex);
```

### API: Time-Series Flow Field Switching

```cpp
// After loading time-series data via PVD
drawObj->GetTimeFrames()->UpdateAnimation(frameIndex);
vectorView.DrawVector("Velocity", dataObj);
```

### API: Structural Deformation + Cloud Map

```cpp
drawObj->GetTimeFrames()->UpdateAnimation(keyframeIdx);
if (obj->GetDeformationData()->GetEnableStatus()) {
    auto deformFilter = iGame::StressDeformationFilter::New();
    deformFilter->SetInput(drawObj);
    deformFilter->Execute();
}
drawObj->ViewCloudPicture(scene, drawObj->GetAttributeIndex());
```

### API: Animation Export

```cpp
// Update frame → capture screen → write video
for (int i = 0; i < frameCount; ++i) {
    drawObj->UpdateAnimation(i);
    scene->Draw();
    scene->CaptureScreen(imagePath);
}
ffmpegWriter->SaveMP4(outputPath);
```

### GUI Invocation

| Dock Panel | Function |
|------------|----------|
| `dockWidget_ScalarField` | Scalar cloud map |
| `dockWidget_VectorField` | Adaptive vector field |
| `dockWidget_TensorField` | Tensor field |
| Deformation panel | Structural deformation |
| `dockWidget_Animation` | Time-series animation playback and export |

## Related Examples

| Example Target | Description |
|----------------|-------------|
| `testSetScalarField` | Scalar cloud map |
| `testTimeVaryingVector` | Time-series flow field |
| `testTensorView` | Tensor field |
| `testDeformation` | Structural deformation |
| `testAnimation` | Animation playback |
| `testSaveAnimation` | Animation video export |