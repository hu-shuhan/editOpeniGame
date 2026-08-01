#include "IGDC/iGameIGDCAttributeDataSource.h"

#include "DataCodec/Filter/Output/iGameDataCodecOutputBinding.h"
#include "IGDC/iGameDataCodecIOSettings.h"
#include "DataCodec/Filter/Adapter/iGameFileByteRangeIO.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <utility>

IGAME_NAMESPACE_BEGIN

namespace {

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

std::string FirstError(const DataCodecDataObjectDecodeResult& result) {
    for (const auto& message : result.messages) {
        if (message.severity == ::datacodec::TelemetryMessageSeverity::Error) {
            return message.text;
        }
    }
    return result.messages.empty() ? std::string{} : result.messages.front().text;
}

}

bool IGDCAttributeDataSource::Open(const std::string& filePath, std::string* error) {
    const auto definition = ::datacodec::MakeDecodeConfigurationParams(
        DataCodecIOSettings::GetDefaultDecodeOptions());
    auto result = m_session.Open({
        .inputReader = std::make_shared<iGameFileByteRangeReader>(
            std::filesystem::path(filePath)),
        .controlParams = &definition.controlParams,
        .executionOptions = &definition.execution,
        .configurationSource = &definition.source,
        .runRecordSink = MakeiGameDataCodecOutputRecordSink(
            {},
            {},
            true,
            definition.logging.enableConsoleLog),
    });
    if (!result.success || result.output == nullptr) {
        if (error != nullptr) {
            *error = FirstError(result);
            if (error->empty()) {
                *error = "failed to open IGC attribute data source";
            }
        }
        m_session.Reset();
        return false;
    }
    return true;
}

DataObject::Pointer IGDCAttributeDataSource::RootObject() const {
    return m_session.GetOutput();
}

DataObject::Pointer IGDCAttributeDataSource::TargetObject(
    const AttributeDataTarget& target) const {
    return m_session.OutputForTarget(ToDataCodecTarget(target));
}

std::vector<AttributeDataDescriptor> IGDCAttributeDataSource::Attributes() const {
    std::vector<AttributeDataDescriptor> descriptors;
    const auto codecDescriptors = m_session.AvailableAttributes();
    descriptors.reserve(codecDescriptors.size());
    for (const auto& descriptor : codecDescriptors) {
        descriptors.push_back(AttributeDataDescriptor{
            .target = ToAttributeDataTarget(descriptor.target),
            .name = descriptor.metadata.name,
            .role = ToNativeRole(descriptor.metadata.type),
            .attachment = ToNativeAttachment(descriptor.metadata.attachmentType),
            .componentCount = descriptor.metadata.dimension,
            .state = descriptor.committed
                ? AttributeDataLoadState::Loaded
                : AttributeDataLoadState::Unloaded,
            .nativeIndex = descriptor.committed
                ? m_session.NativeAttributeIndex(descriptor.target)
                : -1,
        });
    }
    return descriptors;
}

std::optional<AttributeDataDescriptor> IGDCAttributeDataSource::Attribute(
    const AttributeDataTarget& target) const {
    const auto codecTarget = ToDataCodecTarget(target);
    const auto descriptors = m_session.AvailableAttributes();
    const auto iterator = std::find_if(
        descriptors.begin(),
        descriptors.end(),
        [&codecTarget](const ::datacodec::DecodeAttributeDescriptor& descriptor) {
            return descriptor.target.frameIndex == codecTarget.frameIndex &&
                descriptor.target.blockPath == codecTarget.blockPath &&
                descriptor.target.attrIndex == codecTarget.attrIndex;
        });
    if (iterator == descriptors.end()) {
        return std::nullopt;
    }
    const auto nativeIndex = m_session.NativeAttributeIndex(codecTarget);
    return AttributeDataDescriptor{
        .target = target,
        .name = iterator->metadata.name,
        .role = ToNativeRole(iterator->metadata.type),
        .attachment = ToNativeAttachment(iterator->metadata.attachmentType),
        .componentCount = iterator->metadata.dimension,
        .state = iterator->committed
            ? AttributeDataLoadState::Loaded
            : AttributeDataLoadState::Unloaded,
        .nativeIndex = iterator->committed ? nativeIndex : -1,
    };
}

AttributeDataLoadResult IGDCAttributeDataSource::PrepareAttribute(
    const AttributeDataTarget& target,
    const std::stop_token stopToken) {
    const auto codecTarget = ToDataCodecTarget(target);
    DataCodecDataObjectAttributeRequest request;
    request.attributeTargets.push_back(codecTarget);
    request.mode = ::datacodec::AttributeDecodeRequestMode::DecodeToCache;
    request.stopToken = stopToken;
    auto result = m_session.RequestAttributes(request);

    AttributeDataLoadResult loadResult;
    loadResult.target = target;
    loadResult.object = m_session.OutputForTarget(codecTarget);
    loadResult.success = result.success && loadResult.object != nullptr;
    if (!loadResult.success) {
        loadResult.error = FirstError(result);
        if (loadResult.error.empty()) {
            loadResult.error = "failed to load IGC attribute";
        }
    }
    return loadResult;
}

AttributeDataLoadResult IGDCAttributeDataSource::CommitAttribute(
    const AttributeDataTarget& target) {
    const auto codecTarget = ToDataCodecTarget(target);
    DataCodecDataObjectAttributeRequest request;
    request.attributeTargets.push_back(codecTarget);
    request.mode = ::datacodec::AttributeDecodeRequestMode::CommitCached;
    auto result = m_session.RequestAttributes(request);

    AttributeDataLoadResult loadResult;
    loadResult.target = target;
    loadResult.object = m_session.OutputForTarget(codecTarget);
    loadResult.nativeIndex = m_session.NativeAttributeIndex(codecTarget);
    loadResult.success = result.success &&
        loadResult.object != nullptr &&
        loadResult.nativeIndex >= 0;
    if (!loadResult.success) {
        loadResult.error = FirstError(result);
        if (loadResult.error.empty()) {
            loadResult.error = "failed to commit IGC attribute";
        }
    }
    return loadResult;
}

::datacodec::AttributeTarget IGDCAttributeDataSource::ToDataCodecTarget(
    const AttributeDataTarget& target) {
    return ::datacodec::AttributeTarget{
        .frameIndex = target.frameIndex,
        .blockPath = target.blockPath,
        .attrIndex = target.sourceIndex,
    };
}

AttributeDataTarget IGDCAttributeDataSource::ToAttributeDataTarget(
    const ::datacodec::AttributeTarget& target) {
    return AttributeDataTarget{
        .frameIndex = target.frameIndex,
        .blockPath = target.blockPath,
        .sourceIndex = target.attrIndex,
    };
}

IGAME_NAMESPACE_END
