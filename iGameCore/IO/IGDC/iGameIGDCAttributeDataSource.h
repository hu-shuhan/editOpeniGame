#ifndef iGameIGDCAttributeDataSource_h
#define iGameIGDCAttributeDataSource_h

#include "Attribute/iGameAttributeDataSource.h"
#include "DataCodec/Filter/Adapter/iGameDataCodecDataObjectBridge.h"

#include <string>

IGAME_NAMESPACE_BEGIN

class IGDCAttributeDataSource final : public IAttributeDataSource {
public:
    bool Open(const std::string& filePath, std::string* error = nullptr);

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
    [[nodiscard]] static ::datacodec::AttributeTarget ToDataCodecTarget(
        const AttributeDataTarget& target);
    [[nodiscard]] static AttributeDataTarget ToAttributeDataTarget(
        const ::datacodec::AttributeTarget& target);

    DataCodecDataObjectDecodeSession m_session;
};

IGAME_NAMESPACE_END

#endif
