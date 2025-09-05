#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameSelection.h>
#include <vector>
IGAME_NAMESPACE_BEGIN
class iGameSetPointsSelect : public Filter {
public:
    I_OBJECT(iGameSetPointsSelect);
    static Pointer New(Selection::Pointer selection, Selection::Event::Operate ope, const std::vector<int>& ids,
                       Points* points, CellArray* cellArrays, Painter3D* painter = nullptr) {
        return new iGameSetPointsSelect(selection, ope, ids, points, cellArrays, painter);
    }
    bool Execute() override;
    const std::vector<Selection::Event>& GetResult();

private:
    void RUN();

protected:
    iGameSetPointsSelect(Selection::Pointer selection, Selection::Event::Operate ope, const std::vector<int>& ids,
                         Points* points, CellArray* cellArrays, Painter3D* painter = nullptr);
    ~iGameSetPointsSelect() override = default;

private:
    /* Input */
    Selection::Pointer m_Selection;
    Selection::Event::Operate m_Operate;
    std::vector<int> m_Ids;
    Points* m_Points{};
    CellArray* m_CellArrays{};
    Painter3D* m_Painter{};

private:
    /* Output */
    std::vector<Selection::Event> m_Events;
};
IGAME_NAMESPACE_END