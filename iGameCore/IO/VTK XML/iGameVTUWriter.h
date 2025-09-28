#ifndef IGAME_VTUWRITER_H
#define IGAME_VTUWRITER_H

#include "iGameFileWriter.h"
#include "iGameUnstructuredMesh.h"
#include "VTK/iGameVTKAbstractReader.h"
IGAME_NAMESPACE_BEGIN

class VTUWriter : public FileWriter {
public:
    using Pointer = std::shared_ptr<VTUWriter>;
    static Pointer New() { return std::make_shared<VTUWriter>(); }

    bool GenerateBuffers() override;

    void SetInput(UnstructuredMesh::Pointer mesh) { m_UnstructuredMesh = mesh; }

protected:
    void WriteHeaderToBuffer();
    void WritePointsToBuffer(Points::Pointer Points);
    void WriteCellsToBuffer(CellArray::Pointer Cells);
    void WriteCellsTypeToBuffer();
    void WritePointsAttributesToBuffer(AttributeSet::Pointer AttributeSet);
    void WriteCellsAttributesToBuffer(AttributeSet::Pointer AttributeSet);

protected:
    UnstructuredMesh::Pointer m_UnstructuredMesh;
};

IGAME_NAMESPACE_END

#endif // IGAME_VTUWRITER_H
