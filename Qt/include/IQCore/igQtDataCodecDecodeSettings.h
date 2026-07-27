#ifndef igQtDataCodecDecodeSettings_h
#define igQtDataCodecDecodeSettings_h

#include "IQCore/igQtExportModule.h"

#include <DataCodec/API/Params/CodecPerformancePresetParams.h>

#include <cstddef>

struct IG_QT_MODULE_EXPORT igQtDataCodecDecodeSettings {
    ::datacodec::DataCodecDecodeTier performanceTier{
        ::datacodec::DataCodecDecodeTier::Fast};
    bool decodeAttributesOnDemand{false};
    bool enableDecodedResultCache{false};
    std::size_t decodedResultCacheFrameLimit{3u};
};

class IG_QT_MODULE_EXPORT igQtDataCodecDecodeSettingsStore final {
public:
    [[nodiscard]] static igQtDataCodecDecodeSettings Load();
    static void Save(const igQtDataCodecDecodeSettings& settings);
    static void Apply(const igQtDataCodecDecodeSettings& settings);
};

#endif
