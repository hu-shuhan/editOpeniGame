/**
 * @class   iGameVTMReader
 * @brief   iGameVTMReader's brief
 */

#pragma once

#include "XML/iGameXMLFileReader.h"
#include <iGameDataObject.h>

IGAME_NAMESPACE_BEGIN

class iGameVTMReader : public iGameXMLFileReader{
public:
    I_OBJECT(iGameVTMReader)

    static Pointer New(){return new iGameVTMReader;}

    bool Parsing() override;

protected:

    iGameVTMReader() = default;

public:
    bool CreateDataObject() override;

protected:
    ~iGameVTMReader() override = default;

protected:
    DataObject::Pointer parseData;

};
IGAME_NAMESPACE_END