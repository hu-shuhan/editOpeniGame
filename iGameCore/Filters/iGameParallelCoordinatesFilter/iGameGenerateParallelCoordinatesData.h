#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameModel.h>
#include <iGameParallelCoordinatesData.h>
#include <iGameUnstructuredMesh.h>
IGAME_NAMESPACE_BEGIN
class iGameGenerateParallelCoordinatesData : public Filter {
public:
    I_OBJECT(iGameGenerateParallelCoordinatesData);
    static Pointer New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType) {
        return new iGameGenerateParallelCoordinatesData(attrs, dataType);
    }
    bool Execute() override;

private:
    void RUN();

protected:
    iGameGenerateParallelCoordinatesData(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType);
    ~iGameGenerateParallelCoordinatesData() override = default;

private:
    /* Input */
    ElementArray<AttributeSet::Attribute>::Pointer m_Attrs;
    IGenum m_DataType{};

private:
    /* Output */
    ParallelCoordinatesData::Pointer m_Data;
};
IGAME_NAMESPACE_END