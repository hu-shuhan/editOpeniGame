#ifndef DATACODEC_WORKFLOW_FRAME_FRAMETOPOLOGYOWNERSHIP_H
#define DATACODEC_WORKFLOW_FRAME_FRAMETOPOLOGYOWNERSHIP_H

#include "DataCodec/Runtime/Cache/DecodeCache/DecodedTopologyCache.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Storage/LeafPackage/LeafPackage.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageWireLayout.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace datacodec {

class FrameTopologyOwnership {
public:
    [[nodiscard]] static std::string MakeTopologyOwnerKey(
        const std::uint32_t frameIndex,
        const BlockPath& path) {
        return std::to_string(frameIndex) + ":" + path;
    }

    [[nodiscard]] static bool ContainsTopologyField(const LeafPackage& leafPackage) {
        return FindTopologyField(leafPackage) != nullptr;
    }

    [[nodiscard]] static const LeafPackageField* FindTopologyField(const LeafPackage& leafPackage) {
        for (const auto& field : leafPackage.fields) {
            if (field.type == FieldType::Topology) {
                return &field;
            }
        }
        return nullptr;
    }

    template<typename TFrameSource, typename TDecodeTopologyReference>
    static bool PrepareOwnerTopologyReference(
        TFrameSource& source,
        const std::uint32_t ownerFrameIndex,
        const BlockPath& path,
        const std::shared_ptr<DecodedTopologyReferenceCacheStore>& store,
        TDecodeTopologyReference&& decodeTopologyReference,
        std::string* error = nullptr) {
        if (store == nullptr) {
            return validation::AssignError(error, "topology reference store is missing");
        }

        const auto cacheKey = MakeTopologyOwnerKey(ownerFrameIndex, path);
        if (store->Contains(cacheKey)) {
            return true;
        }

        LeafPackage ownerLeafPackage;
        if (source.ReadLeafPackage(ownerFrameIndex, path, ownerLeafPackage, error) == nullptr) {
            return false;
        }
        if (!ContainsTopologyField(ownerLeafPackage)) {
            return validation::AssignError(error, "owner leaf package does not contain topology");
        }

        std::shared_ptr<DecodedTopologyCache> topologyCache;
        if (!decodeTopologyReference(ownerLeafPackage, topologyCache, error)) {
            return false;
        }
        if (topologyCache == nullptr || !topologyCache->complete) {
            return validation::AssignError(
                error,
                "topology reference decode did not produce a complete topology cache");
        }

        store->Put(cacheKey, std::move(topologyCache));
        return true;
    }

};

} // namespace datacodec

#endif

