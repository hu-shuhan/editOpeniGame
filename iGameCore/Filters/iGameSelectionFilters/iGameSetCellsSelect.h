#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <vector>
IGAME_NAMESPACE_BEGIN
class iGameSetCellsSelect : public Filter {
public:
    I_OBJECT(iGameSetCellsSelect);
    static Pointer New(Selection::Event::Operate ope, const std::vector<int>& ids, Points* points,
                       CellArray* cellArrays, Painter3D* painter = nullptr) {
        return new iGameSetCellsSelect(ope, ids, points, cellArrays, painter);
    }
    bool Execute() override;

private:
    void RUN();

protected:
    iGameSetCellsSelect(Selection::Event::Operate ope, const std::vector<int>& ids, Points* points,
                        CellArray* cellArrays, Painter3D* painter = nullptr);
    ~iGameSetCellsSelect() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    Selection::Event::Operate m_Operate;
    std::vector<int> m_Ids;
    Points* m_Points{};
    CellArray* m_CellArrays{};
    Painter3D* m_Painter{};

private:
    /* Output */
};
IGAME_NAMESPACE_END