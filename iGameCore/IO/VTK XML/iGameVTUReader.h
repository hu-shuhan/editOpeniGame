/**
 * @class   iGameVTUReader
 * @brief   iGameVTUReader's brief
 */

#pragma once

#include "XML/iGameXMLFileReader.h"

IGAME_NAMESPACE_BEGIN
class iGameVTUReader : public iGameXMLFileReader {

public:
	I_OBJECT(iGameVTUReader);

	static Pointer New() { return new iGameVTUReader; }

	bool Parsing() override;
	bool CreateDataObject() override;

protected:

	iGameVTUReader() = default;
	~iGameVTUReader() = default;
    bool m_Header_8_byte_flag {false};

//	igIndex m_DataObjectType = IG_NONE;
};

IGAME_NAMESPACE_END
