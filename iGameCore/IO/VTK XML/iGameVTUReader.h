/**
 * @class   iGameVTUReader
 * @brief   iGameVTUReader's brief
 */

#pragma once

#include "XML/iGameXMLFileReader.h"
namespace tinyxml2{
    class XMLElement;
}
IGAME_NAMESPACE_BEGIN
class iGameVTUReader : public iGameXMLFileReader {

public:
	I_OBJECT(iGameVTUReader);

	static Pointer New() { return new iGameVTUReader; }

	bool Parsing() override;
	bool CreateDataObject() override;

protected:
    bool ReadPointData();
    bool ReadPointAttribute();
    bool ReadCellData();
    ArrayObject::Pointer ReadCellConnectivity();
    ArrayObject::Pointer ReadCellOffsets();
    ArrayObject::Pointer ReadCellTypes();
protected:

	iGameVTUReader() = default;
	~iGameVTUReader() = default;
    bool m_Header_8_byte_flag {false};

protected:
    tinyxml2::XMLElement* m_CurrentElem;

    int64_t m_PointsNum{-1};
    int64_t m_CellsNum {-1};
//	igIndex m_DataObjectType = IG_NONE;
};

IGAME_NAMESPACE_END
