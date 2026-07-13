# Metric 11.4: High-Precision Parallel Visualization Software for CAE Simulation Results

## Purpose

`iGameCore` is the core library of iGameVis, providing high-precision parallel visualization for CAE simulation results. It integrates data import (IO), algorithm processing (Filters), data models (Core/DataModel), and the rendering pipeline (Rendering) into a unified visualization platform that supports efficient rendering and interactive analysis of large-scale meshes.

Key capabilities:

- Unified data object model (`DataObject` / `DrawObject`) with attribute management
- Multi-threaded parallel computation via `ThreadPool::parallelFor`
- OpenGL scene rendering with Meshlet GPU acceleration
- Full integration with the Qt frontend and Examples programs

## Directory Structure

| Subdirectory | Responsibility |
|--------------|----------------|
| `Core/` | Base objects, cell models, mesh data structures |
| `Filters/` | Feature extraction, flow/vector/tensor, deformation, visual analytics |
| `IO/` | VTK/CGNS/Spline and other format readers/writers |
| `Rendering/` | Scene, OpenGL rendering, Meshlet acceleration, interactors |

## Related Source Paths

- Thread pool: [`Core/Common/iGameThreadPool.h`](Core/Common/iGameThreadPool.h)
- Meshlet acceleration: [`Rendering/Core/Meshleter/`](Rendering/Core/Meshleter/)
- Scene rendering: [`Rendering/Core/iGameScene.cpp`](Rendering/Core/iGameScene.cpp)

## How It Is Called

### GUI Application Path

```
main.cpp → igQtMainWindow → igQtFileLoader::OpenFile()
         → FileIO::ReadFile() → Scene::AddModel()
         → Qt Dock widgets invoke Filters / DrawObject
```

### API: Enable Meshlet Parallel Acceleration

```cpp
auto dataObj = iGame::FileIO::ReadFile(fileName);
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
drawObj->SetAccelerationOption(true);
scene->AddModel(dataObj);
```

### API: Multi-threaded Parallel Processing

```cpp
iGame::ThreadPool::parallelFor(0, count, [&](int i) {
    // parallel work
});
```

## Related Examples

| Example Target | Description |
|----------------|-------------|
| `testMeshletRendering` | Meshlet GPU-accelerated rendering |
| `testSetRenderingPressure` | Adaptive rendering pressure and mesh simplification |

Build examples with `EXAMPLE_COMPILE=ON`.