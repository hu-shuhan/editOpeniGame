#ifndef iGameIGCMReader_h
#define iGameIGCMReader_h

#include "XML/iGameXMLFileReader.h"
#include "iGameDataObject.h"

#include <filesystem>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class IGCMReader : public iGameXMLFileReader {
public:
    I_OBJECT(IGCMReader);
    static Pointer New() { return new IGCMReader; }

    bool Parsing() override;
    bool CreateDataObject() override;

protected:
    IGCMReader() = default;
    ~IGCMReader() override = default;

private:
    DataObject::Pointer parseData;
    int m_dataSetCount = 0;

    bool ParseMultiBlock(const std::filesystem::path& manifestDir);
    bool ParseTimeSeries(const std::filesystem::path& manifestDir);

    bool ParseChildren(tinyxml2::XMLElement* parentElem, const std::filesystem::path& manifestDir,
                       std::vector<DataObject::Pointer>& outNodes);
    bool ParseElement(tinyxml2::XMLElement* elem, const std::filesystem::path& manifestDir, DataObject::Pointer& outObj);
};

IGAME_NAMESPACE_END

#endif
