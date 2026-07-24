#ifndef iGameDecodedFrameAttributeDataSource_h
#define iGameDecodedFrameAttributeDataSource_h

#include "Attribute/iGameAttributeDataSource.h"
#include "DataCodec/API/Adapter/IDecodedFrameAttributeAccess.h"

#include <map>
#include <memory>
#include <mutex>
#include <tuple>

IGAME_NAMESPACE_BEGIN

class DecodedFrameAttributeDataSource final : public IAttributeDataSource {
public:
    explicit DecodedFrameAttributeDataSource(
        ::datacodec::IDecodedFrameAttributeAccess::Pointer attributeAccess);

    [[nodiscard]] std::shared_ptr<DecodedFrameAttributeDataSource> ForFrame(
        ::datacodec::DecodedFrameLease::Pointer frame) const;
    [[nodiscard]] std::shared_ptr<DecodedFrameAttributeDataSource> ForFrameIndex(
        std::uint32_t frameIndex) const;
    [[nodiscard]] std::shared_ptr<IAttributeDataSource> ForFrameObject(
        const DataObject::Pointer& frameObject) const override;
    [[nodiscard]] ::datacodec::DecodedFrameLease::Pointer Frame() const noexcept {
        return m_attributeAccess != nullptr ? m_attributeAccess->Frame() : nullptr;
    }

    [[nodiscard]] DataObject::Pointer RootObject() const override;
    [[nodiscard]] DataObject::Pointer TargetObject(
        const AttributeDataTarget& target) const override;
    [[nodiscard]] std::vector<AttributeDataDescriptor> Attributes() const override;
    [[nodiscard]] std::optional<AttributeDataDescriptor> Attribute(
        const AttributeDataTarget& target) const override;
    [[nodiscard]] AttributeDataLoadResult PrepareAttribute(
        const AttributeDataTarget& target,
        std::stop_token stopToken = {}) override;
    [[nodiscard]] AttributeDataLoadResult CommitAttribute(
        const AttributeDataTarget& target) override;

private:
    using NativeAttributeKey = std::tuple<std::uint32_t, std::string, std::size_t>;

    struct SharedState {
        mutable std::mutex mutex;
        std::map<NativeAttributeKey, int> nativeAttributeIndices;
    };

    DecodedFrameAttributeDataSource(
        ::datacodec::IDecodedFrameAttributeAccess::Pointer attributeAccess,
        std::shared_ptr<SharedState> sharedState);

    [[nodiscard]] static ::datacodec::AttributeTarget ToDataCodecTarget(
        const AttributeDataTarget& target);
    [[nodiscard]] static AttributeDataTarget ToAttributeDataTarget(
        const ::datacodec::AttributeTarget& target);
    [[nodiscard]] int NativeAttributeIndex(const AttributeDataTarget& target) const;
    void SetNativeAttributeIndex(const AttributeDataTarget& target, int nativeIndex);

    ::datacodec::IDecodedFrameAttributeAccess::Pointer m_attributeAccess;
    DataObject::Pointer m_rootObject;
    std::shared_ptr<SharedState> m_sharedState;
};

IGAME_NAMESPACE_END

#endif
