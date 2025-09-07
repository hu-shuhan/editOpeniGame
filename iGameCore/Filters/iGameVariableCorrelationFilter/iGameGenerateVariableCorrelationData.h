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
    static Pointer New(IGenum dataType) { return new iGameGenerateVariableCorrelationData(dataType); }
    bool Execute() override;

private:
    void RUN();

protected:
    iGameGenerateVariableCorrelationData(IGenum dataType);
    ~iGameGenerateVariableCorrelationData() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    IGenum m_DataType{};

private:
    /* Output */
    VariableCorrelationData::Pointer m_Data;
};
IGAME_NAMESPACE_END