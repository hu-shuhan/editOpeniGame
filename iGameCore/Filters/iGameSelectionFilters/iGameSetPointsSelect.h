#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <vector>
IGAME_NAMESPACE_BEGIN
class iGameSetPointsSelect : public Filter {
public:
    I_OBJECT(iGameSetPointsSelect);
    static Pointer New(Selection::Event::Operate ope, const std::vector<int>& ids, Painter3D* painter = nullptr) {
        return new iGameSetPointsSelect(ope, ids, painter);
    }
    bool Execute() override;

private:
    void Run();

protected:
    iGameSetPointsSelect(Selection::Event::Operate ope, const std::vector<int>& ids, Painter3D* painter = nullptr);
    ~iGameSetPointsSelect() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    Selection::Event::Operate m_Operate;
    std::vector<int> m_Ids;
    Painter3D* m_Painter{};

private:
    /* Output */
};
IGAME_NAMESPACE_END