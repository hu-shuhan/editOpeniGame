#pragma once
#include <iGamePlotLineData.h>
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameModel.h>
#include <iGamePoints.h>
#include <iGameUnstructuredMesh.h>
IGAME_NAMESPACE_BEGIN
class iGameGeneratePlotLineData : public Filter {
public:
    I_OBJECT(iGameGeneratePlotLineData);
    static Pointer New(IGenum dataType, const Point& startPoint, const Point& endPoint) {
        return new iGameGeneratePlotLineData(dataType, startPoint, endPoint);
    }
    bool Execute() override;

private:
    void Run();

protected:
    iGameGeneratePlotLineData(IGenum dataType, const Point& startPoint, const Point& endPoint);
    ~iGameGeneratePlotLineData() override = default;

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