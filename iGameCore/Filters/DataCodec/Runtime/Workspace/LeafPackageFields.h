#ifndef DATACODEC_RUNTIME_WORKSPACE_LEAFPACKAGEFIELDS_H
#define DATACODEC_RUNTIME_WORKSPACE_LEAFPACKAGEFIELDS_H

#include "DataCodec/Storage/LeafPackage/LeafPackage.h"

#include <array>
#include <cstddef>
#include <vector>
namespace datacodec {

class LeafPackageFields {
public:
    explicit LeafPackageFields(const LeafPackage* leafPackage = nullptr) { Reset(leafPackage); }

    void Reset(const LeafPackage* leafPackage) {
        Clear();
        m_leafPackage = leafPackage;
        if (m_leafPackage == nullptr) {
            return;
        }
        for (const auto& field : m_leafPackage->fields) {
            const auto index = FieldTypeIndex(field.type);
            if (index < m_fieldsByType.size()) {
                m_fieldsByType[index].push_back(&field);
            }
        }
    }

    void Clear() noexcept {
        m_leafPackage = nullptr;
        for (auto& bucket : m_fieldsByType) {
            bucket.clear();
        }
    }

    [[nodiscard]] bool HasField(const FieldType type, const std::size_t ordinal = 0u) const {
        return FindField(type, ordinal) != nullptr;
    }

    [[nodiscard]] const LeafPackage::Field* FindField(
        const FieldType type,
        const std::size_t ordinal = 0u) const {
        const auto index = FieldTypeIndex(type);
        if (index >= m_fieldsByType.size()) {
            return nullptr;
        }
        const auto& bucket = m_fieldsByType[index];
        return ordinal < bucket.size() ? bucket[ordinal] : nullptr;
    }

    [[nodiscard]] const LeafPackage::Field* FieldFor(
        const FieldType type,
        const std::size_t ordinal = 0u) const {
        return FindField(type, ordinal);
    }

private:
    static constexpr std::size_t kFieldTypeSlotCount = 5u;

    [[nodiscard]] static constexpr std::size_t FieldTypeIndex(const FieldType type) noexcept {
        switch (type) {
            case FieldType::Params:
                return 0u;
            case FieldType::Geometry:
                return 1u;
            case FieldType::Topology:
                return 2u;
            case FieldType::Attribute:
                return 3u;
            case FieldType::TemporalMetadata:
                return 4u;
        }
        return kFieldTypeSlotCount;
    }

    const LeafPackage* m_leafPackage{nullptr};
    std::array<std::vector<const LeafPackage::Field*>, kFieldTypeSlotCount> m_fieldsByType{};
};

} // namespace datacodec

#endif
