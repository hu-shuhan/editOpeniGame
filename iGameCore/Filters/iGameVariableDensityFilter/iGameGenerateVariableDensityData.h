#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameModel.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVariableDensityData.h>
IGAME_NAMESPACE_BEGIN
class iGameGenerateVariableDensityData : public Filter {
public:
    I_OBJECT(iGameGenerateVariableDensityData);
    static Pointer New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType, int boxNum) {
        return new iGameGenerateVariableDensityData(attrs, dataType, boxNum);
    }
    bool Execute() override;

private:
    void RUN();

protected:
    iGameGenerateVariableDensityData(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType, int boxNum);
    ~iGameGenerateVariableDensityData() override = default;

private:
    /* Input */
    ElementArray<AttributeSet::Attribute>::Pointer m_Attrs;
    IGenum m_DataType{};
    int m_BoxNum{};

private:
    /* Output */
    VariableDensityData::Pointer m_Data;
};
IGAME_NAMESPACE_END