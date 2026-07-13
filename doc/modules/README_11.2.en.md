# Metric 11.2: VTK/CGNS Data Interface Module

## Purpose

VTK and CGNS import (and VTK export) via unified `FileIO::ReadFile()` dispatch.

## Source Paths

`iGameCore/IO/iGameFileIO.*`, `IO/VTK/`, `IO/VTK XML/`, `IO/CGNS/iGameCGNSReader.*` (requires `ENABLE_CGNS_MODULE=ON`).

## How It Is Called

```cpp
auto obj = iGame::FileIO::ReadFile(filePath);
```

### GUI

`igQtFileLoader::OpenFile()` → `FileIO::ReadFile()` → `Scene::AddModel()`.

## Related Examples

`testCGNS` when `ENABLE_CGNS_MODULE=ON`.

Other formats (Nastran, Fluent, Abaqus) live under other `iGameCore/IO/` subdirectories.
