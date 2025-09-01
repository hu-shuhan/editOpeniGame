#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameModel.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVariableCorrelationData.h>
IGAME_NAMESPACE_BEGIN
class iGameGenerateVariableCorrelationData : public Filter {
public:
    I_OBJECT(iGameGenerateVariableCorrelationData);
    static Pointer New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType) {
        return new iGameGenerateVariableCorrelationData(attrs, dataType);
    }
    bool Execute() override;

private:
    void RUN();

protected:
    iGameGenerateVariableCorrelationData(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType);
    ~iGameGenerateVariableCorrelationData() override = default;

private:
    /* Input */
    ElementArray<AttributeSet::Attribute>::Pointer m_Attrs;
    IGenum m_DataType{};

private:
    /* Output */
    VariableCorrelationData::Pointer m_Data;
};
IGAME_NAMESPACE_END