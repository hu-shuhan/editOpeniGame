#pragma once

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameParallelCoordinatesData.h"

IGAME_NAMESPACE_BEGIN
class ParallelCoordinates : public Filter {
public:
    I_OBJECT(ParallelCoordinates);
    static Pointer New() { return new ParallelCoordinates; }
    bool Execute() override;

protected:
    ParallelCoordinatesData::Pointer CreateParallelCoordinatesPointData();
    ParallelCoordinatesData::Pointer CreateParallelCoordinatesCellData();
    ParallelCoordinates() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(2);
    }
    ~ParallelCoordinates() override = default;

    UnstructuredMesh::Pointer m_Mesh{};
};
IGAME_NAMESPACE_END