#ifndef iGameIGDCReader_h
#define iGameIGDCReader_h

#include "DataCodec/Filter/Adapter/iGameDataCodecDataObjectBridge.h"
#include "DataCodec/API/Adapter/IDecodedFrameCache.h"
#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/API/Params/DecodedFrameCacheParams.h"
#include "DataCodec/API/Params/EncodedInputCacheParams.h"
#include "Attribute/iGameAttributeDataSource.h"
#include "iGameFileReader.h"

#include <cstdint>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

class IGDCReader : public FileReader {
public:
    I_OBJECT(IGDCReader);
    static Pointer New();

    bool Execute() override;
    bool Parsing() override;
    bool CreateDataObject() override;

    void SetCodecControlParams(const ::datacodec::DecodeControlParams& params);
    void SetDecodeControls(const ::datacodec::DataCodecDecodeConfigurationParams& definition);
    void SetDecodeTier(::datacodec::DataCodecDecodeTier tier);
    void SetDecodeOptions(const ::datacodec::DataCodecDecodeOptions& options);
    void SetRequestedFrameIndex(std::uint32_t frameIndex) { m_requestedFrameIndex = frameIndex; }
    void ClearRequestedFrameIndex() { m_requestedFrameIndex.reset(); }
    void SetSelectedFramePaths(std::vector<std::string> framePaths);
    void SetDecodedFrameCachePolicy(const ::datacodec::DecodedFrameCachePolicy& policy);
    void SetDecodedFrameCache(
        std::shared_ptr<::datacodec::IDecodedFrameCache> frameCache);
    void SetEncodedInputCachePolicy(const ::datacodec::EncodedInputCachePolicy& policy);
    void SetEncodedInputCache(
        std::shared_ptr<::datacodec::IEncodedInputCache> inputCache);
    void SetLoadAllAvailableAttributes(bool loadAllAvailableAttributes);
    void SetRunRecordSink(std::shared_ptr<::datacodec::IRunRecordSink> sink);

    [[nodiscard]] AttributeDataSourcePointer GetAttributeDataSource() const;

    const std::vector<::datacodec::TelemetryMessageRecord>& GetMessages() const;

protected:
    IGDCReader();
    ~IGDCReader() override;

private:
    struct State;

    DataObject::Pointer m_DecodedOutput;
    std::optional<std::uint32_t> m_requestedFrameIndex;
    std::unique_ptr<State> m_state;

    bool DecodeInput();
};

IGAME_NAMESPACE_END
#endif
