# Metric 11.4: High-Precision Parallel CAE Visualization Platform

## Purpose

`iGameCore` integrates IO, Filters, DataModel, and Rendering into a unified high-precision parallel visualization platform for CAE results.

## Source Paths

`iGameCore/Core/`, `Filters/`, `IO/`, `Rendering/`, plus `Qt/` frontend.

Key: `iGameThreadPool.h`, `Rendering/Core/Meshleter/`, `iGameScene.cpp`.

## How It Is Called

GUI: `main.cpp` → `igQtMainWindow` → `FileIO::ReadFile()` → `Scene::AddModel()`.

API: `drawObj->SetAccelerationOption(true)`, `ThreadPool::parallelFor(...)`.

## Related Examples

`testMeshletRendering`, `testSetRenderingPressure` (`EXAMPLE_COMPILE=ON`).

See [doc/modules/README.md](README.md) for metrics 7.1, 10.x, 11.x.
