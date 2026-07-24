#ifndef DATACODEC_RUNTIME_WORKSPACE_ENCODELEAFWORKSPACE_H
#define DATACODEC_RUNTIME_WORKSPACE_ENCODELEAFWORKSPACE_H

#include "DataCodec/Common/Views/ArrayViews.h"
#include "DataCodec/Runtime/Failure/FailureCleanable.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Storage/ByteIO/Window/WindowRuntimeParams.h"
#include "DataCodec/Codec/NumericArray/NumericArraySource.h"
#include "DataCodec/Runtime/Workspace/EncodeTransferCacheSet.h"
#include "DataCodec/Codec/Attributes/AttributeSpooler.h"
#include "DataCodec/Codec/Attributes/AttributeEncodeScheduler.h"
#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/API/Params/CodecStorageParams.h"
#include "DataCodec/Codec/Remap/RemapOrderSource.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Codec/Reference/AttributeReferenceSchedule.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <cstdint>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace datacodec {

inline bool ResolveRequiredTransferCacheIndex(
    const std::size_t transferCacheIndex,
    const std::string_view stageType,
    const std::string_view layoutName,
    std::size_t& resolvedTransferCacheIndex,
    std::string* error = nullptr) {
    if (transferCacheIndex != kInvalidTransferCacheIndex) {
        resolvedTransferCacheIndex = transferCacheIndex;
        return true;
    }

    return validation::AssignError(
        error,
        "stage '" + std::string(stageType) + "' requires encode transfer cache layout entry '" +
            std::string(layoutName) + "'");
}

struct EncodeTransferCacheLayout {
    std::size_t params{kInvalidTransferCacheIndex};
    std::size_t geometry{kInvalidTransferCacheIndex};
    std::optional<std::size_t> topology;
    std::optional<std::size_t> attributes;
    std::vector<std::size_t> pointAttrs;
    std::vector<std::size_t> cellAttrs;
};

class EncodeLeafWorkspace final : public IFailureCleanable {
public:
    void Reset() {
        m_failureCleanupCompleted.store(false, std::memory_order_release);
        m_stopSource = std::stop_source{};
        m_storageParams = {};
        m_resourceBudget = {};
        m_cacheResources.Clear();
        m_attributeEncodeScheduler.Configure(nullptr);
        ClearAttributeReferenceSchedules();
        ClearGeometrySource();
        (void)ReleasePointRemap();
        (void)ReleaseCellRemap();
        m_byteStoreSession.Reset();
        {
            std::lock_guard<std::mutex> lock(m_transferCacheMutex);
            m_transferCaches.Clear();
        }
        m_transferCacheLayout = {};
    }

    [[nodiscard]] CodecStorageParams& StorageParams() noexcept { return m_storageParams; }
    [[nodiscard]] const CodecStorageParams& StorageParams() const noexcept { return m_storageParams; }
    [[nodiscard]] const ResourceBudgetControlParams& ResourceBudget() const noexcept { return m_resourceBudget; }
    void SetResourceBudget(ResourceBudgetControlParams params) {
        m_resourceBudget = std::move(params);
        m_byteStoreSession.ConfigureResidentLimit(m_resourceBudget.ResidentLimitBytes());
    }
    [[nodiscard]] CacheResources& CacheResourcesRef() noexcept { return m_cacheResources; }
    [[nodiscard]] const CacheResources& CacheResourcesRef() const noexcept { return m_cacheResources; }
    [[nodiscard]] bytestore::ByteStoreSession& ByteStoreSessionRef() noexcept { return m_byteStoreSession; }
    [[nodiscard]] const bytestore::ByteStoreSession& ByteStoreSessionRef() const noexcept { return m_byteStoreSession; }
    [[nodiscard]] bytestore::ByteStoreSession TakeByteStoreSession() {
        return std::move(m_byteStoreSession);
    }
    [[nodiscard]] std::vector<std::string> TakeByteStoreDiagnostics() {
        return m_byteStoreSession.TakeDiagnostics();
    }
    [[nodiscard]] ScratchByteBufferPool& ScratchBytePool() noexcept { return m_cacheResources.scratchBytePool; }
    [[nodiscard]] const ScratchByteBufferPool& ScratchBytePool() const noexcept { return m_cacheResources.scratchBytePool; }
    void ConfigureCacheResources(
        const std::size_t accessWindowBytes = kDefaultEncodeAccessWindowBytes,
        const std::uint64_t activeWindowBytes = kDefaultEncodeActiveWindowBytes,
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
    void ConfigureAttributeEncodeResources(
        const bool collectTiming = false) {
        m_attributeEncodeScheduler.Configure(&m_resourceBudget, collectTiming);
    }
    [[nodiscard]] AttributeEncodeScheduler& AttributeEncodeSchedulerRef() noexcept {
        return m_attributeEncodeScheduler;
    }
    [[nodiscard]] const AttributeEncodeScheduler& AttributeEncodeSchedulerRef() const noexcept {
        return m_attributeEncodeScheduler;
    }
    [[nodiscard]] const RemapOrderSource& PointOrderSource() const noexcept { return m_pointOrderSource; }
    [[nodiscard]] const RemapOrderSource& PointInverseOrderSource() const noexcept {
        return m_pointInverseOrderSource;
    }
    [[nodiscard]] const RemapOrderSource& CellOrderSource() const noexcept { return m_cellOrderSource; }
    bool SetPointRemap(std::shared_ptr<IRemapProvider> order, std::shared_ptr<IRemapProvider> inverse) {
        auto orderSource = RemapOrderSource::TryComputed(std::move(order));
        auto inverseSource = RemapOrderSource::TryComputed(std::move(inverse));
        if (!orderSource.has_value() || !inverseSource.has_value()) {
            return false;
        }
        m_pointOrderSource = std::move(*orderSource);
        m_pointInverseOrderSource = std::move(*inverseSource);
        return true;
    }
    bool SetCellRemap(std::shared_ptr<IRemapProvider> order) {
        auto orderSource = RemapOrderSource::TryComputed(std::move(order));
        if (!orderSource.has_value()) {
            return false;
        }
        m_cellOrderSource = std::move(*orderSource);
        return true;
    }
    [[nodiscard]] const NumericArrayView* GeometrySourceView() const noexcept {
        return m_geometrySourceInitialized ? &m_geometrySourceView : nullptr;
    }
    [[nodiscard]] numericarray::NumericArraySource GeometryNumericArraySource() const {
        numericarray::NumericArraySource source;
        source.values = m_geometrySourceView;
        // Geometry 明确消费 Pipeline 绑定的 Order Source
        source.orderProvider = m_pointOrderSource.Provider();
        source.layout = numericarray::MakeNumericArrayLayoutFromView(
            m_geometrySourceView,
            m_geometrySourceView.tupleCount,
            3u);
        source.owner = m_geometrySourceOwner;
        return source;
    }
    [[nodiscard]] EncodeTransferCacheLayout& TransferCacheLayout() noexcept { return m_transferCacheLayout; }
    [[nodiscard]] const EncodeTransferCacheLayout& TransferCacheLayout() const noexcept { return m_transferCacheLayout; }
    [[nodiscard]] EncodeTransferCacheSet& TransferCaches() noexcept { return m_transferCaches; }
    [[nodiscard]] const EncodeTransferCacheSet& TransferCaches() const noexcept { return m_transferCaches; }
    [[nodiscard]] std::stop_token StopToken() const noexcept { return m_stopSource.get_token(); }
    [[nodiscard]] bool StopRequested() const noexcept { return m_stopSource.stop_requested(); }
    void RequestStop() noexcept { m_stopSource.request_stop(); }

    [[nodiscard]] std::uint64_t ReleasePointRemap() {
        auto released = m_pointOrderSource.Release();
        released = validation::SaturatingAddU64(released, m_pointInverseOrderSource.Release());
        return released;
    }
    [[nodiscard]] std::uint64_t ReleaseCellRemap() {
        return m_cellOrderSource.Release();
    }
    [[nodiscard]] std::uint64_t ReleaseGeometrySource() {
        ClearGeometrySource();
        return 0u;
    }
    bool InitializeGeometrySource(
        const NumericArrayView& geometry,
        std::shared_ptr<const void> owner = nullptr,
        std::string* error = nullptr) {
        ClearGeometrySource();
        m_geometrySourceOwner = std::move(owner);
        if (!ValidateGeometrySourceInput(geometry, error)) {
            m_geometrySourceOwner.reset();
            return false;
        }
        auto normalized = NormalizeGeometrySourceView(geometry);
        if (!ValidateGeometrySourceView(normalized, error)) {
            m_geometrySourceOwner.reset();
            return false;
        }
        m_geometrySourceView = std::move(normalized);
        m_geometrySourceInitialized = true;
        ApplyGeometryCacheShape(m_geometrySourceView);
        return true;
    }
    bool InitializeAttributeOutput(const std::size_t attrCount, std::string* error = nullptr) {
        if (attrCount == 0u) {
            return true;
        }
        if (!m_transferCacheLayout.attributes.has_value()) {
            return validation::AssignError(error, "attribute output layout was not initialized");
        }
        MarkTransferCachePending(*m_transferCacheLayout.attributes);
        auto transferCache = std::make_shared<AttributeSpooler>(
            attrCount,
            StopToken(),
            bytestore::ByteSourceConsumptionMode::OneShot);
        m_storageParams.attrPayloadOrder = MakeDefaultAttributePayloadOrder(attrCount);
        auto& record = m_transferCaches.TransferCache(*m_transferCacheLayout.attributes);
        if (record.transferCache != nullptr) {
            record.transferCache->Release();
        }
        record.schedule.codecType = EncodedFieldCodecType::Raw;
        record.schedule.rawSize = bytestore::kUnknownByteSize;
        record.schedule.compressionType = EncodedFieldCompressionType::None;
        record.transferCache = std::move(transferCache);
        return true;
    }

    [[nodiscard]] std::shared_ptr<AttributeSpooler> AttributeOutput() const {
        if (!m_transferCacheLayout.attributes.has_value()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<AttributeSpooler>(
            TransferCache(*m_transferCacheLayout.attributes).transferCache);
    }

    bool PublishAttributeFragmentOutput(
        const std::size_t attrMetaIndex,
        std::shared_ptr<bytestore::IByteSource> transferCache,
        std::string* error = nullptr) {
        auto attributeOutput = AttributeOutput();
        if (attributeOutput == nullptr) {
            return validation::AssignError(error, "attribute output was not initialized");
        }
        return attributeOutput->SetRecordSource(attrMetaIndex, std::move(transferCache), error);
    }

    void InitializeAttributeReferenceSchedules(
        const std::size_t pointAttrCount,
        const std::size_t cellAttrCount) {
        std::lock_guard<std::mutex> lock(m_attrReferenceScheduleMutex);
        m_pointAttrReferenceSchedule = {};
        m_cellAttrReferenceSchedule = {};
        m_pointAttrReferenceSchedule.topologyOrder.resize(pointAttrCount);
        m_pointAttrReferenceSchedule.entries.resize(pointAttrCount);
        m_cellAttrReferenceSchedule.topologyOrder.resize(cellAttrCount);
        m_cellAttrReferenceSchedule.entries.resize(cellAttrCount);
        for (std::size_t index = 0; index < pointAttrCount; ++index) {
            m_pointAttrReferenceSchedule.topologyOrder[index] = index;
        }
        for (std::size_t index = 0; index < cellAttrCount; ++index) {
            m_cellAttrReferenceSchedule.topologyOrder[index] = index;
        }
    }

    [[nodiscard]] EncodeAttributeReferenceSchedule AttributeReferenceScheduleSnapshot(
        const AttrAttachment attachment) const {
        std::lock_guard<std::mutex> lock(m_attrReferenceScheduleMutex);
        return attachment == AttrAttachment::Point
            ? m_pointAttrReferenceSchedule
            : m_cellAttrReferenceSchedule;
    }

    bool PublishAttributeReferenceSchedule(
        const AttrAttachment attachment,
        EncodeAttributeReferenceSchedule schedule,
        std::string* error = nullptr) {
        {
            std::lock_guard<std::mutex> lock(m_attrReferenceScheduleMutex);
            auto& target = attachment == AttrAttachment::Point
                ? m_pointAttrReferenceSchedule
                : m_cellAttrReferenceSchedule;
            target = std::move(schedule);
            target.initialized = true;
        }
        return ApplyAttributeRecordOrder(error);
    }

    void ClearTransferCaches() {
        std::lock_guard<std::mutex> lock(m_transferCacheMutex);
        m_transferCaches.Clear();
        m_transferCacheLayout = {};
    }

    void CleanupOnFailure() noexcept override {
        if (m_failureCleanupCompleted.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        RequestStop();
        m_resourceBudget = {};
        ClearGeometrySource();
        (void)ReleasePointRemap();
        (void)ReleaseCellRemap();
        m_byteStoreSession.ReleaseAll();
        ClearAttributeReferenceSchedules();
        {
            std::lock_guard<std::mutex> lock(m_transferCacheMutex);
            m_transferCaches.AbortAll();
            m_transferCacheLayout = {};
        }
        m_cacheResources.Clear();
    }

    [[nodiscard]] std::size_t AddTransferCache(
        const FieldType type,
        std::string label,
        const std::size_t ordinal = 0,
        const AttrAttachment attachment = AttrAttachment::Point,
        const EncodedFieldCodecType codecType = EncodedFieldCodecType::Unknown) {
        std::lock_guard<std::mutex> lock(m_transferCacheMutex);
        const auto index = m_transferCaches.AddTransferCache(type, std::move(label), ordinal, attachment, codecType);
        return index;
    }

    [[nodiscard]] EncodeTransferUnit& TransferCache(const std::size_t index) {
        return m_transferCaches.TransferCache(index);
    }
    [[nodiscard]] const EncodeTransferUnit& TransferCache(const std::size_t index) const {
        return m_transferCaches.TransferCache(index);
    }

    void PublishTransferCacheBytes(
        const std::size_t index,
        std::vector<std::uint8_t> bytes,
        const EncodedFieldCodecType codecType) {
        {
            std::lock_guard<std::mutex> lock(m_transferCacheMutex);
            m_transferCaches.PublishTransferCacheBytes(index, std::move(bytes), codecType);
        }
    }

    void PublishTransferCache(
        const std::size_t index,
        std::shared_ptr<bytestore::IByteSource> transferCache,
        const EncodedFieldCodecType codecType) {
        {
            std::lock_guard<std::mutex> lock(m_transferCacheMutex);
            m_transferCaches.PublishTransferCache(index, std::move(transferCache), codecType);
        }
    }

    void MarkTransferCachePending(const std::size_t index) {
        std::lock_guard<std::mutex> lock(m_transferCacheMutex);
        m_transferCaches.MarkPending(index);
    }
    void MarkTransferCacheReady(const std::size_t index) {
        {
            std::lock_guard<std::mutex> lock(m_transferCacheMutex);
            m_transferCaches.MarkReady(index);
        }
    }
    [[nodiscard]] bool IsTransferCacheReady(const std::size_t index) const { return m_transferCaches.IsReady(index); }
    bool WaitUntilTransferCacheReady(const std::size_t index, const std::stop_token& stopToken = {}) const {
        return m_transferCaches.WaitUntilReady(index, stopToken);
    }
    [[nodiscard]] std::size_t TransferCacheCount() const noexcept { return m_transferCaches.Count(); }

    [[nodiscard]] std::size_t PointAttrMetaIndex(const std::size_t attrIndex) const noexcept {
        return attrIndex < m_transferCacheLayout.pointAttrs.size()
            ? m_transferCacheLayout.pointAttrs[attrIndex]
            : kInvalidTransferCacheIndex;
    }

    [[nodiscard]] std::size_t CellAttrMetaIndex(const std::size_t attrIndex) const noexcept {
        return attrIndex < m_transferCacheLayout.cellAttrs.size()
            ? m_transferCacheLayout.cellAttrs[attrIndex]
            : kInvalidTransferCacheIndex;
    }

    [[nodiscard]] std::size_t PointAttrSourceIndex(const std::size_t attrIndex) const noexcept {
        return ResolveAttrSourceIndex(m_storageParams, PointAttrMetaIndex(attrIndex));
    }

    [[nodiscard]] std::size_t CellAttrSourceIndex(const std::size_t attrIndex) const noexcept {
        return ResolveAttrSourceIndex(m_storageParams, CellAttrMetaIndex(attrIndex));
    }

    std::mutex& AttributeReferenceScheduleBuildMutex() noexcept { return m_attrReferenceScheduleBuildMutex; }

    [[nodiscard]] EncodeAttributeReferenceSchedule& MutableAttributeReferenceSchedule(
        const AttrAttachment attachment) noexcept {
        return attachment == AttrAttachment::Point
            ? m_pointAttrReferenceSchedule
            : m_cellAttrReferenceSchedule;
    }

    [[nodiscard]] const EncodeAttributeReferenceSchedule& AttributeReferenceSchedule(
        const AttrAttachment attachment) const noexcept {
        return attachment == AttrAttachment::Point
            ? m_pointAttrReferenceSchedule
            : m_cellAttrReferenceSchedule;
    }

    std::mutex& AttributeReferenceScheduleMutex() noexcept { return m_attrReferenceScheduleMutex; }

private:
    void ClearGeometrySource() noexcept {
        m_geometrySourceView = {};
        m_geometrySourceOwner.reset();
        m_geometrySourceInitialized = false;
    }

    static NumericArrayView NormalizeGeometrySourceView(NumericArrayView view) {
        constexpr std::size_t kGeometryComponentCount = 3u;
        const auto valueSize = view.ComponentSize();
        const auto sourceComponentCount = view.componentCount > 0
            ? static_cast<std::size_t>(view.componentCount)
            : 0u;
        if (view.layout == ArrayLayout::GetterOnly) {
            return view;
        }
        if (view.layout == ArrayLayout::CompactAOS && sourceComponentCount > kGeometryComponentCount) {
            view.layout = ArrayLayout::StridedAOS;
            view.tupleStrideBytes = sourceComponentCount * valueSize;
            view.componentCount = static_cast<int>(kGeometryComponentCount);
            return view;
        }
        if (view.layout == ArrayLayout::CompactAOS) {
            view.tupleStrideBytes = kGeometryComponentCount * valueSize;
            view.componentCount = static_cast<int>(kGeometryComponentCount);
            return view;
        }
        if (view.layout == ArrayLayout::StridedAOS) {
            if (view.tupleStrideBytes == 0u) {
                view.tupleStrideBytes = sourceComponentCount * valueSize;
            }
            view.componentCount = static_cast<int>(kGeometryComponentCount);
            return view;
        }
        view.componentCount = static_cast<int>(kGeometryComponentCount);
        return view;
    }

    static bool ValidateGeometrySourceInput(
        const NumericArrayView& view,
        std::string* error = nullptr) {
        constexpr std::size_t kGeometryComponentCount = 3u;
        if (!view.IsValid()) {
            return validation::AssignError(error, "geometry view is invalid");
        }
        if (view.ComponentSize() == 0u) {
            return validation::AssignError(error, "geometry scalar type is unsupported");
        }
        if (view.componentCount < static_cast<int>(kGeometryComponentCount)) {
            return validation::AssignError(error, "geometry component count is too small");
        }
        return true;
    }

    static bool ValidateGeometrySourceView(
        const NumericArrayView& view,
        std::string* error = nullptr) {
        constexpr std::size_t kGeometryComponentCount = 3u;
        if (!view.IsValid()) {
            return validation::AssignError(error, "geometry view is invalid");
        }
        if (view.ComponentSize() == 0u) {
            return validation::AssignError(error, "geometry scalar type is unsupported");
        }
        if (view.tupleCount != 0u && view.componentCount < static_cast<int>(kGeometryComponentCount)) {
            return validation::AssignError(error, "geometry component count is too small");
        }
        return true;
    }

    void ApplyGeometryCacheShape(const NumericArrayView& view) {
        m_storageParams.geomParams.dataType = ToDataType(view.scalarType);
        m_storageParams.geomParams.valueSize = view.ComponentSize();
        m_storageParams.geomParams.elementCount = view.tupleCount;
        m_storageParams.geomParams.dimension = 3;
    }

    void ClearAttributeReferenceSchedules() {
        std::lock_guard<std::mutex> lock(m_attrReferenceScheduleMutex);
        m_pointAttrReferenceSchedule = {};
        m_cellAttrReferenceSchedule = {};
    }

    bool ApplyAttributeRecordOrder(std::string* error) {
        if (!m_transferCacheLayout.attributes.has_value()) {
            return true;
        }
        auto attributeOutput = AttributeOutput();
        if (attributeOutput == nullptr) {
            return validation::AssignError(error, "attribute transfer cache was not initialized");
        }

        EncodeAttributeReferenceSchedule pointSchedule;
        EncodeAttributeReferenceSchedule cellSchedule;
        {
            std::lock_guard<std::mutex> lock(m_attrReferenceScheduleMutex);
            pointSchedule = m_pointAttrReferenceSchedule;
            cellSchedule = m_cellAttrReferenceSchedule;
        }

        std::vector<std::size_t> recordOrder;
        recordOrder.reserve(m_transferCacheLayout.pointAttrs.size() + m_transferCacheLayout.cellAttrs.size());
        if (pointSchedule.topologyOrder.size() == m_transferCacheLayout.pointAttrs.size()) {
            for (const auto localIndex : pointSchedule.topologyOrder) {
                if (localIndex < m_transferCacheLayout.pointAttrs.size()) {
                    recordOrder.push_back(m_transferCacheLayout.pointAttrs[localIndex]);
                }
            }
        } else {
            for (std::size_t localIndex = 0; localIndex < m_transferCacheLayout.pointAttrs.size(); ++localIndex) {
                recordOrder.push_back(m_transferCacheLayout.pointAttrs[localIndex]);
            }
        }
        if (cellSchedule.topologyOrder.size() == m_transferCacheLayout.cellAttrs.size()) {
            for (const auto localIndex : cellSchedule.topologyOrder) {
                if (localIndex < m_transferCacheLayout.cellAttrs.size()) {
                    recordOrder.push_back(m_transferCacheLayout.cellAttrs[localIndex]);
                }
            }
        } else {
            for (std::size_t localIndex = 0; localIndex < m_transferCacheLayout.cellAttrs.size(); ++localIndex) {
                recordOrder.push_back(m_transferCacheLayout.cellAttrs[localIndex]);
            }
        }
        if (recordOrder.size() != m_transferCacheLayout.pointAttrs.size() + m_transferCacheLayout.cellAttrs.size()) {
            return validation::AssignError(error, "attribute record order contains an invalid local index");
        }
        std::vector<ParamSize> storageOrder;
        storageOrder.reserve(recordOrder.size());
        for (const auto attrIndex : recordOrder) {
            storageOrder.push_back(static_cast<ParamSize>(attrIndex));
        }
        if (!attributeOutput->SetRecordOrder(std::move(recordOrder), error)) {
            return false;
        }
        m_storageParams.attrPayloadOrder = std::move(storageOrder);
        return true;
    }

    std::stop_source m_stopSource;
    CodecStorageParams m_storageParams;
    ResourceBudgetControlParams m_resourceBudget;
    CacheResources m_cacheResources;
    AttributeEncodeScheduler m_attributeEncodeScheduler;
    bytestore::ByteStoreSession m_byteStoreSession;
    NumericArrayView m_geometrySourceView;
    std::shared_ptr<const void> m_geometrySourceOwner;
    bool m_geometrySourceInitialized{false};
    RemapOrderSource m_pointOrderSource;
    RemapOrderSource m_pointInverseOrderSource;
    RemapOrderSource m_cellOrderSource;
    EncodeTransferCacheLayout m_transferCacheLayout;
    EncodeTransferCacheSet m_transferCaches;
    mutable std::mutex m_transferCacheMutex;
    EncodeAttributeReferenceSchedule m_pointAttrReferenceSchedule;
    EncodeAttributeReferenceSchedule m_cellAttrReferenceSchedule;
    mutable std::mutex m_attrReferenceScheduleMutex;
    std::mutex m_attrReferenceScheduleBuildMutex;
    std::atomic_bool m_failureCleanupCompleted{false};
};

} // namespace datacodec

#endif
