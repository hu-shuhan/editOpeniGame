#ifndef iGameGhostVTKReader_h
#define iGameGhostVTKReader_h

#include "iGameDataObject.h"

#include <string>

IGAME_NAMESPACE_BEGIN

class GhostVTKReader {
public:
    static DataObject::Pointer ReadFile(const std::string& filePath);
};

IGAME_NAMESPACE_END

#endif