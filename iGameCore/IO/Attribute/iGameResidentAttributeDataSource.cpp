#include "Attribute/iGameResidentAttributeDataSource.h"

#include "DataCodec/Filter/Adapter/iGameBlockTreeAdapter.h"

IGAME_NAMESPACE_BEGIN

namespace {

std::string ChildPath(const std::string& parentPath, const int childIndex) {
    return parentPath.empty()
        ? "/" + std::to_string(childIndex)
        : parentPath + "/" + std::to_string(childIndex);
}

}

ResidentAttributeDataSource::ResidentAttributeDataSource(DataObject::Pointer rootObject)
    : m_rootObject(std::move(rootObject)) {}

DataObject::Pointer ResidentAttributeDataSource::RootObject() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_rootObject;
}

DataObject::Pointer ResidentAttributeDataSource::TargetObject(
    const AttributeDataTarget& target) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return FindObject(target.blockPath);
}

std::vector<AttributeDataDescriptor> ResidentAttributeDataSource::Attributes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AttributeDataDescriptor> descriptors;
    if (m_rootObject == nullptr) {
        return descriptors;
    }
    if (m_rootObject->HasSubDataObject()) {
        int childIndex = 0;
        for (auto iterator = m_rootObject->SubDataObjectIteratorBegin();
             iterator != m_rootObject->SubDataObjectIteratorEnd();
             ++iterator, ++childIndex) {
            CollectObject(iterator->second, ChildPath({}, childIndex), descriptors);
        }
    } else {
        CollectObject(m_rootObject, {}, descriptors);
    }
    return descriptors;
}

std::optional<AttributeDataDescriptor> ResidentAttributeDataSource::Attribute(
    const AttributeDataTarget& target) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto object = FindObject(target.blockPath);
    const auto attributes = object != nullptr && object->GetAttributeSet() != nullptr
        ? object->GetAttributeSet()->GetAllAttributes()
        : nullptr;
    if (attributes == nullptr ||
        target.sourceIndex >= static_cast<std::size_t>(attributes->GetNumberOfElements())) {
        return std::nullopt;
    }
    const auto& attribute = attributes->GetElement(static_cast<int>(target.sourceIndex));
    if (attribute.IsNone()) {
        return std::nullopt;
    }
    return AttributeDataDescriptor{
        .target = target,
        .name = attribute.pointer->GetName(),
        .role = attribute.type,
        .attachment = attribute.attachmentType,
        .componentCount = attribute.pointer->GetDimension(),
        .state = AttributeDataLoadState::Loaded,
        .nativeIndex = static_cast<int>(target.sourceIndex),
    };
}

AttributeDataLoadResult ResidentAttributeDataSource::PrepareAttribute(
    const AttributeDataTarget& target,
    const std::stop_token stopToken) {
    std::lock_guard<std::mutex> lock(m_mutex);
    AttributeDataLoadResult result;
    result.target = target;
    if (stopToken.stop_requested()) {
        return result;
    }
    result.object = FindObject(target.blockPath);
    if (result.object == nullptr || result.object->GetAttributeSet() == nullptr) {
        result.error = "attribute target object is unavailable";
        return result;
    }
    const auto attributes = result.object->GetAttributeSet()->GetAllAttributes();
    if (attributes == nullptr || target.sourceIndex >= static_cast<std::size_t>(attributes->GetNumberOfElements())) {
        result.error = "resident attribute target is outside the attribute set";
        return result;
    }
    const auto& attribute = attributes->GetElement(static_cast<int>(target.sourceIndex));
    if (attribute.IsNone()) {
        result.error = "resident attribute target is unavailable";
        return result;
    }
    result.success = true;
    result.nativeIndex = static_cast<int>(target.sourceIndex);
    return result;
}

AttributeDataLoadResult ResidentAttributeDataSource::CommitAttribute(
    const AttributeDataTarget& target) {
    return PrepareAttribute(target, {});
}

void ResidentAttributeDataSource::CollectObject(
    const DataObject::Pointer& object,
    const std::string& blockPath,
    std::vector<AttributeDataDescriptor>& descriptors) const {
    if (object == nullptr) {
        return;
    }
    if (object->HasSubDataObject()) {
        int childIndex = 0;
        for (auto iterator = object->SubDataObjectIteratorBegin();
             iterator != object->SubDataObjectIteratorEnd();
             ++iterator, ++childIndex) {
            CollectObject(iterator->second, ChildPath(blockPath, childIndex), descriptors);
        }
        return;
    }
    const auto attributes = object->GetAttributeSet() != nullptr
        ? object->GetAttributeSet()->GetAllAttributes()
        : nullptr;
    if (attributes == nullptr) {
        return;
    }
    for (int index = 0; index < attributes->GetNumberOfElements(); ++index) {
        const auto& attribute = attributes->GetElement(index);
        if (attribute.IsNone()) continue;
        descriptors.push_back(AttributeDataDescriptor{
            .target = AttributeDataTarget{
                .frameIndex = 0u,
                .blockPath = blockPath,
                .sourceIndex = static_cast<std::size_t>(index),
            },
            .name = attribute.pointer->GetName(),
            .role = attribute.type,
            .attachment = attribute.attachmentType,
            .componentCount = attribute.pointer->GetDimension(),
            .state = AttributeDataLoadState::Loaded,
            .nativeIndex = index,
        });
    }
}

DataObject::Pointer ResidentAttributeDataSource::FindObject(const std::string& blockPath) const {
    if (m_rootObject == nullptr || blockPath.empty()) {
        return m_rootObject;
    }
    iGameBlockTreeAdapter adapter(m_rootObject);
    const auto* leaf = adapter.FindLeaf(blockPath);
    return leaf != nullptr ? leaf->object : DataObject::Pointer{};
}

IGAME_NAMESPACE_END
