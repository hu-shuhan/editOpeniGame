#ifndef iGameAttributeDataSource_h
#define iGameAttributeDataSource_h

#include "iGameDataObject.h"
#include "iGameType.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stop_token>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

struct AttributeDataTarget {
    std::uint32_t frameIndex{0u};
    std::string blockPath;
    std::size_t sourceIndex{0u};

    bool operator==(const AttributeDataTarget&) const = default;
};

enum class AttributeDataLoadState : std::uint8_t {
    Unloaded = 0u,
    Loading = 1u,
    Loaded = 2u,
    Failed = 3u,
};

struct AttributeDataDescriptor {
    AttributeDataTarget target;
    std::string name;
    IGenum role{IG_SCALAR};
    IGenum attachment{IG_POINT};
    int componentCount{1};
    AttributeDataLoadState state{AttributeDataLoadState::Unloaded};
    int nativeIndex{-1};
};

struct AttributeDataLoadResult {
    bool success{false};
    AttributeDataTarget target;
    DataObject::Pointer object;
    int nativeIndex{-1};
    std::string error;
};

class IAttributeDataSource {
public:
    virtual ~IAttributeDataSource() = default;

    [[nodiscard]] virtual std::shared_ptr<IAttributeDataSource> ForFrameObject(
        const DataObject::Pointer&) const {
        return {};
    }

    [[nodiscard]] virtual DataObject::Pointer RootObject() const = 0;
    [[nodiscard]] virtual DataObject::Pointer TargetObject(
        const AttributeDataTarget& target) const = 0;
    [[nodiscard]] virtual std::vector<AttributeDataDescriptor> Attributes() const = 0;
    [[nodiscard]] virtual std::optional<AttributeDataDescriptor> Attribute(
        const AttributeDataTarget& target) const = 0;
    [[nodiscard]] virtual AttributeDataLoadResult PrepareAttribute(
        const AttributeDataTarget& target,
        std::stop_token stopToken = {}) = 0;
    [[nodiscard]] virtual AttributeDataLoadResult CommitAttribute(
        const AttributeDataTarget& target) = 0;
};

using AttributeDataSourcePointer = std::shared_ptr<IAttributeDataSource>;

IGAME_NAMESPACE_END

#endif
