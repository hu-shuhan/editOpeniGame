#pragma once
#include <iGamePlotLineData.h>
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameModel.h>
#include <iGamePoints.h>
#include <iGameUnstructuredMesh.h>
IGAME_NAMESPACE_BEGIN
class GeneratePlotLineDataFilter : public Filter {
public:
    I_OBJECT(GeneratePlotLineDataFilter);
    static Pointer New(IGenum dataType, const Point& startPoint, const Point& endPoint) {
        return new GeneratePlotLineDataFilter(dataType, startPoint, endPoint);
    }
    bool Execute() override;

private:
    void Run();

protected:
    GeneratePlotLineDataFilter(IGenum dataType, const Point& startPoint, const Point& endPoint);
    ~GeneratePlotLineDataFilter() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    IGenum m_DataType{};
    Point m_StartPoint, m_EndPoint;

private:
    /* Output */
    PlotLineData::Pointer m_Data;
};
IGAME_NAMESPACE_END