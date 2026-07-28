# Metric 11.2: VTK / CGNS Data Interface Module

## Metric Components

This module provides unified interfaces for converting VTK / CGNS files into the iGameVis data model. It is designed for CAE simulation meshes, physical-field attributes, multiblock data, and time-series data, and consists of four sub-features:

| # | Sub-feature | Status |
|---|-------------|--------|
| 1 | Unified file-type detection and IO dispatch | ✅ Implemented |
| 2 | VTK Legacy / VTK XML data import | ✅ Implemented |
| 3 | VTK data export | ✅ Legacy VTK ASCII implemented; VTM routing integrated; other XML writers are not connected to the unified entry point |
| 4 | CGNS data import | ✅ Implemented as an optional module |

> This document records the source paths, APIs, GUI entry points, and examples for the sub-features above.
> This metric focuses on VTK / CGNS. Interfaces for OBJ, STL, PLY, MESH, IGC, Nastran, Fluent CAS, Abaqus ODB, and other formats are located in other subdirectories under `iGameCore/IO/`.

---

## Sub-feature 1: Unified File-Type Detection and IO Dispatch

### Description

`FileIO` detects the data format from the file extension and creates the corresponding reader or writer. After a file has been read successfully, it assigns a common object name and attempts to merge consecutive scalar components named `X/Y/Z`, `0/1/2`, or `1/2/3` into three-dimensional vector attributes that can be used directly by subsequent field-visualization operations.

Typical reading flow:

```text
FileIO::ReadFile(filePath)
  → GetFileType(filePath)
  → Create a concrete reader
  → Reader::SetFilePath() / Execute() / GetOutput()
  → DataObject / DrawObject
  → Organize attributes and name the object
```

### VTK / CGNS Dispatch Coverage

| Extension | `FileType` | Read | `FileIO::WriteFile` | Description |
|-----------|------------|------|---------------------|-------------|
| `.vtk` | `VTK` | ✅ | ✅ | VTK Legacy |
| `.vtu` | `VTU` | ✅ | ❌ | VTK XML UnstructuredGrid |
| `.vts` | `VTS` | ✅ | ❌ | VTK XML StructuredGrid |
| `.vtp` | `VTP` | ✅ | ❌ | VTK XML PolyData |
| `.vtm` | `VTM` | ✅ | ⚠️ Integrated | VTK XML MultiBlock; child-block output must be verified for the current object type |
| `.pvd` | `PVD` | ✅ | ❌ | VTK time-series collection |
| `.cgns` | `CGNS` | Conditionally enabled | ❌ | Requires `ENABLE_CGNS_MODULE=ON` |

> “Read support” means that the format is connected to `FileIO::ReadFile()`. “Write support” means that the format is connected to `FileIO::WriteFile()`. The existence of `VTUWriter` or `VTSWriter` in the source tree does not mean that the unified write entry point already supports `.vtu` or `.vts`.

### Source Paths

| Path | Class / API | Description |
|------|-------------|-------------|
| `iGameCore/IO/iGameFileIO.h` | `FileIO` | Unified read/write interface and `FileType` enumeration |
| `iGameCore/IO/iGameFileIO.cpp` | `GetFileType`, `ReadFile`, `WriteFile` | Extension detection and reader/writer dispatch |
| `iGameCore/IO/iGameFileReader.*` | `FileReader` | Reader base class and file / memory-buffer management |
| `iGameCore/IO/iGameFileWriter.*` | `FileWriter` | Writer base class, buffer generation, and file output |
| `iGameCore/Core/DataModel/iGameDataObject.*` | `DataObject` | Unified data object produced by IO operations |
| `iGameCore/Core/Common/iGameAttributeSet.*` | `AttributeSet` | Point- and cell-associated attribute collections |

### Usage

**Unified reading interface (recommended):**

```cpp
#include "iGameFileIO.h"

const std::string filePath = "./Models/Tet_Plane.vtk";
auto obj = iGame::FileIO::ReadFile(filePath);

if (obj == nullptr) {
    // Unsupported extension, missing file, or parsing failure
    return;
}
```

**Detecting only the file type:**

```cpp
#include "iGameFileIO.h"

auto type = iGame::FileIO::GetFileType("result.vtu");
auto name = iGame::FileIO::GetFileTypeAsString(type); // "VTU"
```

Extension detection is case-insensitive. A path with no extension or with an unknown extension returns `FileIO::NONE`.

### GUI

```text
Main window: Open File
  → igQtFileLoader::LoadFile()
  → igQtFileLoader::OpenFile(filePath)
  → FileIO::ReadFile(filePath)
  → emit NewModel(obj, ItemSource::File)
  → Scene::AddModel(obj)
```

| Entry point | Description |
|-------------|-------------|
| File menu / toolbar: Open | Opens a file-selection dialog with multiple selection enabled |
| Recent files | Calls `igQtFileLoader::OpenFile()` again |
| `Qt/src/IQCore/igQtFileLoader.cpp` | File filters, unified reading, and model creation |
| `Qt/include/IQCore/igQtFileLoader.h` | Declarations of `OpenFile` / `OpenFiles` |

> The current GUI “All Files” filter lists `.vtk`, `.vtu`, `.vts`, `.vtm`, `.pvd`, and `.cgns`, but does not yet list `.vtp`. A `.vtp` file can still be read through the `FileIO::ReadFile()` API.

### Test Cases

Unified dispatch does not have an independent example target; it is covered indirectly by examples for individual formats. The basic VTK path can be tested with `testSetRenderWindow` or `testSetScalarField`, while CGNS can be tested with `testCGNS`.

---

## Sub-feature 2: VTK Legacy / VTK XML Data Import

### Description

VTK import is divided into the Legacy VTK and VTK XML paths. Both paths ultimately produce a `SurfaceMesh`, `StructuredMesh`, `UnstructuredMesh`, or a `DrawObject` containing child objects or time frames.

| Format | Reader | Output / Purpose |
|--------|--------|------------------|
| `.vtk` | `VTKReader` | Legacy VTK meshes and attributes |
| `.vtu` | `iGameVTUReader` | Unstructured meshes |
| `.vts` | `iGameVTSReader` | Structured meshes |
| `.vtp` | `iGameVTPReader` | PolyData points, lines, and surface cells |
| `.vtm` | `iGameVTMReader` | Multiblock data and referenced child files |
| `.pvd` | `iGamePVDReader` | Multi-timestep, multi-file data |

### Legacy VTK

`VTKReader` supports ASCII and binary headers and creates the corresponding mesh according to the `DATASET` type:

| VTK `DATASET` | iGameVis Output |
|---------------|------------------|
| `UNSTRUCTURED_GRID` | `UnstructuredMesh` |
| `POLYDATA` | `SurfaceMesh` |
| `STRUCTURED_POINTS` | `StructuredMesh` |
| `STRUCTURED_GRID` | `StructuredMesh` |

Point and cell attributes are stored in `AttributeSet`. Parsing paths include scalar, vector, tensor, normal, and Field data.

### VTK XML

- `.vtu` / `.vts` readers load points, cell connectivity, PointData, and CellData.
- `.vtp` supports PolyData Verts, Lines, Polys, and Strips, and handles ASCII, inline binary, and appended data.
- `.vtm` parses `<Block>` or flat `<DataSet>` elements. Referenced `.vtk`, `.vtu`, `.vts`, and conditionally enabled `.cgns` files are loaded as child objects.
- `.pvd` organizes `StreamingData` by `timestep`. Child files for the first frame are loaded in parallel; later frames are selected through `DataObject::UpdateAnimation()`.

> VTM reference paths are resolved relative to the directory containing the VTM file. The current reader uses `/` to locate the parent directory, so relative references should be tested carefully when the supplied Windows path contains backslashes.

### Source Paths

| Path | Class / API | Description |
|------|-------------|-------------|
| `iGameCore/IO/VTK/iGameVTKAbstractReader.*` | `VTKAbstractReader` | Shared Legacy VTK parsing logic |
| `iGameCore/IO/VTK/iGameVTKReader.*` | `VTKReader` | Legacy VTK entry point and data-object creation |
| `iGameCore/IO/XML/iGameXMLFileReader.*` | `iGameXMLFileReader` | Base class for VTK XML readers |
| `iGameCore/IO/VTK XML/iGameVTUReader.*` | `iGameVTUReader` | `.vtu` reader |
| `iGameCore/IO/VTK XML/iGameVTSReader.*` | `iGameVTSReader` | `.vts` reader |
| `iGameCore/IO/VTK XML/iGameVTPReader.*` | `iGameVTPReader` | `.vtp` reader |
| `iGameCore/IO/VTK XML/iGameVTMReader.*` | `iGameVTMReader` | `.vtm` multiblock reader |
| `iGameCore/IO/VTK XML/iGamePVDReader.*` | `iGamePVDReader` | `.pvd` time-series reader |
| `iGameCore/Core/Common/iGameStreamingData.*` | `StreamingData` / `TimeFrame` | PVD time-frame metadata |

### Usage

**Reading through the unified entry point:**

```cpp
#include "iGameFileIO.h"

auto legacyObj = iGame::FileIO::ReadFile("./Models/Tet_Plane.vtk");
auto vtuObj = iGame::FileIO::ReadFile("./Models/result.vtu");
auto vtsObj = iGame::FileIO::ReadFile("./Models/result.vts");
auto vtpObj = iGame::FileIO::ReadFile("./Models/surface.vtp");
auto vtmObj = iGame::FileIO::ReadFile("./Models/blocks.vtm");
auto pvdObj = iGame::FileIO::ReadFile("./Models/frames.pvd");
```

**Using the VTU reader directly:**

```cpp
#include "VTK XML/iGameVTUReader.h"

auto reader = iGame::iGameVTUReader::New();
reader->SetFilePath("./Models/result.vtu");

if (!reader->Execute()) {
    return;
}
auto obj = reader->GetOutput();
```

**Reading from memory:**

```cpp
#include "iGameFileIO.h"

// The lifetime of bytes must cover at least this function call.
const void* data = bytes.data();
const std::size_t size = bytes.size();

auto vtkObj = iGame::FileIO::ReadVTKFromMemory(data, size);
auto vtuObj = iGame::FileIO::ReadVTUFromMemory(data, size);
auto vtpObj = iGame::FileIO::ReadVTPFromMemory(data, size);
```

Memory-reading functions return `nullptr` for a null pointer or a zero-length buffer. The currently exposed VTK memory entry points cover `.vtk`, `.vtu`, and `.vtp`, but not `.vts`, `.vtm`, or `.pvd`.

![VTK 导入效果](../../Resources/Images/VTK导入.png)

### GUI

| Entry point | Description |
|-------------|-------------|
| File menu / toolbar: Open | `.vtk`, `.vtu`, `.vts`, `.vtm`, and `.pvd` can be selected through the file filter |
| `.vtp` | Supported by the API, but not currently listed in the file-selection filter |
| Animation dock | Calls `UpdateAnimation()` by time frame after a PVD file has been loaded |
| Model tree | VTM / PVD multiblock data is displayed as a parent object with child objects |

### Test Cases

| Target | Source file | Default data | Description |
|--------|-------------|--------------|-------------|
| `testSetRenderWindow` | `Examples/Rendering/RenderWindow/SetRenderWindow.cpp` | `./Models/Tet_Plane.vtk` | Legacy VTK reading and display |
| `testSetScalarField` | `Examples/Rendering/SetScalarField.cpp` | `./Models/Tet_Plane.vtk` | VTK attribute reading and scalar-field visualization |
| `testAnimation` | `Examples/Animation/TestAnimation.cpp` | `./Models/CAD11/_frames.pvd` (must be provided separately) | PVD time-series reading |

> There are currently no separately registered `testVTUReader`, `testVTSReader`, `testVTPReader`, or `testVTMReader` targets.

---

## Sub-feature 3: VTK Data Export

### Description

`VTKWriter` writes `SurfaceMesh`, `VolumeMesh`, `UnstructuredMesh`, and `StructuredMesh` objects in the Legacy VTK format, including point and cell attributes. `FileIO::WriteFile()` dispatches `.vtk` files to this writer and uses ASCII by default.

| Data object | Legacy VTK output |
|-------------|-------------------|
| `SurfaceMesh` | `POLYDATA` |
| `VolumeMesh` / `UnstructuredMesh` | `UNSTRUCTURED_GRID` |
| `StructuredMesh` | `STRUCTURED_GRID` |

Exported attribute types include `IG_SCALAR`, `IG_VECTOR`, `IG_TENSOR`, and `IG_NORMAL`. Cell-type mapping covers common lines, triangles, quadrilaterals, polygons, tetrahedra, hexahedra, prisms, pyramids, and some higher-order / Lagrange cells.

### Unified Export Status

| Format | Status | Description |
|--------|--------|-------------|
| `.vtk` | ✅ | `FileIO::WriteFile` → `VTKWriter` |
| `.vtm` | ⚠️ Routing integrated | `VTMWriter` contains manifest and `.vts` / `.vtu` child-block logic; different parent-object types require practical verification |
| `.vtu` | ⚠️ Writer class exists | The current `FileIO::WriteFile` branch does not call `VTUWriter` |
| `.vts` | ⚠️ Writer class exists | The current `FileIO::WriteFile` branch does not call `VTSWriter` |
| `.vtp` | ❌ | No unified export implementation |
| `.pvd` | ❌ | Not implemented by the current `FileIO::WriteFile` branch |
| `.cgns` | ❌ | Import only |

### Source Paths

| Path | Class / API | Description |
|------|-------------|-------------|
| `iGameCore/IO/iGameFileWriter.*` | `FileWriter` | Writer base class and buffer output |
| `iGameCore/IO/VTK/iGameVTKWriter.*` | `VTKWriter` | Legacy VTK output |
| `iGameCore/IO/VTK XML/iGameVTUWriter.*` | `VTUWriter` | VTU writer, not connected to unified extension dispatch |
| `iGameCore/IO/VTK XML/iGameVTSWriter.*` | `VTSWriter` | VTS writer, not connected to unified extension dispatch |
| `iGameCore/IO/VTK XML/iGameVTMWriter.*` | `VTMWriter` | VTM manifest and child-block output |
| `iGameCore/IO/iGameFileIO.cpp` | `FileIO::WriteFile` | Actual scope of the public unified export capability |

### Usage

**Writing Legacy VTK through the unified entry point (recommended):**

```cpp
#include "iGameFileIO.h"

auto obj = iGame::FileIO::ReadFile("./Models/Tet_Plane.vtk");
if (obj == nullptr) {
    return;
}

const bool ok = iGame::FileIO::WriteFile("./Output/result.vtk", obj);
if (!ok) {
    // Unwritable path, unsupported object, or writer execution failure
    return;
}
```

**Using the VTK writer directly:**

```cpp
#include "VTK/iGameVTKWriter.h"

auto writer = iGame::VTKWriter::New();
writer->SetFileType(IGAME_ASCII);

const bool ok = writer->WriteToFile(obj, "./Output/result.vtk");
```

> The `SetFileType(IGAME_BINARY)` interface exists, but the current binary-output branches do not yet provide complete coverage of cell types and attribute data. ASCII is recommended for production use and acceptance testing.

### GUI

```text
File menu: Save As
  → igQtFileLoader::SaveFile()
  → Obtain the current model's DataObject
  → FileIO::WriteFile(filePath, obj)
```

The GUI save filter may list extensions that have not yet been connected to `FileIO::WriteFile()`. Actual write support should be determined from the “Unified Export Status” table above and the Boolean result returned by `WriteFile()`.

### Test Cases

There is currently no independently registered VTK Writer example target. Round-trip verification is recommended:

```cpp
#include "iGameFileIO.h"

auto source = iGame::FileIO::ReadFile("./Models/Tet_Plane.vtk");
bool written = iGame::FileIO::WriteFile("./Output/roundtrip.vtk", source);
auto result = written
    ? iGame::FileIO::ReadFile("./Output/roundtrip.vtk")
    : nullptr;
```

Acceptance testing should compare the point count, cell count, cell types, attribute names, attribute associations, and value ranges before and after the round trip.

---

## Sub-feature 4: CGNS Data Import

### Description

`iGameCGNSReader` uses the CGNS Library to read bases, zones, coordinates, cell connectivity, and FlowSolution data from ADF / HDF5 containers. A single zone produces a mesh directly, while multiple zones or bases are organized as a parent object with child objects.

| CGNS content | Conversion result |
|--------------|-------------------|
| `Structured` zone | `StructuredMesh` |
| `Unstructured` zone | `UnstructuredMesh` |
| Multiple zones / bases | Parent `DrawObject` + multiple child meshes |
| `GridLocation=Vertex` | `IG_POINT` attribute |
| `GridLocation=CellCenter` | `IG_CELL` attribute |
| `Integer` / `LongInteger` | `IntArray` / `LongLongArray` |
| `RealSingle` / `RealDouble` | `FloatArray` / `DoubleArray` |

Common cell mappings include:

| CGNS type | iGameVis type |
|-----------|----------------|
| `TRI_3` | `IG_TRIANGLE` |
| `QUAD_4` | `IG_QUAD` |
| `TETRA_4` | `IG_TETRA` |
| `HEXA_8` | `IG_HEXAHEDRON` |
| `PYRA_5` | `IG_PYRAMID` |
| `PENTA_6` | `IG_PRISM` |

The reader also handles MIXED, NGON / NFACE, and other connectivity representations, and collects boundary section names for the polyhedral-section processing path.

### Source Paths

| Path | Class / API | Description |
|------|-------------|-------------|
| `iGameCore/IO/CGNS/iGameCGNSReader.h` | `iGameCGNSReader` | CGNS reader interface |
| `iGameCore/IO/CGNS/iGameCGNSReader.cpp` | `Execute`, `ReadFields`, etc. | Base / zone / mesh / attribute parsing |
| `ThirdParty/cgns/` | CGNS Library | Optional third-party dependency |
| `iGameCore/CMakeLists.txt` | `CGNS_ENABLE` | Compile definition and `cgns_static` linkage |
| `Examples/IO/CGNSReader.cpp` | `testCGNS` | CGNS reading and display example |

### Build Conditions

CGNS is disabled by default and must be enabled during configuration:

```shell
cmake -S . -B out/build/cgns -DENABLE_CGNS_MODULE=ON -DEXAMPLE_COMPILE=ON
cmake --build out/build/cgns --target testCGNS
```

When enabled:

- `iGameCore` defines `CGNS_ENABLE`;
- `iGameCore` links against `cgns_static`;
- the `testCGNS` example target is registered;
- `FileIO::ReadFile("*.cgns")` enters the CGNS reader branch.

### Usage

**Reading through the unified entry point (recommended):**

```cpp
#include "iGameFileIO.h"

auto obj = iGame::FileIO::ReadFile(
    "./Models/F6-coarse-vol-v2.cgns");

if (obj == nullptr) {
    return;
}
```

**Using the CGNS reader directly:**

```cpp
#include "CGNS/iGameCGNSReader.h"

auto reader = iGame::iGameCGNSReader::New();
auto obj = reader->ReadFile(
    "./Models/F6-coarse-vol-v2.cgns");

if (obj == nullptr) {
    return;
}
```

### GUI

| Entry point | Description |
|-------------|-------------|
| File menu / toolbar: Open → CGNS file | Selects a `.cgns` file |
| `igQtFileLoader::OpenFile()` | Enters the CGNS reader through `FileIO::ReadFile()` |
| Reading progress | The reader updates progress during the base, zone, and attribute stages |
| Model tree | A multi-zone file is displayed as a parent object with multiple child meshes |

> The GUI filter may always display `.cgns`. If the project was built without `ENABLE_CGNS_MODULE`, the unified entry point does not execute CGNS reading and returns `nullptr`.

![CGNS 导入效果](../../Resources/Images/CGNS导入.png)

### Test Cases

| Target | Source file | Default data | Condition |
|--------|-------------|--------------|-----------|
| `testCGNS` | `Examples/IO/CGNSReader.cpp` | `./Models/F6-coarse-vol-v2.cgns` | `ENABLE_CGNS_MODULE=ON`; the data file must exist |

---

## Overall Data Flow into the Visualization Platform

```text
VTK / CGNS files or VTK data in memory
  → FileIO / concrete reader
  → Points + Cells + AttributeSet
  → SurfaceMesh / StructuredMesh / UnstructuredMesh
      ├─ VTM / CGNS with multiple zones: parent object + child objects
      └─ PVD: StreamingData + TimeFrame
  → igQtFileLoader / Scene::AddModel
  → 11.3 Field Visualization
  → 11.4 Parallel and GPU-Accelerated Rendering
```

---

## Related Examples

| Example target | Description | Condition |
|----------------|-------------|-----------|
| `testSetRenderWindow` | Legacy VTK reading and basic display | Default |
| `testSetScalarField` | VTK attribute reading and scalar-field visualization | Default |
| `testAnimation` | PVD time-series data | Example data must be provided separately |
| `testCGNS` | CGNS mesh, attributes, and display | `ENABLE_CGNS_MODULE=ON` |

Related examples must be enabled at build time:

```cmake
EXAMPLE_COMPILE=ON
```

---

## Acceptance Self-Check List

| Sub-feature | Recommended verification |
|-------------|--------------------------|
| Unified dispatch | Uppercase and lowercase extensions are both recognized; unknown formats return `nullptr`; the object name is correct after a successful read |
| Legacy VTK | ASCII / binary files can be read; structured, unstructured, and surface-mesh types are correct |
| VTK attributes | Point / cell attribute counts, names, dimensions, associations, and value ranges are correct |
| VTK XML | Geometry and attributes are correct for `.vtu`, `.vts`, and `.vtp`; ASCII / binary / appended samples are verified within the implemented scope |
| VTM | Child-block count, names, and hierarchy are correct; relative references are verified separately with `/` and `\` paths |
| PVD | Timestep count and values are correct; first-frame child blocks are visible; `UpdateAnimation()` can select later frames |
| VTK export | `.vtk` is written successfully and can be read again; points, cells, types, and attributes remain consistent after a round trip |
| VTM export | For both a single mesh and a parent object with multiple child blocks, inspect the manifest and `.vtu` / `.vts` child files and confirm that the output can be reopened |
| CGNS build | With the module disabled, the CGNS branch is not entered; with it enabled, `testCGNS` builds successfully |
| CGNS meshes | Structured / unstructured zones, common cells, and multi-zone hierarchy are correct |
| CGNS attributes | Vertex / CellCenter associations and Integer / Real data types are correct |
