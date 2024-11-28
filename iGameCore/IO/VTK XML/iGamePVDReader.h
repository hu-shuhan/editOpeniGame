/**
 * @class   iGamePVDReader
 * @brief   iGamePVDReader's brief
 */

#pragma once

#include "XML/iGameXMLFileReader.h"
#include <iGameDataObject.h>

IGAME_NAMESPACE_BEGIN
class iGamePVDReader : public iGameXMLFileReader{
public:
    I_OBJECT(iGamePVDReader)

    static Pointer New(){return new iGamePVDReader;}

    bool Parsing() override;

    bool CreateDataObject() override;

protected:

    iGamePVDReader() = default;
    ~iGamePVDReader() = default;

protected:
    DataObject::Pointer  m_data_object{nullptr};
};

IGAME_NAMESPACE_END