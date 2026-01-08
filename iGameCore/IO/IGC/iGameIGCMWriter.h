#ifndef iGameIGCMWriter_h
#define iGameIGCMWriter_h

#include "MeshCodec/iGameMeshEncoderFilter.h"
#include "iGameFileWriter.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace tinyxml2 {
    class XMLDocument;
    class XMLElement;
}

IGAME_NAMESPACE_BEGIN

class IGCMWriter : public FileWriter {
public:
    I_OBJECT(IGCMWriter);
    static Pointer New() { return new IGCMWriter; }

    bool GenerateBuffers() override;

    void SetCodecControlParams(const CodecControlParams& params) {
        m_hasCodecParams = true;
        m_CodecParams = params;
    }

    std::vector<std::pair<std::string, std::string>> GetReport() const {
        return m_report;
    }

protected:
    IGCMWriter() = default;
    ~IGCMWriter() override = default;

private:
    bool m_hasCodecParams = false;
    CodecControlParams m_CodecParams;

    std::vector<std::pair<std::string, std::string>> m_report;

    bool WriteSubBlocksToDiskAndManifest(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parentElem,
                                         DataObject::Pointer obj, const std::filesystem::path& outputDir,
                                         const std::string& baseName, int leafTotal, int& leafPreorderIndex);

    void AppendLeafReport(const std::string& displayName, const std::string& fileName,
                          const std::vector<std::pair<std::string, std::string>>& leafReport);
};

IGAME_NAMESPACE_END

#endif
