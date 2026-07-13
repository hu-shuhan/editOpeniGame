# Metric 7.1: High-Order Visualization Module

## Purpose

Provides high-fidelity visualization for high-order CAE simulation data, including Lagrange high-order element mesh conversion and NURBS/Spline geometry reading and rendering. Supports upgrading linear meshes to high-order unstructured meshes and precise drawing of spline surface and volume data.

Key capabilities:

- Conversion from linear to Lagrange high-order unstructured meshes
- Lagrange high-order cell type definitions (triangles, tetrahedra, etc.)
- CPU/GPU reading and visualization of NURBS/Spline geometry data

## Core Implementation in This Directory

| File | Class | Description |
|------|-------|-------------|
| `iGameConvertToLagrangeUnstructuredMeshFilter.h/.cpp` | `ConvertToLagrangeUnstructuredMeshFilter` | Converts linear unstructured mesh to Lagrange high-order mesh |

## Related Source Paths

- High-order cell models: [`../../Core/CellModel/iGameLagrangeTriangle.h`](../../Core/CellModel/iGameLagrangeTriangle.h), etc.
- High-order mesh data: [`../../Core/DataModel/iGameLagrangeUnstructuredMesh.h`](../../Core/DataModel/iGameLagrangeUnstructuredMesh.h)
- Spline geometry: [`../../Core/DataModel/iGameSplineGeometry.h`](../../Core/DataModel/iGameSplineGeometry.h)
- Spline XML readers: [`../../IO/Spline XML/iGameSplineReaderCPU.h`](../../IO/Spline%20XML/iGameSplineReaderCPU.h), [`iGameSplineReaderGPU.h`](../../IO/Spline%20XML/iGameSplineReaderGPU.h)

## How It Is Called

### API: High-Order Mesh Conversion

```cpp
auto filter = iGame::ConvertToLagrangeUnstructuredMeshFilter::New();
filter->SetInput(mesh);
filter->Execute();
auto output = filter->GetOutput(0);
```

### API: Spline Data Reading

```cpp
auto reader = iGame::SplineReaderCPU::New();
reader->SetFilePath(filePath);
reader->Execute();
auto obj = reader->GetOutput();
```

### GUI Invocation

The Qt frontend loads `.xml` spline files via `igQtFileLoader::OpenSplineFile()`, selecting CPU/GPU surface or volume readers (`SplineReaderCPU` / `SplineReaderGPU`) based on user options.

## Related Examples

| Example Target | Description |
|----------------|-------------|
| `testConvertToLagrangeUnstructuredMesh` | Linear to Lagrange high-order mesh conversion |
| `testSplineReaderCPU` | CPU spline data reading and visualization |