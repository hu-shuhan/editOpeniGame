#ifndef iGameCellCenterFilter_h
#define iGameCellCenterFilter_h

#include "iGameFilter.h"
#include "iGameDataObject.h"
#include "iGamePointSet.h"

IGAME_NAMESPACE_BEGIN

//CellCenterFilter: 遍历网格的所有单元，计算每个单元的几何中心
//输出一个 PointSet，每个点对应一个单元中心，可直接作为点云渲染
class CellCenterFilter : public Filter {
public:
    I_OBJECT(CellCenterFilter);
    static Pointer New() { return new CellCenterFilter; }

    bool Execute() override;

protected:
    CellCenterFilter();
    ~CellCenterFilter() override = default;
};

IGAME_NAMESPACE_END
#endif
