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
    static Pointer New(IGenum dataType) { return new iGameGenerateParallelCoordinatesData(dataType); }
    bool Execute() override;

private:
    void Run();

protected:
    iGameGenerateParallelCoordinatesData(IGenum dataType);
    ~iGameGenerateParallelCoordinatesData() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    IGenum m_DataType{};

private:
    /* Output */
    ParallelCoordinatesData::Pointer m_Data;
};
IGAME_NAMESPACE_END