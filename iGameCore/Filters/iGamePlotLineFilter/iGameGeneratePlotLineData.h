#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameModel.h>
#include <iGameUnstructuredMesh.h>
#include <iGameDataChangeData.h>
#include <iGamePoints.h>
IGAME_NAMESPACE_BEGIN
class iGameGeneratePlotLineData : public Filter {
public:
    I_OBJECT(iGameGeneratePlotLineData);
    static Pointer New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType, const Point& startPoint,
                       const Point& endPoint) {
        new iGameGeneratePlotLineData(attrs, dataType, startPoint, endPoint);
    }
    bool Execute() override;

private:
    void RUN();

protected:
    iGameGeneratePlotLineData(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType,
                              const Point& startPoint, const Point& endPoint);
    ~iGameGeneratePlotLineData() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    ElementArray<AttributeSet::Attribute>::Pointer m_Attrs;
    IGenum m_DataType{};
    Point m_StartPoint, m_EndPoint;

private:
    /* Output */
    DataChangeData::Pointer m_Data;
};
IGAME_NAMESPACE_END