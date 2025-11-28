#ifndef iGameConvertPolyhedralCells_h
#define iGameConvertPolyhedralCells_h

/**
* @class    ConvertPolyhedralCells
 * @brief   转换体网格单元的过滤器
 */

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN
class ConvertPolyhedralCellsFilter : public Filter {
public:
    I_OBJECT(ConvertPolyhedralCellsFilter);
    static Pointer New() { return new ConvertPolyhedralCellsFilter; }

    bool Execute() override;

    // 转化为四面体网格
    static bool ConvertToTetra(VolumeMesh::Pointer OldMesh, VolumeMesh::Pointer NewMesh);
    static bool ConvertToTetra(UnstructuredMesh::Pointer OldMesh, UnstructuredMesh::Pointer NewMesh);

protected:
    ConvertPolyhedralCellsFilter();
    ~ConvertPolyhedralCellsFilter() override = default;
};
IGAME_NAMESPACE_END
#endif
