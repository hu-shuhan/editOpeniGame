#ifndef IGAME_VTSWRITER_H
#define IGAME_VTSWRITER_H

#include "iGameFileWriter.h"
#include "iGameStructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class VTSWriter : public FileWriter {
public:
    using Pointer = std::shared_ptr<VTSWriter>;
    static Pointer New() { return std::make_shared<VTSWriter>(); }
    void SetInput(StructuredMesh::Pointer mesh) { m_StructuredMesh = mesh; }
    bool GenerateBuffers() override;

protected:
    void WriteHeaderToBuffer();
    void WritePointsToBuffer(Points::Pointer Points);
    void WritePointAttributesToBuffer(AttributeSet::Pointer AttributeSet);
    void WriteCellAttributesToBuffer(AttributeSet::Pointer AttributeSet);

protected:
    StructuredMesh::Pointer m_StructuredMesh;
};

IGAME_NAMESPACE_END

#endif // IGAME_VTSWRITER_H