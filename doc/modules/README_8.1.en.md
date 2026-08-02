# Metric 8.1: Lightweight Visualization and High-Performance Rendering for CAE Data

## Subfunctions

| # | Subfunction | Status |
|---|-------------|--------|
| 1 | Meshlet-based GPU accelerated rendering | Implemented for triangular surface meshes |
| 2 | Quadric-error-metric mesh simplification | Implemented for triangular surface meshes |

## Meshlet Rendering

`SurfaceMeshMeshleter` uses meshoptimizer to build Meshlets for triangular surface meshes, uploads Meshlet data to OpenGL buffers, and uses `MeshletCull.comp` for visibility culling. A Meshlet is currently limited to 64 vertices and 124 triangles.

| Path | Description |
|------|-------------|
| `iGameCore/Rendering/Core/Meshleter/` | Meshlet data structures and GPU synchronization |
| `iGameCore/Rendering/Shaders/GLSL/MeshletCull.comp` | Meshlet culling shader |
| `Examples/Rendering/MeshletRendering.cpp` | `testMeshletRendering` |

```cpp
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
drawObj->SetAccelerationOption(true);
scene->AddModel(dataObj);
```

### GUI

| Entry | Action | Description |
|-------|--------|-------------|
| Right-click a selected object in the model tree | Build rendering acceleration structure | Builds the Meshlet rendering acceleration structure for the selected object. |
| Right-click a selected object in the model tree | Disable rendering acceleration structure | Disables the accelerated rendering path for the selected object. |
| Right-click a selected object in the model tree | Toggle Meshlet visualization | Switches the Meshlet visualization state for inspecting Meshlet partitioning. |

### GUI Screenshot

![Meshlet rendering GUI](../../Resources/Images/Meshlet加速.png)

Figure 1: Meshlet rendering acceleration and visualization actions in the model-tree context menu.

## Mesh Simplification

`MeshSimplificationFilter` reduces triangular surface meshes with configurable target reduction and boundary preservation.

| Path | Description |
|------|-------------|
| `iGameCore/Filters/DataProcessing/iGameMeshSimplificationFilter.*` | Simplification filter |
| `iGameCore/Filters/DataProcessing/Simplification/` | Simplification support algorithms |
| `Examples/Filter/Compression/TestSimplification.cpp` | `testSimplification` |

```cpp
auto filter = iGame::MeshSimplificationFilter::New();
filter->SetTargetReduction(0.5);
filter->SetPreserveBoundary(true);
filter->SetInput(surfaceMesh);
filter->Execute();
```

### GUI

| Entry | Action | Description |
|-------|--------|-------------|
| `Mesh Processing` menu | `Surface Simplification` | Opens the surface simplification dialog; set the reduction ratio from 0 to 1 and run the standard surface simplification filter. |
| `Mesh Processing` menu | `Fast Surface Simplification` | Opens the fast simplification dialog; set the target reduction ratio from 0 to 1 and run the fast surface simplification filter. |

Only triangular surface meshes support these simplification operations.

### GUI Screenshot

| Before simplification | After simplification |
|-----------------------|----------------------|
| ![Surface mesh before simplification](../../Resources/Images/before_Simplification.png) | ![Surface mesh after simplification](../../Resources/Images/simplification.png) |

Figure 2: Comparison before and after surface mesh simplification.

## Test Targets and Assessment

| Target | Input | Purpose |
|--------|-------|---------|
| `testMeshletRendering` | `Tet_Plane.vtk` | Meshlet build and rendering |
| `testSimplification` | `mazewheel.obj` | Surface mesh simplification |

Third-party assessment should record data scale, client/server hardware, network, import and transfer time, decoding, first-frame time, interactive frame rate, and peak memory. The current repository does not contain a server-side scheduler or a third-party report that proves the billion-scale C/S requirement.
