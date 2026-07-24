#ifndef iGameFramePresentationBridge_h
#define iGameFramePresentationBridge_h

#include "DataCodec/Storage/FramePackage/FramePackageFormat.h"
#include "iGameDataObject.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

IGAME_NAMESPACE_BEGIN

struct DataCodecFramePresentationResult {
    bool recognized{false};
    bool drawableReady{false};
    std::size_t reusedTopologyLeafCount{0u};
};

[[nodiscard]] bool PrepareDataCodecDecodedLeaf(
    const DataObject::Pointer& output,
    const ::datacodec::FramePackageLeafRecord& leaf,
    std::uint32_t frameIndex,
    std::string* error = nullptr);

[[nodiscard]] DataCodecFramePresentationResult PrepareDataCodecFramePresentation(
    DataObject* currentFrame,
    DataObject* nextFrame);

[[nodiscard]] bool IsDataCodecPreparedFrame(const DataObject* object);
[[nodiscard]] std::optional<std::uint32_t> DataCodecFrameIndex(const DataObject* object);

IGAME_NAMESPACE_END

#endif
