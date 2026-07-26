#pragma once

#include <iGameDataObject.h>
#include <iGameUnstructuredMesh.h>
#include <iGameSurfaceMesh.h>
#include <iGameFlatArray.h>
#include <vector>
#include <iGameType.h>

IGAME_NAMESPACE_BEGIN
// 输入原始mesh和含分块信息的unstructmesh，将分块信息映射回原始mesh
class BlockMapping {
public:
    static std::vector<int> GetMappingBlockCells(SurfaceMesh::Pointer oriMesh, UnstructuredMesh::Pointer partedMesh);
    static IntArray::Pointer GetMappingBlockCellsArray(SurfaceMesh::Pointer oriMesh, UnstructuredMesh::Pointer partedMesh);

private:
    // 获取part_id
    static ArrayObject::Pointer GetPartId(UnstructuredMesh::Pointer partedMesh);
};
IGAME_NAMESPACE_END