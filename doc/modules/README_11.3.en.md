# Metric 11.3: Cloud Map / Adaptive Vector / Tensor / Deformation / Time-Series Flow / Animation Output

## Composition

For CAE simulation results, this metric provides multi-type field visualization plus time-series / animation capabilities. It comprises six sub-features (plus contour as a companion visualization):

| # | Sub-feature | Status |
|---|-------------|--------|
| 1 | Scalar cloud-map visualization | ✅ Implemented |
| 2 | Adaptive vector field (sampling: AllCell / CellInRange / EveryNth) | ✅ Implemented |
| 3 | Tensor-field visualization (ellipsoid / cuboid glyphs) | ✅ Implemented |
| 4 | Structural deformation visualization | ✅ Implemented (whole mesh; region-limited deform is TBD under 10.2) |
| 5 | Time-series flow fields and streamlines | ✅ Implemented |
| 6 | Animation playback and export (image sequence / MP4 / GIF) | ✅ Implemented (video export needs FFMPEG) |

> This document covers source paths, APIs, GUI, and examples for the above.
> Difference from **10.1**: 10.1 focuses on **analysis data generation** (entropy seeds, streamline filtering); 11.3 focuses on **field display and time playback**.
> Difference from **10.2**: 10.2 produces feature scalars / vortex predict; 11.3 visualizes them via cloud maps, time series, and deformation.
> Difference from **10.3**: 10.3 focuses on brush ↔ 3D linking; 11.3 focuses on standard field-visualization panels.

---

## Sub-feature 1: Scalar cloud maps

### Description

Map a selected `AttributeSet` attribute (and optional component `dimension`) to mesh colors. Supports continuous / interval cloud maps, color bars, and custom value ranges for scalars such as pressure, temperature, and vortex predict.

### Source Paths

| Path | Class / API | Notes |
|------|-------------|-------|
| `iGameCore/Core/DataModel/iGameDrawObject.*` | `ViewCloudPicture(Scene*, index, dimension=-1)` | Scalar coloring |
| same | `ViewCloudPictureOfModel(...)` | Bubble refresh to parent model |
| `iGameCore/Rendering/Core/iGameModel.*` | `Model::ViewCloudPicture` | Thin wrapper → DrawObject |
| `Qt/src/IQWidgets/igQtScalarViewWidget.*` | `igQtScalarViewWidget` | Cloud-map dock |

### How It Is Called

```cpp
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
drawObj->ViewCloudPicture(scene, attributeIndex);
drawObj->ViewCloudPicture(scene, attributeIndex, dimension);
```

### GUI

| Entry | Notes |
|-------|-------|
| Menu View → Scalar / `action_Scalar` | Opens left “Scalar” panel |
| `dockWidget_ScalarField` / `igQtScalarViewWidget` | Cloud type, color bar, value range |

---

## Sub-feature 2: Adaptive vector fields

### Description

Generate arrow glyphs from point / cell vector attributes. Here “adaptive” means **configurable sampling strategies** that control arrow density for readability, not mesh adaptive refinement.

| `DrawType` mode | Meaning | Main params |
|-----------------|---------|-------------|
| `AllCell` | Glyph at every point / cell center | — |
| `CellInRange` | Index range `[min, max)` only | `SetCellRange(min, max)` (default `0..100000`) |
| `EveryNth` | Keep every Nth sample | `SetNth(n)` (default `1200`) |

Arrow shape: `SetArrow(headRadius, headLength, tailRadius, tailLength)`.

### Source Paths

| Path | Class | Notes |
|------|-------|-------|
| `iGameCore/Filters/VectorView/iGameVectorBase.*` | `iGameVectorBase` | Glyphs + sampling modes |
| `Qt/src/IQWidgets/igQtVectorWidget.*` | `igQtVectorWidget` | Vector dock (mode switch) |

### How It Is Called

```cpp
iGame::iGameVectorBase vectorView;
vectorView.SetDrawMode(iGame::iGameVectorBase::EveryNth);  // or AllCell / CellInRange
vectorView.SetNth(1200);
vectorView.SetArrow(/* hR, hL, tR, tL */);
vectorView.DrawVector("Velocity", dataObj);
```

### GUI

| Entry | Notes |
|-------|-------|
| Menu View → Vector / Glyph | Opens “Vector” panel |
| `dockWidget_VectorField` | Modes: 0=All, 1=Range, 2=EveryNth |

---

## Sub-feature 3: Tensor fields

### Description

Eigen-decompose 3×3 tensors at points (stress / strain, etc.) and show principal directions/values as ellipsoid or cuboid glyphs. Optionally generate a principal eigenvector field.

| Param | API | Notes |
|-------|-----|-------|
| Glyph type | `SetGlyphType` | `ELLIPSOID` / `CUBOID` |
| Scale | `SetGlyphScale` / `UpdateGlyphScale` | Glyph size |
| Tessellation | `SetSliceNum` | Ellipsoid resolution |
| Data | `SetPoints` / `SetTensorAttributes` | 9-component / 3×3 tensors |

### Source Paths

| Path | Class | Notes |
|------|-------|-------|
| `iGameCore/Filters/TensorView/iGameTensorFilter.*` | `iGameTensorFilter` | Filter pipeline → SurfaceMesh glyphs |
| `iGameCore/Filters/TensorView/iGameTensorBase.*` | `iGameTensorBase` | GUI / DrawObject path |
| `iGameCore/Filters/TensorView/iGameTensorRepresentation.*` | `iGameTensorRepresentation` | Eigen + ELLIPSOID/CUBOID |
| `Qt/src/IQWidgets/igQtTensorWidget.*` | `igQtTensorWidget` | Tensor dock |

### How It Is Called

```cpp
auto tensorFilter = iGame::iGameTensorFilter::New();
tensorFilter->SetInput(drawObj);
tensorFilter->SetTensorAttributes(tensorArray);
tensorFilter->SetGlyphType(/* ELLIPSOID or CUBOID */);
tensorFilter->SetGlyphScale(scale);
tensorFilter->SetSliceNum(slices);
tensorFilter->Execute();
// optional: tensorFilter->GenerateVectorField();
```

GUI path may also use `iGameTensorBase::ShowTensorField()`.

### GUI

| Entry | Notes |
|-------|-------|
| Menu View → Tensor / `action_Tensor` | Opens “Tensor” panel |
| `dockWidget_TensorField` | Glyph type, scale, coloring |

> Only ellipsoid and cuboid glyphs are implemented; the representation class notes room for later glyph types.

---

## Sub-feature 4: Structural deformation

### Description

Offset **render coordinates** by a displacement vector attribute: \(p' = p + (s_x, s_y, s_z) \cdot U\). Supports uniform / non-uniform scale factors and an ideal DSF estimate \(DSF \approx K \cdot \sqrt[3]{V_{bbox}} / U_{max}\) (\(K \approx 0.15\)).

Deformation state lives on each `DataObject` via `DeformationData`. When animation play has deformation enabled, it is reapplied each frame.

### Source Paths

| Path | Class / API | Notes |
|------|-------------|-------|
| `iGameCore/Core/Common/iGameDeformationData.*` | `DeformationData` | Scales, attribute name, enable, auto DSF |
| `iGameCore/Filters/Deformation/iGameStressDeformationFilter.*` | `StressDeformationFilter` | In-place render-point offset (GUI) |
| `iGameCore/Filters/Deformation/iGameStressDeformationFilterCode.*` | `StressDeformationCodeFilter` | Variant that can emit new geometry (examples) |
| `Qt/src/IQWidgets/igQtDeformationWidget.*` | `igQtDeformationWidget` | Deformation dock |

### How It Is Called

```cpp
auto deform = dataObj->GetDeformationData();
deform->SetEnableDeformation(true);
deform->SetAttributeName("Displacement");
deform->SetScaleFactors(idealDsf);  // or SetScaleFactorX/Y/Z

auto filter = iGame::StressDeformationFilter::New();
filter->SetInput(drawObj);
filter->CalculateIdealDSF();
filter->Execute();
```

### GUI

| Entry | Notes |
|-------|-------|
| Toolbar `action_deformation` / `action_StrucDeformation` | Opens deformation panel |
| `DeformationDockWidget` (created in code, moved into left tabs) | Vector attribute, auto/uniform/non-uniform DSF, enable offset, execute |

> **Region-limited deformation** is not wired yet; current behavior is whole-mesh offset. See `README_10.2.md` sub-feature 4 for the planned selection binding.

---

## Sub-feature 5: Time-series flow fields and streamlines

### Description

Two related capabilities:

1. **Time-series switching**: move between timesteps (e.g. PVD) by swapping meshes or attribute sets.
2. **Streamline visualization**: seed and integrate on a vector field; the GUI can also use entropy seeding and streamline filtering (algorithm details in **10.1**).

The menu item “Time-series flow field” opens the **streamline / flow panel**; timeline playback is in the Animation panel (sub-feature 6).

### Time-series data model

| `StreamingType` | Meaning |
|-----------------|---------|
| `MultiSubFiles` | One sub-mesh / file per frame (typical PVD) |
| `SingleFieldAttributes` | Same mesh; swap `AttributeSet` per frame |

Key APIs: `DataObject::UpdateAnimation(keyframeIdx)`, `GetTimeFrames()`.

### Source Paths

| Path | Class | Notes |
|------|-------|-------|
| `iGameCore/Core/DataModel/iGameDataObject.*` | `UpdateAnimation` | Frame switch |
| `iGameCore/Core/Common/iGameStreamingData.*` | `StreamingData` / `TimeFrame` | Frame list + cache |
| `iGameCore/IO/VTK XML/iGamePVDReader.*` | `iGamePVDReader` | PVD → MultiSubFiles |
| `iGameCore/Filters/StreamView/iGameStreamTracer.*` | `StreamTracer` | Integration / seeding |
| `iGameCore/Filters/StreamView/iGameStreamBase.*` | `StreamBase` | Drawable streamline container |
| `iGameCore/Filters/StreamView/iGameStreamlineSimplifier.*` | `StreamlineSimplifier` | Filtering (10.1) |
| `Qt/src/IQWidgets/igQtStreamTracerWidget.*` | `igQtStreamTracerWidget` | Flow dock |

### How It Is Called

**Time switch + vector refresh:**

```cpp
drawObj->UpdateAnimation(keyframeIdx);
vectorView.DrawVector("Velocity", dataObj);
```

**Streamlines:**

```cpp
auto streamBase = iGame::StreamBase::New();
auto streamtracer = streamBase->streamFilter;
streamtracer->initStreamTracer(dataObj);
streamtracer->SetInput(seeds, vectorName, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
streamtracer->Execute();
streamBase->SetUpdate(true);
scene->AddModel(streamBase);
```

### GUI

| Entry | Notes |
|-------|-------|
| Menu View → Time-series flow / `action_FlowField` | Opens flow / streamline panel |
| `dockWidget_FlowField` | Seeding, integration, Cluster filtering |
| Toolbar Streamline | Same panel |

---

## Sub-feature 6: Animation playback and export

### Description

Play the timeline of loaded time-series data and export as an image sequence, MP4, or GIF. Each played frame may optionally reapply deformation and refresh the cloud map.

### Playback flow (GUI)

Typical Snap path in `igQtAnimationWidget`:

```text
UpdateAnimation(i)
  → ConvertToDrawableData if needed
  → if deformation enabled: StressDeformationFilter::Execute
  → refresh cloud map / scene
```

Also supports interpolate playback (VCR path) and configurable frame cache size.

### Export flow

```cpp
for (int i = 0; i < frameCount; ++i) {
    drawObj->UpdateAnimation(i);
    // optional: deformation, cloud map
    scene->Draw();
    scene->CaptureScreen(imagePath);  // or GUI grab → RGBA
}
// FFMPEGVideoWriter::SaveMP4() / SaveGIF()
```

| Export form | Condition |
|-------------|-----------|
| PNG / screenshot sequence | Always available |
| MP4 / GIF | Requires `FFMPEG_FOUND` at build time (`FFMPEG_ENABLE`) |

### Source Paths

| Path | Class | Notes |
|------|-------|-------|
| `Qt/src/IQWidgets/igQtAnimationWidget.*` | `igQtAnimationWidget` | Play / export dock |
| `Qt/src/IQWidgets/igQtAnimationVcrController.*` | VCR controller | Playback control |
| `iGameCore/IO/FFMPEG/iGameFFMPEGVideoWriter.*` | `FFMPEGVideoWriter` | `SaveMP4` / `SaveGIF` |
| `Examples/Animation/SaveAnimation.cpp` | `testSaveAnimation` | Export example |

### GUI

| Entry | Notes |
|-------|-------|
| Menu View → Animation export / `action_ExportAnimation` | Opens bottom animation dock |
| `dockWidget_Animation` | Play, cache, export |
| Toolbar `action_SaveAnimation` | Calls `saveAnimation()` |

---

## Companion: Contour / isolines

Not one of the six title bullets, but part of field visualization output and already wired in GUI/examples.

| Path | Notes |
|------|-------|
| `iGameCore/Filters/Contour/iGameContourFilter.*` | Iso-surface / iso-line |
| `Qt/src/IQWidgets/igQtContourExtractWidget.*` | `dockWidget_ContourExtract` |
| Example `testContourLine` | Isolines |

---

## Meshlet acceleration (optional)

Large-mesh cloud maps / drawing can use Meshlet acceleration:

```cpp
drawObj->SetAccelerationOption(/* ... */);
```

See `iGameDrawObject::SetAccelerationOption`; more detail in metric **11.4**.

---

## Related Examples (summary)

| Target | Notes | Condition |
|--------|-------|-----------|
| `testSetScalarField` | Cloud map | default |
| `testVector` / `testVectorAllCell` / `testVectorCellInRange` / `testVectorEveryNth` / `testVectorSubData` | Vector sampling modes | default |
| `testTensorView` | Tensor glyphs | default |
| `testDeformation` | Deformation (minimal) | default |
| `testDeformationCode` | Explicit DSF + Execute | default |
| `testTimeVaryingVector` | Time series + vectors | default |
| `testStreamline` | Streamlines | default |
| `testContourLine` | Isolines | default |
| `testAnimation` | Animation play setup | default |
| `testSaveAnimation` | Animation export | `FFMPEG_FOUND` |

---

## Acceptance Checklist

| Sub-feature | Suggested check |
|-------------|-----------------|
| Cloud map | Scalar attribute colors and color bar look correct |
| Vector field | Three sampling modes produce expected arrow density |
| Tensor field | Ellipsoid / cuboid glyphs follow principal values |
| Deformation | Geometry moves with displacement when enabled; persists across animation frames |
| Time series / streamlines | PVD frames switch; streamlines integrate visibly |
| Animation export | Playback is smooth; MP4/GIF export works when FFMPEG is built |
