#pragma once

#include <iGameDataObject.h>
#include <iGameUnstructuredMesh.h>
#include <iGameSurfaceMesh.h>
#include <iGameVolumeMesh.h>
#include <iGamePointFinder.h>
#include <iGameFlatArray.h>
#include <vector>
#include <iGameType.h>

IGAME_NAMESPACE_BEGIN
// 输入原始mesh和含分块信息的unstructmesh，将分块信息映射回原始mesh
class BlockMapping {
public:
    static std::vector<int> GetMappingBlockCells(SurfaceMesh::Pointer oriMesh, UnstructuredMesh::Pointer partedMesh);
    static IntArray::Pointer GetMappingBlockCellsArray(SurfaceMesh::Pointer oriMesh, UnstructuredMesh::Pointer partedMesh);

    static std::vector<int> GetMappingBlockCells(UnstructuredMesh::Pointer oriMesh, UnstructuredMesh::Pointer partedMesh);
    static IntArray::Pointer GetMappingBlockCellsArray(UnstructuredMesh::Pointer oriMesh, UnstructuredMesh::Pointer partedMesh);

    static std::vector<int> GetMappingBlockCells(VolumeMesh::Pointer oriMesh, UnstructuredMesh::Pointer partedMesh);
    static IntArray::Pointer GetMappingBlockCellsArray(VolumeMesh::Pointer oriMesh, UnstructuredMesh::Pointer partedMesh);

private:
    static ArrayObject::Pointer GetPartId(UnstructuredMesh::Pointer partedMesh);
    static PointFinder::Pointer BuildCentroidFinder(UnstructuredMesh::Pointer partedMesh);
};
IGAME_NAMESPACE_END