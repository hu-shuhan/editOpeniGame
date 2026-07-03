#ifndef iGameFileIO_h
#define iGameFileIO_h

#include "MeshCodec/Utils/iGameMeshCodecParams.h"
#include "iGameDataObject.h"
#include "iGameObject.h"
#include <cstddef>

IGAME_NAMESPACE_BEGIN
class FileIO : public Object {
public:
    I_OBJECT(FileIO);

    enum FileType {
        NONE = 0,
        VTK,
        IGC,
        OBJ,
        OFF,
        MESH,
        STL,
        PLY,
        STEP,
        IGES,
        PVD,
        VTU,
        VTP,
        VTM,
        VTS,
        EX2,
        CGNS,
        INP,
        ODB,
        CAS,
        BDF,
        IGCM,
        FILETYPE_COUNT
    };

    static DataObject::Pointer ReadFile(const std::string& file_name);
    static DataObject::Pointer ReadVTKFromMemory(const void* data, size_t size);
    static DataObject::Pointer ReadVTUFromMemory(const void* data, size_t size);
    static DataObject::Pointer ReadVTPFromMemory(const void* data, size_t size);
    static DataObject::Pointer ReadIGCFromMemory(const void* data, size_t size);
    static bool WriteFile(const std::string& file_name, DataObject::Pointer);
    static IGenum GetFileType(const std::string& file_name);
    static std::string GetFileTypeAsString(IGenum type);
};

IGAME_NAMESPACE_END
#endif
