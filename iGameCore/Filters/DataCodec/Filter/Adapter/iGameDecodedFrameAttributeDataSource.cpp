#include "DataCodec/Filter/Adapter/iGameDecodedFrameAttributeDataSource.h"

#include "DataCodec/Filter/Adapter/iGameBlockTreeAdapter.h"
#include "DataCodec/Filter/Adapter/iGameFramePackageDecodeAssembly.h"
#include "DataCodec/Filter/Adapter/iGameFramePresentationBridge.h"

#include <algorithm>
#include <utility>

IGAME_NAMESPACE_BEGIN

namespace
{

IGenum ToNativeRole(const ::datacodec::AttrRole role) {
    switch (role) {
    case ::datacodec::AttrRole::Vector: return IG_VECTOR;
    case ::datacodec::AttrRole::Normal: return IG_NORMAL;
    case ::datacodec::AttrRole::TexCoord: return IG_TCOORD;
    case ::datacodec::AttrRole::Tensor: return IG_TENSOR;
    case ::datacodec::AttrRole::Color: return IG_RGB;
    case ::datacodec::AttrRole::Scalar:
    case ::datacodec::AttrRole::Unknown:
    default: return IG_SCALAR;
    }
}

IGenum ToNativeAttachment(const ::datacodec::AttrAttachment attachment) {
    return attachment == ::datacodec::AttrAttachment::Cell ? IG_CELL : IG_POINT;
}

std::string FirstError(const ::datacodec::DecodedFrameAttributeResult& result) {
    for (const auto& message : result.messages) {
        if (message.severity == ::datacodec::TelemetryMessageSeverity::Error) {
            return message.text;
        }
    }
    return result.messages.empty() ? std::string{} : result.messages.front().text;
}

}

DecodedFrameAttributeDataSource::DecodedFrameAttributeDataSource(
    ::datacodec::IDecodedFrameAttributeAccess::Pointer attributeAccess)
    : DecodedFrameAttributeDataSource(
          std::move(attributeAccess),
          std::make_shared<SharedState>()) {}

DecodedFrameAttributeDataSource::DecodedFrameAttributeDataSource(
    ::datacodec::IDecodedFrameAttributeAccess::Pointer attributeAccess,
    std::shared_ptr<SharedState> sharedState)
    : m_attributeAccess(std::move(attributeAccess)),
      m_rootObject(DataObjectFromDecodedFrame(Frame())),
      m_sharedState(std::move(sharedState)) {}

std::shared_ptr<DecodedFrameAttributeDataSource>
DecodedFrameAttributeDataSource::ForFrame(
    ::datacodec::DecodedFrameLease::Pointer frame) const {
    if (m_attributeAccess == nullptr || frame == nullptr) { return {}; }
    auto attributeAccess = m_attributeAccess->ForFrame(std::move(frame));
    return attributeAccess != nullptr
        ? std::shared_ptr<DecodedFrameAttributeDataSource>(
            new DecodedFrameAttributeDataSource(std::move(attributeAccess), m_sharedState))
        : std::shared_ptr<DecodedFrameAttributeDataSource>{};
}

std::shared_ptr<DecodedFrameAttributeDataSource>
DecodedFrameAttributeDataSource::ForFrameIndex(const std::uint32_t frameIndex) const {
    if (m_attributeAccess == nullptr) { return {}; }
    const auto lookup = m_attributeAccess->FindCachedFrame(frameIndex);
    return lookup.IsHit()
        ? ForFrame(lookup.value)
        : std::shared_ptr<DecodedFrameAttributeDataSource>{};
}

std::shared_ptr<IAttributeDataSource>
DecodedFrameAttributeDataSource::ForFrameObject(
    const DataObject::Pointer& frameObject) const {
    const auto frameIndex = DataCodecFrameIndex(frameObject.get());
    return frameIndex.has_value()
        ? std::static_pointer_cast<IAttributeDataSource>(ForFrameIndex(*frameIndex))
        : std::shared_ptr<IAttributeDataSource>{};
}

DataObject::Pointer DecodedFrameAttributeDataSource::RootObject() const {
    return m_rootObject;
}

DataObject::Pointer DecodedFrameAttributeDataSource::TargetObject(
    const AttributeDataTarget& target) const {
    const auto frame = Frame();
    if (m_rootObject == nullptr || frame == nullptr || target.frameIndex != frame->FrameIndex()) {
        return nullptr;
    }
    if (target.blockPath.empty()) { return m_rootObject; }
    iGameBlockTreeAdapter adapter(m_rootObject);
    const auto* leaf = adapter.FindLeaf(target.blockPath);
    return leaf != nullptr ? leaf->object : DataObject::Pointer{};
}

std::vector<AttributeDataDescriptor> DecodedFrameAttributeDataSource::Attributes() const {
    std::vector<AttributeDataDescriptor> descriptors;
    if (m_attributeAccess == nullptr || Frame() == nullptr) { return descriptors; }
    const auto codecDescriptors = m_attributeAccess->AvailableAttributes();
    descriptors.reserve(codecDescriptors.size());
    for (const auto& descriptor : codecDescriptors) {
        const auto target = ToAttributeDataTarget(descriptor.target);
        const auto nativeIndex = NativeAttributeIndex(target);
        descriptors.push_back(AttributeDataDescriptor{
            .target = target,
            .name = descriptor.metadata.name,
            .role = ToNativeRole(descriptor.metadata.type),
            .attachment = ToNativeAttachment(descriptor.metadata.attachmentType),
            .componentCount = descriptor.metadata.dimension,
            .state = descriptor.committed
                ? AttributeDataLoadState::Loaded
                : AttributeDataLoadState::Unloaded,
            .nativeIndex = descriptor.committed ? nativeIndex : -1,
        });
    }
    return descriptors;
}

std::optional<AttributeDataDescriptor> DecodedFrameAttributeDataSource::Attribute(
    const AttributeDataTarget& target) const {
    const auto descriptors = Attributes();
    const auto iterator = std::find_if(
        descriptors.begin(),
        descriptors.end(),
        [&target](const AttributeDataDescriptor& descriptor) {
            return descriptor.target == target;
        });
    return iterator == descriptors.end()
        ? std::optional<AttributeDataDescriptor>{}
        : std::optional<AttributeDataDescriptor>{*iterator};
}

AttributeDataLoadResult DecodedFrameAttributeDataSource::PrepareAttribute(
    const AttributeDataTarget& target,
    const std::stop_token stopToken) {
    AttributeDataLoadResult loadResult;
    loadResult.target = target;
    loadResult.object = TargetObject(target);
    if (m_attributeAccess == nullptr || Frame() == nullptr || loadResult.object == nullptr ||
        stopToken.stop_requested()) {
        return loadResult;
    }
    auto result = m_attributeAccess->RequestAttributes({
        .attributeTargets = {ToDataCodecTarget(target)},
        .mode = ::datacodec::AttributeDecodeRequestMode::DecodeToCache,
        .stopToken = stopToken,
    });
    loadResult.success = result.success;
    if (!loadResult.success && !result.cancelled) {
        loadResult.error = FirstError(result);
        if (loadResult.error.empty()) {
            loadResult.error = "failed to load playback attribute";
        }
    }
    return loadResult;
}

AttributeDataLoadResult DecodedFrameAttributeDataSource::CommitAttribute(
    const AttributeDataTarget& target) {
    AttributeDataLoadResult loadResult;
    loadResult.target = target;
    loadResult.object = TargetObject(target);
    if (m_attributeAccess == nullptr || Frame() == nullptr || loadResult.object == nullptr) {
        return loadResult;
    }
    const auto previousCount = loadResult.object->GetAttributeSet() != nullptr
        ? loadResult.object->GetAttributeSet()->GetNumberOfAttributes()
        : 0;
    auto result = m_attributeAccess->RequestAttributes({
        .attributeTargets = {ToDataCodecTarget(target)},
        .mode = ::datacodec::AttributeDecodeRequestMode::CommitCached,
    });
    const auto currentCount = loadResult.object->GetAttributeSet() != nullptr
        ? loadResult.object->GetAttributeSet()->GetNumberOfAttributes()
        : 0;
    if (result.success && currentCount > previousCount) {
        loadResult.nativeIndex = currentCount - 1;
        SetNativeAttributeIndex(target, loadResult.nativeIndex);
    } else if (result.success) {
        loadResult.nativeIndex = NativeAttributeIndex(target);
    }
    loadResult.success = result.success && loadResult.nativeIndex >= 0;
    if (!loadResult.success) {
        loadResult.error = FirstError(result);
        if (loadResult.error.empty()) {
            loadResult.error = "failed to commit playback attribute";
        }
    }
    return loadResult;
}

::datacodec::AttributeTarget DecodedFrameAttributeDataSource::ToDataCodecTarget(
    const AttributeDataTarget& target) {
    return {
        .frameIndex = target.frameIndex,
        .blockPath = target.blockPath,
        .attrIndex = target.sourceIndex,
    };
}

AttributeDataTarget DecodedFrameAttributeDataSource::ToAttributeDataTarget(
    const ::datacodec::AttributeTarget& target) {
    return {
        .frameIndex = target.frameIndex,
        .blockPath = target.blockPath,
        .sourceIndex = target.attrIndex,
    };
}

int DecodedFrameAttributeDataSource::NativeAttributeIndex(
    const AttributeDataTarget& target) const {
    if (m_sharedState == nullptr) { return -1; }
    std::lock_guard<std::mutex> lock(m_sharedState->mutex);
    const auto iterator = m_sharedState->nativeAttributeIndices.find({
        target.frameIndex,
        target.blockPath,
        target.sourceIndex,
    });
    return iterator == m_sharedState->nativeAttributeIndices.end() ? -1 : iterator->second;
}

void DecodedFrameAttributeDataSource::SetNativeAttributeIndex(
    const AttributeDataTarget& target,
    const int nativeIndex) {
    if (m_sharedState == nullptr) { return; }
    std::lock_guard<std::mutex> lock(m_sharedState->mutex);
    m_sharedState->nativeAttributeIndices[{
        target.frameIndex,
        target.blockPath,
        target.sourceIndex,
    }] = nativeIndex;
}

IGAME_NAMESPACE_END
