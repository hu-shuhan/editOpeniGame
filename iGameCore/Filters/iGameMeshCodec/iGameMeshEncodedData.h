#ifndef EncodedData_h
#define EncodedData_h

#include "iGameDataObject.h"

IGAME_NAMESPACE_BEGIN

class MeshEncodedData : public DataObject {
public:
    I_OBJECT(MeshEncodedData);
    static Pointer New() { return new MeshEncodedData; }

    IGenum GetDataObjectType() const override { return IG_MESH_ENCODED_DATA; }

    std::vector<unsigned char> m_Buffers;
protected:
    MeshEncodedData() = default;
    ~MeshEncodedData() override = default;
};

IGAME_NAMESPACE_END

#endif

