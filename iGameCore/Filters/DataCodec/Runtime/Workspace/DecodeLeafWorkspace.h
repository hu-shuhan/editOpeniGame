#ifndef DATACODEC_RUNTIME_WORKSPACE_DECODELEAFWORKSPACE_H
#define DATACODEC_RUNTIME_WORKSPACE_DECODELEAFWORKSPACE_H

#include "DataCodec/Common/Views/TopologyViews.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedAttributeCacheSet.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedGeometryCache.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedTopologyCache.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Runtime/Failure/FailureCleanable.h"
#include "DataCodec/Codec/NumericArray/NumericArraySource.h"
#include "DataCodec/Storage/LeafPackage/LeafPackage.h"
#include "DataCodec/Storage/ByteIO/Window/WindowRuntimeParams.h"
#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/API/Params/CodecStorageParams.h"
#include "DataCodec/Validation/Policy/CodecValidationPolicy.h"
#include "DataCodec/Runtime/Workspace/LeafPackageFields.h"

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <stop_token>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

struct DecodeLeafWorkspace : IFailureCleanable {
    LeafPackageFields packageFields;
    DecodedGeometryCache geometry;
    DecodedAttributeCacheSet attributes;
    std::shared_ptr<DecodedTopologyCache> topology;
    bool topologyBorrowed{false};

    void ConfigureCacheResources(
        const std::size_t accessWindowBytes = kDefaultDecodeAccessWindowBytes,
        const std::uint64_t activeWindowBytes = kDefaultDecodeActiveWindowBytes,
        const std::size_t scratchRetainedBlockCount = 16u,
        const std::size_t scratchRetainedBlockBytes = 64u * 1024u * 1024u,
        const std::uint64_t scratchRetainedTotalBytes = 1024ull * 1024ull * 1024ull,
        const std::uint64_t topologyBufferBudgetBytes = 4u * 1024u * 1024u,
        const std::uint64_t remapScratchBudgetBytes = 256u * 1024u * 1024u) {
        m_cacheResources.Configure(
            accessWindowBytes,
            activeWindowBytes,
            scratchRetainedBlockCount,
            scratchRetainedBlockBytes,
            scratchRetainedTotalBytes,
            topologyBufferBudgetBytes,
            remapScratchBudgetBytes);
    }

    void InheritCacheResourceConfigFrom(const DecodeLeafWorkspace& parent) {
        const auto& budget = parent.ResourceBudget();
        ConfigureCacheResources(
            parent.AccessWindowBytes(),
            parent.ActiveWindowBytes(),
            budget.ScratchRetainedBlockCount(),
            budget.ScratchRetainedBlockBytes(),
            budget.ScratchRetainedTotalBytes(),
            budget.TopologyBufferBudgetBytes(),
            budget.RemapScratchQuotaBytes());
    }

    [[nodiscard]] const CodecStorageParams& StorageParams() const noexcept { return m_storageParams; }
    void SetStorageParams(CodecStorageParams storageParams) noexcept {
        m_storageParams = std::move(storageParams);
        m_committedAttributes.assign(m_storageParams.attrParams.size(), 0u);
    }
    [[nodiscard]] const CacheResources& CacheResourcesRef() const noexcept { return m_cacheResources; }
    [[nodiscard]] CacheResources& CacheResourcesRef() noexcept { return m_cacheResources; }
    [[nodiscard]] const CodecValidationPolicy& ValidationPolicy() const noexcept { return m_validationPolicy; }
    void SetValidationPolicy(CodecValidationPolicy policy) noexcept { m_validationPolicy = policy; }
    [[nodiscard]] const ResourceBudgetControlParams& ResourceBudget() const noexcept { return m_resourceBudget; }
    void SetResourceBudget(ResourceBudgetControlParams params) {
        m_resourceBudget = std::move(params);
        m_byteStoreSession.ConfigureResidentLimit(m_resourceBudget.ResidentLimitBytes());
    }
    [[nodiscard]] bytestore::ByteStoreSession& ByteStoreSessionRef() noexcept { return m_byteStoreSession; }
    [[nodiscard]] const bytestore::ByteStoreSession& ByteStoreSessionRef() const noexcept { return m_byteStoreSession; }
    [[nodiscard]] ScratchByteBufferPool& ScratchBytePool() noexcept { return m_cacheResources.scratchBytePool; }
    [[nodiscard]] const ScratchByteBufferPool& ScratchBytePool() const noexcept { return m_cacheResources.scratchBytePool; }
    [[nodiscard]] std::stop_token StopToken() const noexcept { return m_stopSource.get_token(); }
    [[nodiscard]] bool StopRequested() const noexcept {
        return m_stopSource.stop_requested() || m_externalStopToken.stop_requested();
    }
    void RequestStop() noexcept { m_stopSource.request_stop(); }
    void SetExternalStopToken(const std::stop_token stopToken) {
        m_externalStopToken = stopToken;
        ResetStopSource();
    }
    void ClearExternalStopToken() noexcept {
        m_externalStopCallback.reset();
        m_externalStopToken = {};
    }
    [[nodiscard]] window::WindowBudget& WindowBudgetRef() noexcept { return m_cacheResources.windowBudget; }
    [[nodiscard]] const window::WindowBudget& WindowBudgetRef() const noexcept { return m_cacheResources.windowBudget; }
    [[nodiscard]] std::size_t AccessWindowBytes() const noexcept { return m_cacheResources.accessWindowBytes; }
    [[nodiscard]] std::uint64_t ActiveWindowBytes() const noexcept { return m_cacheResources.activeWindowBytes; }
    [[nodiscard]] bool MatchesLeafPackage(const LeafPackage* leafPackage) const noexcept {
        return m_leafPackage == leafPackage && leafPackage != nullptr;
    }

    [[nodiscard]] bool PreparedAttributePayload(
        const LeafPackageField* field,
        std::shared_ptr<bytestore::IByteSource>& owner,
        std::span<const std::uint8_t>& bytes) const noexcept {
        if (field == nullptr || field != m_attributePayloadField || m_attributePayloadOwner == nullptr) {
            owner.reset();
            bytes = {};
            return false;
        }
        owner = m_attributePayloadOwner;
        bytes = m_attributePayloadBytes;
        return true;
    }

    void SetPreparedAttributePayload(
        const LeafPackageField* field,
        std::shared_ptr<bytestore::IByteSource> owner,
        const std::span<const std::uint8_t> bytes) noexcept {
        m_attributePayloadField = field;
        m_attributePayloadOwner = std::move(owner);
        m_attributePayloadBytes = bytes;
    }

    void PrepareSupplementRun(
        const CodecValidationPolicy validationPolicy,
        const ResourceBudgetControlParams resourceBudget) {
        m_failureCleanupCompleted.store(false, std::memory_order_release);
        ResetStopSource();
        m_validationPolicy = validationPolicy;
        m_resourceBudget = resourceBudget;
        m_byteStoreSession.ConfigureResidentLimit(m_resourceBudget.ResidentLimitBytes());
        ConfigureCacheResources(
            m_resourceBudget.AccessWindowBytes(),
            m_resourceBudget.ActiveWindowBytes(),
            m_resourceBudget.ScratchRetainedBlockCount(),
            m_resourceBudget.ScratchRetainedBlockBytes(),
            m_resourceBudget.ScratchRetainedTotalBytes(),
            m_resourceBudget.TopologyBufferBudgetBytes(),
            m_resourceBudget.RemapScratchQuotaBytes());
    }

    [[nodiscard]] bool HasField(const FieldType type, const std::size_t ordinal = 0u) const {
        return packageFields.HasField(type, ordinal);
    }

    [[nodiscard]] bool AttributeCommitted(const std::size_t attrIndex) const noexcept {
        return attrIndex < m_committedAttributes.size() && m_committedAttributes[attrIndex] != 0u;
    }

    void MarkAttributesCommitted(const std::span<const std::size_t> attrIndices) noexcept {
        for (const auto attrIndex : attrIndices) {
            if (attrIndex < m_committedAttributes.size()) {
                m_committedAttributes[attrIndex] = 1u;
            }
        }
    }

    [[nodiscard]] DecodedTopologyCache& MutableTopology() {
        topologyBorrowed = false;
        if (topology == nullptr) {
            topology = std::make_shared<DecodedTopologyCache>();
        }
        return *topology;
    }

    void BindTopologyReference(std::shared_ptr<DecodedTopologyCache> reference) {
        topology = std::move(reference);
        topologyBorrowed = true;
    }

    [[nodiscard]] const DecodedTopologyCache* TopologyForCommit() const noexcept {
        return topology != nullptr && topology->complete ? topology.get() : nullptr;
    }

    void Reset(const LeafPackage* leafPackage = nullptr) {
        m_failureCleanupCompleted.store(false, std::memory_order_release);
        m_attributePayloadBytes = {};
        m_attributePayloadOwner.reset();
        m_attributePayloadField = nullptr;
        ResetStopSource();
        m_storageParams = {};
        m_committedAttributes.clear();
        m_validationPolicy = {};
        m_resourceBudget = {};
        m_cacheResources.Clear();
        m_byteStoreSession.Reset();
        packageFields.Reset(leafPackage);
        geometry.Release();
        attributes.Reset();
        if (topology != nullptr && !topologyBorrowed) {
            topology->Release();
        }
        topology.reset();
        topologyBorrowed = false;
        m_leafPackage = leafPackage;
    }

    void CleanupOnFailure() noexcept override {
        if (m_failureCleanupCompleted.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        RequestStop();
        m_storageParams = {};
        m_committedAttributes.clear();
        m_validationPolicy = {};
        m_resourceBudget = {};
        m_cacheResources.Clear();
        m_byteStoreSession.ReleaseAll();
        m_attributePayloadBytes = {};
        m_attributePayloadOwner.reset();
        m_attributePayloadField = nullptr;
        packageFields.Clear();
        geometry.Release();
        attributes.Reset();
        if (topology != nullptr && !topologyBorrowed) {
            topology->Release();
        }
        topology.reset();
        topologyBorrowed = false;
        m_leafPackage = nullptr;
    }

private:
    using ExternalStopCallback = std::stop_callback<std::function<void()>>;

    void ResetStopSource() {
        m_externalStopCallback.reset();
        m_stopSource = std::stop_source{};
        if (m_externalStopToken.stop_possible()) {
            m_externalStopCallback = std::make_unique<ExternalStopCallback>(
                    m_externalStopToken,
                    [this]() { m_stopSource.request_stop(); });
        }
    }

    CodecStorageParams m_storageParams;
    std::vector<std::uint8_t> m_committedAttributes;
    CodecValidationPolicy m_validationPolicy;
    ResourceBudgetControlParams m_resourceBudget;
    CacheResources m_cacheResources;
    bytestore::ByteStoreSession m_byteStoreSession;
    const LeafPackage* m_leafPackage{nullptr};
    const LeafPackageField* m_attributePayloadField{nullptr};
    std::shared_ptr<bytestore::IByteSource> m_attributePayloadOwner;
    std::span<const std::uint8_t> m_attributePayloadBytes;
    std::stop_source m_stopSource;
    std::stop_token m_externalStopToken;
    std::unique_ptr<ExternalStopCallback> m_externalStopCallback;
    std::atomic_bool m_failureCleanupCompleted{false};
};

} // namespace datacodec

#endif
