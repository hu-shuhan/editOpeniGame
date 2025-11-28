#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameModel.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVariableDensityData.h>
IGAME_NAMESPACE_BEGIN
class GenerateVariableDensityDataFilter : public Filter {
public:
    I_OBJECT(GenerateVariableDensityDataFilter);
    static Pointer New(IGenum dataType, int boxNum) { return new GenerateVariableDensityDataFilter(dataType, boxNum); }
    bool Execute() override;

private:
    void Run();

protected:
    GenerateVariableDensityDataFilter(IGenum dataType, int boxNum);
    ~GenerateVariableDensityDataFilter() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    IGenum m_DataType{};
    int m_BoxNum{};

private:
    /* Output */
    VariableDensityData::Pointer m_Data;
};
IGAME_NAMESPACE_END