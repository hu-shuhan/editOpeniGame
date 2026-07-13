# Metric 11.2: VTK/CGNS Data Interface Module

## Purpose

Provides VTK and CGNS format import for CAE simulation data. Uses a unified `FileIO` entry point that automatically dispatches to the appropriate reader based on file extension, converting external simulation results into iGameCore `DataObject` data structures for subsequent visualization and analysis.

Key capabilities:

- Legacy VTK format reading (`.vtk`)
- VTK XML format reading (`.vtu`, `.vts`, `.vtp`, `.vtm`, `.pvd`)
- CGNS format reading (`.cgns`)
- VTK format writing
- Unified file type detection and read dispatch

## Core Implementation in This Directory

| File | Class | Description |
|------|-------|-------------|
| `iGameFileIO.h/.cpp` | `FileIO` | Unified read/write entry, `ReadFile()` dispatches by extension |
| `iGameFileReader.h/.cpp` | `FileReader` | Reader base class |

## Related Source Paths

- Legacy VTK: [`VTK/iGameVTKReader.h`](VTK/iGameVTKReader.h), [`iGameVTKWriter.h`](VTK/iGameVTKWriter.h)
- VTK XML: [`VTK XML/iGameVTUReader.h`](VTK%20XML/iGameVTUReader.h), [`iGameVTSReader.h`](VTK%20XML/iGameVTSReader.h), [`iGameVTPReader.h`](VTK%20XML/iGameVTPReader.h), [`iGameVTMReader.h`](VTK%20XML/iGameVTMReader.h), [`iGamePVDReader.h`](VTK%20XML/iGamePVDReader.h)
- CGNS: [`CGNS/iGameCGNSReader.h`](CGNS/iGameCGNSReader.h)

## How It Is Called

### API: Unified Reading (Recommended)

```cpp
iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(filePath);
```

`FileIO::ReadFile()` automatically selects the reader based on file extension. Supported formats include `.vtk`, `.vtu`, `.vts`, `.vtp`, `.vtm`, `.pvd`, `.cgns`, etc.

### API: Direct CGNS Reading

```cpp
auto reader = iGame::iGameCGNSReader::New();
auto obj = reader->ReadFile(filePath);
```

### GUI Invocation

```
igQtFileLoader::OpenFile(filePath)
  → iGame::FileIO::ReadFile(filePath)
  → Scene::AddModel(obj)
```

Triggered via Qt main window File → Open or drag-and-drop, with `igQtFileLoader` calling `FileIO::ReadFile()` to complete import.

## Related Examples

| Example Target | Description |
|----------------|-------------|
| `testCGNS` | CGNS file reading |