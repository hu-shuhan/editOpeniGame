#ifndef iGameFileIO_h
#define iGameFileIO_h

#include "iGameObject.h"
#include "iGameDataObject.h"
#include "iGameFileReader.h"
#include "iGameFileWriter.h"
IGAME_NAMESPACE_BEGIN
class FileIO : public Object {
public:
	I_OBJECT(FileIO);
	
	enum FileType {
		NONE = 0,
		VTK,
		OBJ,
		OFF,
		MESH,
		STL,
		PLY,
		STEP,
		IGES,
		PVD,
        VTU,
        VTM,
        VTS,
        EX2,
		CGNS,
		INP,
		ODB,
		CAS,
		FILETYPE_COUNT
	};

    static DataObject::Pointer ReadFile(const std::string &file_name);  
	static bool WriteFile(const std::string& file_name, DataObject::Pointer);
	static IGenum GetFileType(const std::string& file_name);
	static std::string GetFileTypeAsString(IGenum type);

};

IGAME_NAMESPACE_END
#endif