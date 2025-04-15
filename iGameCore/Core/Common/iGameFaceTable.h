#ifndef iGameFaceTable_h
#define iGameFaceTable_h

#include "iGameObject.h"
#include "iGameCellArray.h"
#include "iGameElementArray.h"

IGAME_NAMESPACE_BEGIN
class FaceTable : public Object {
public:
    I_OBJECT(FaceTable);
    static Pointer New() { return new FaceTable; }

    igIndex IsFace(igIndex* face, int size);

    void InsertFace(igIndex* face, int size);

    igIndex GetNumberOfFaces();
    CellArray::Pointer GetOutput();

protected:
    FaceTable();
    ~FaceTable() override = default;

    void Resize(igIndex newSize);

    std::vector<IdArray::Pointer> Array;
    igIndex Size;

    CellArray::Pointer Faces;
    igIndex NumberOfFaces;
};

IGAME_NAMESPACE_END
#endif