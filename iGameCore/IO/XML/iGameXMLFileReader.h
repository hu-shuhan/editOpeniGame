/**
 * @class   iGameXMLFileReader
 * @brief   Currently, iGameXMLFileReader only support ASCII encoded files,
 *          the file which mixed with binary and ASCII is not supported.
 */

#pragma once
#include "iGameFilter.h"
#include "iGameDataCollection.h"


namespace tinyxml2{
    class XMLElement;
    class XMLDocument;
}

IGAME_NAMESPACE_BEGIN

class iGameXMLFileReader : public Filter {
public:
    I_OBJECT(iGameXMLFileReader)
public:
    void SetFilePath(const std::string& filePath);
    void SetMemoryBuffer(const void* data, size_t size);

    bool Open();
    virtual bool CreateDataObject();
    virtual bool Parsing() = 0;

    bool Execute() override;

protected:
    std::string m_FilePath;
    std::string m_FileName;
    const char* m_MemoryBuffer{nullptr};
    size_t m_MemoryBufferSize{0};
    bool m_UseMemoryBuffer{false};

    DataObject::Pointer m_Output;
    // tinyxml stuff
    tinyxml2::XMLElement* root{nullptr};
    tinyxml2::XMLDocument* doc{nullptr};

protected:
    tinyxml2::XMLElement* FindTargetItem(tinyxml2::XMLElement *root, const char *itemName);

    tinyxml2::XMLElement* FindTargetAttributeItem(tinyxml2::XMLElement* root, const char* itemName,
                                          const char* attributeName, const char* attributeData);

    DataCollection m_Data;
protected:
    iGameXMLFileReader();
    ~iGameXMLFileReader() override;


};


IGAME_NAMESPACE_END
