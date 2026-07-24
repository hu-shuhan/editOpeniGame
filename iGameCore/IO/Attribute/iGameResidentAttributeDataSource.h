#ifndef iGameResidentAttributeDataSource_h
#define iGameResidentAttributeDataSource_h

#include "Attribute/iGameAttributeDataSource.h"

#include <mutex>

IGAME_NAMESPACE_BEGIN

class ResidentAttributeDataSource final : public IAttributeDataSource {
public:
    explicit ResidentAttributeDataSource(DataObject::Pointer rootObject);

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
    void CollectObject(
        const DataObject::Pointer& object,
        const std::string& blockPath,
        std::vector<AttributeDataDescriptor>& descriptors) const;
    [[nodiscard]] DataObject::Pointer FindObject(const std::string& blockPath) const;

    mutable std::mutex m_mutex;
    DataObject::Pointer m_rootObject;
};

IGAME_NAMESPACE_END

#endif
