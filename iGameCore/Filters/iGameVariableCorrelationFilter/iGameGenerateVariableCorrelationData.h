#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameModel.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVariableCorrelationData.h>
IGAME_NAMESPACE_BEGIN
class GenerateVariableCorrelationDataFilter : public Filter {
public:
    I_OBJECT(GenerateVariableCorrelationDataFilter);
    static Pointer New(IGenum dataType) { return new GenerateVariableCorrelationDataFilter(dataType); }
    bool Execute() override;

private:
    void Run();

protected:
    GenerateVariableCorrelationDataFilter(IGenum dataType);
    ~GenerateVariableCorrelationDataFilter() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    IGenum m_DataType{};

private:
    /* Output */
    VariableCorrelationData::Pointer m_Data;
};
IGAME_NAMESPACE_END