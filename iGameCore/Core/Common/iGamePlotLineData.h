#pragma once
#include <iGameDataObject.h>
#include <iGameCtxPresObjData.h>
#include <vector>
#include <map>
#include <tuple>
#include <set>
#include <utility>
#include <iGamePoints.h>
#include <iGameCellArray.h>
#include <iGameUnstructuredMesh.h>
IGAME_NAMESPACE_BEGIN
class PlotLineData : public DataObject, public CtxPresObjData_Main, public CtxPresObjData_LightAlpha {
public:
    I_OBJECT(PlotLineData);

    void SetObjDistance(const std::vector<double>& objDistance);
    const std::vector<double>& GetObjDistance();

    void SetObjDrawSort(const std::vector<int>& objDrawSort);
    const std::vector<int>& GetObjDrawSort();

    //void SetVariableHue(const std::vector<int>& variableHue);
    //const std::vector<int>& GetVariableHue();

    void SetVariableHS(const std::vector<std::pair<int, int>>& variableHS);
    const std::vector<std::pair<int, int>>& GetVariableHS();

    void SetVariableColor(const std::vector<std::tuple<int, int, int>>& variableColor);
    const std::vector<std::tuple<int, int, int>>& GetVariableColor();

    void SetChoosedVariableColor(const std::vector<std::tuple<int, int, int>>& variableColor);
    const std::vector<std::tuple<int, int, int>>& GetChoosedVariableColor();

    void SetMaxDistance(double maxDistance);
    double GetMaxDistance() const;

    void SetMinDistance(double minDistance);
    double GetMinDistance() const;

    void SetMaxValue(double value);
    double GetMaxValue() const;

    void SetMinValue(double value);
    double GetMinValue() const;

    //[objId, objIndex in m_ObjDistance and m_ObjectDatas]
    void SetObjIndexs(const std::map<int, int>& objIndexs);

    //[objId, objIndex in m_ObjDistance and m_ObjectDatas]
    const std::map<int, int>& GetObjIndexs();

protected:
    std::vector<double> m_ObjDistance;//[objIndex]

    std::vector<int> m_ObjDrawSort;//[objId]

    //std::vector<int> m_VariableHue;

    std::vector<std::pair<int, int>> m_VariableHS;

    std::vector<std::tuple<int, int, int>> m_VariableColor;

    std::vector<std::tuple<int, int, int>> m_ChoosedVariableColor;

    double m_MaxDistance{};
    double m_MinDistance{};

    double m_MaxValue{};
    double m_MinValue{};

    std::map<int, int> m_ObjIndexs;//[objId, objIndex in m_ObjDistance]

public:
    /* static funcs */
    static std::map<int, int> GenerateObjIndex(const Point& startPoint, const Point& endPoint, Points::Pointer points,
                                               CellArray::Pointer cells, UnstructuredMesh::Pointer mesh,
                                               IGenum dataType);
    static std::vector<double> GenerateObjDistance(const Point& startPoint, const std::map<int, int>& objIndexs,
                                                   Points::Pointer points);
    static double GenerateObjDistance(const Point& startPoint, int objId, Points::Pointer points);
    static std::vector<double> GenerateObjDistance(const Point& startPoint, const std::map<int, int>& objIndexs,
                                                   CellArray::Pointer cells, Points::Pointer points);
    static double GenerateObjDistance(const Point& startPoint, int objId, CellArray::Pointer cells,
                                      Points::Pointer points);
    static std::vector<int> GenerateObjDrawSort(const std::vector<double>& objDistance,
                                                const std::map<int, int>& objIndexs);
    static double GenerateObjMaxDistance(const std::vector<int>& objDrawSort, const std::vector<double>& objDistance,
                                         const std::map<int, int>& objIndexs);
    static double GenerateObjMinDistance(const std::vector<int>& objDrawSort, const std::vector<double>& objDistance,
                                         const std::map<int, int>& objIndexs);
    static std::pair<double, double> GenerateObjMinMaxValue(const std::map<int, int>& objIndexs,
                                                            const std::vector<bool>& variableShow,
                                                            CtxPresObjData_Main* theData);
    static std::vector<int> GenerateVariableHue(int variableNum);
    static std::vector<std::pair<int, int>> GenerateHS(int variableNum, int minH, int maxH, int minS, int maxS);
    static std::vector<std::tuple<int, int, int>> GenerateVariableColor(const std::vector<int>& variableHue,
                                                                        int saturation, int light);
    static std::vector<std::tuple<int, int, int>>
    GenerateVariableColor(const std::vector<std::pair<int, int>>& variableHS, int light);

private:
    static double GenerateMinValueInChoosedVariable(const std::vector<double>& minValues,
                                                    const std::vector<bool>& variableShow);
    static double GenerateMaxValueInChoosedVariable(const std::vector<double>& maxValues,
                                                    const std::vector<bool>& variableShow);

public:
    /* delete func */
    void SetChoosedAlpha(int alpha) = delete;
    int GetChoosedAlpha() const = delete;

    void SetUnChoosedAlpha(int alpha) = delete;
    int GetUnChoosedAlpha() const = delete;

    void SetKeyObjectIds(const std::vector<int>& keyObjIds) = delete;
    const std::vector<int>& GetKeyObjectIds() = delete;

public:
    /* init func */
    static Pointer New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType, int minH, int maxH,
                       int minS, int maxS);
    void SetRadialData(ElementArray<AttributeSet::Attribute>::Pointer attrs, int objNum,
                       ScalarsToColors::Pointer colorMap, const Point& startPoint, const Point& endPoint,
                       UnstructuredMesh::Pointer mesh);
    void SetRadialData(ElementArray<AttributeSet::Attribute>::Pointer attrs, const Point& startPoint,
                       const Point& endPoint, UnstructuredMesh::Pointer mesh);

protected:
    PlotLineData() = default;
    static Pointer New() { return new PlotLineData(); }

public:
    /* choose func */
    std::vector<igIndex> FiltInRangeIds(double minDistance, double maxDistance, double minValue, double maxValue,
                                        std::vector<bool> variableCanBeChoose);

public:
    /* selection set */
    void SetDefaultSelectionFunc(const std::string& funcName, Selection* selection);

protected:
    void DefaultSelectionCallBackFunc(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope);
    void DefaultClearSelectionCallBackFunc();
};
IGAME_NAMESPACE_END