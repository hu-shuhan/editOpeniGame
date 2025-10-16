#ifndef EncodedData_h
#define EncodedData_h

#include "iGameDataObject.h"

IGAME_NAMESPACE_BEGIN

class EncodedMeshData : public DataObject {
public:
    I_OBJECT(EncodedMeshData);
    static Pointer New() { return new EncodedMeshData; }

    IGenum GetDataObjectType() const override { return IG_MESH_ENCODED_DATA; }

    std::vector<unsigned char> m_Buffers;
protected:
    EncodedMeshData() = default;
    ~EncodedMeshData() override = default;
};

IGAME_NAMESPACE_END

#endif

