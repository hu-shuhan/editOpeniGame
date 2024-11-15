/**
 * @class   XMLFileReader2
 * @brief   XMLFileReader2's brief
 */

#pragma once

#include "iGameFileReader.h"

IGAME_NAMESPACE_BEGIN
//class XMLHelper : public Object{
//public:
//    I_OBJECT(XMLHelper)
//    static Pointer New(){return new XMLHelper;}
//
//    XMLHelper() = default;
//};

class XMLFileReader2 : public FileReader{
public:
    I_OBJECT(XMLFileReader2)
    static Pointer New() { return new XMLFileReader2; }

    virtual bool InitXMLStructure();

    bool Execute() override;

    bool Parsing() override;
protected:
    bool ReadRootNode();
//    bool Read

protected:
//    XMLHelper::Pointer m_XmlHelper;
protected:
    XMLFileReader2() = default;
    ~XMLFileReader2() override = default;
};


IGAME_NAMESPACE_END
