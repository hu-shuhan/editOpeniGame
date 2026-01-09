#ifndef iGameIGCMTimeSeriesWriter_h
#define iGameIGCMTimeSeriesWriter_h

#include "MeshCodec/iGameMeshEncoderFilter.h"
#include "iGameFileWriter.h"

#include <string>
#include <utility>
#include <vector>

namespace tinyxml2 {
    class XMLDocument;
    class XMLElement;
}

IGAME_NAMESPACE_BEGIN

class IGCMTimeSeriesWriter : public FileWriter {
public:
    I_OBJECT(IGCMTimeSeriesWriter);
    static Pointer New() { return new IGCMTimeSeriesWriter; }

    bool GenerateBuffers() override;

    void SetCodecControlParams(const CodecControlParams& params) {
        m_hasCodecParams = true;
        m_CodecParams = params;
    }

    std::vector<std::pair<std::string, std::string>> GetReport() const {
        return m_report;
    }

protected:
    IGCMTimeSeriesWriter() = default;
    ~IGCMTimeSeriesWriter() override = default;

private:
    bool m_hasCodecParams = false;
    CodecControlParams m_CodecParams;

    std::vector<std::pair<std::string, std::string>> m_report;

    static void CollectLeafParts(DataObject::Pointer obj, std::vector<DataObject::Pointer>& outParts);
    void AppendPartReport(int frameIndex, int partIndex, const std::string& displayName, const std::string& fileName,
                          const std::vector<std::pair<std::string, std::string>>& partReport);
};

IGAME_NAMESPACE_END

#endif
