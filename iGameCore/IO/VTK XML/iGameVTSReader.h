/**
 * @class   iGameVTSReader
 * @brief   iGameVTSReader's brief
 */
#ifndef iGameVTSReader_h
#define iGameVTSReader_h


#include "XML/iGameXMLFileReader.h"

IGAME_NAMESPACE_BEGIN
class iGameVTSReader : public iGameXMLFileReader{
public:
    I_OBJECT(iGameVTSReader)

    static Pointer New(){return new iGameVTSReader;}

    bool Parsing() override;

    bool CreateDataObject() override;

protected:
    iGameVTSReader() = default;
    ~iGameVTSReader() = default;

protected:
    bool m_Header_8_byte_flag {false};
};

IGAME_NAMESPACE_END
#endif