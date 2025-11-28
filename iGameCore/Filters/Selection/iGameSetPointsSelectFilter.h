#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <vector>
IGAME_NAMESPACE_BEGIN
class SetPointsSelectFilter : public Filter {
public:
    I_OBJECT(SetPointsSelectFilter);
    static Pointer New(Selection::Operate ope, const std::vector<int>& ids, Painter3D* painter = nullptr) {
        return new SetPointsSelectFilter(ope, ids, painter);
    }
    bool Execute() override;

private:
    void Run();

protected:
    SetPointsSelectFilter(Selection::Operate ope, const std::vector<int>& ids, Painter3D* painter = nullptr);
    ~SetPointsSelectFilter() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    Selection::Operate m_Operate;
    std::vector<int> m_Ids;
    Painter3D* m_Painter{};

private:
    /* Output */
};
IGAME_NAMESPACE_END