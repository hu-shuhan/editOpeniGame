#ifndef DATACODEC_WORKFLOW_LEAF_ENCODEOUTPUTWRITER_H
#define DATACODEC_WORKFLOW_LEAF_ENCODEOUTPUTWRITER_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Storage/ByteStore/SegmentedBinaryObject.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageByteWriter.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageFieldEncode.h"
#include "DataCodec/Storage/LeafPackage/EncodedLeafFieldBundle.h"
#include "DataCodec/Runtime/Context/EncodeContext.h"
#include "DataCodec/Runtime/Workspace/EncodeLeafWorkspace.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace datacodec {

class EncodeOutputWriter final {
public:
    static constexpr double kEncodeStageProgressEnd = 0.75;
    static constexpr double kPackageFieldProgressEnd = 0.98;
    static constexpr double kPackageFinalizeProgress = 0.985;
    static constexpr double kEncodedOutputProgressEnd = 0.99;

    static bool BuildEncodedLeafFieldBundle(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace,
        EncodedLeafFieldBundle& output,
        std::uint64_t* exportedByteCount = nullptr,
        std::string* error = nullptr) {
        output.Release();
        EncodedLeafFieldBundle staging;
        staging.path = context.path;
        RefreshAttributeDecodeScheduleHints(workspace.StorageParams());
        if (!SerializeCodecStorageParams(workspace.StorageParams(), staging.paramsBytes, error)) {
            return false;
        }

        const auto geometryIndex = workspace.TransferCacheLayout().geometry;
        if (geometryIndex == kInvalidTransferCacheIndex) {
            return validation::AssignError(error, "encoded geometry staging source is missing");
        }
        auto& geometry = workspace.TransferCache(geometryIndex);
        if (!AppendEncodedLeafSegment(
                staging.segments,
                EncodedLeafSegmentKind::Geometry,
                0u,
                std::move(geometry.transferCache),
                error) ||
            !AppendTopologySegments(staging.segments, staging.backingOwners, workspace, error) ||
            !AppendAttributeSegments(staging.segments, staging.backingOwners, workspace, error)) {
            return false;
        }

        staging.byteStoreSession = std::make_shared<bytestore::ByteStoreSession>(
            workspace.TakeByteStoreSession());
        std::uint64_t outputBytes = staging.paramsBytes.size();
        for (const auto& segment : staging.segments) {
            outputBytes = validation::SaturatingAddU64(
                outputBytes,
                segment.data != nullptr ? segment.data->ByteSizeHint() : 0u);
        }
        if (exportedByteCount != nullptr) {
            *exportedByteCount = outputBytes;
        }
        context.runSummary.outputBytes = outputBytes;
        output = std::move(staging);
        return true;
    }

    static bool WriteLeafPackage(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace,
        const PackageFieldEncodingParams& encodingParams,
        IByteRangeOutput* outputSink,
        std::uint64_t* encodedByteCount,
        std::string* error) {
        if (outputSink == nullptr) {
            return validation::AssignError(
                error,
                "leaf package byte writer requires a byte range output");
        }

        std::uint64_t totalRawBytes = 0u;
        for (std::size_t index = 0u; index < workspace.TransferCaches().Count(); ++index) {
            const auto& record = workspace.TransferCache(index);
            if (record.transferCache == nullptr) {
                continue;
            }
            const auto byteSize = record.transferCache->ByteSizeHint();
            if (!bytestore::IsUnknownByteSize(byteSize)) {
                totalRawBytes = validation::SaturatingAddU64(totalRawBytes, byteSize);
            }
        }

        SubmitProgress(
            context,
            kEncodeStageProgressEnd,
            encodingParams.mode == PackageFieldEncodingMode::Zstd
                ? "打包压缩"
                : "打包写入");
        LeafPackageByteWriter packageWriter;
        if (!packageWriter.BeginPackageToSink(
                workspace.TransferCaches().Count(),
                0u,
                *outputSink,
                error)) {
            return false;
        }

        std::uint64_t completedRawBytes = 0u;
        double lastReportedProgress = kEncodeStageProgressEnd;
        for (std::size_t index = 0u; index < workspace.TransferCaches().Count(); ++index) {
            if (!workspace.WaitUntilTransferCacheReady(index, workspace.StopToken())) {
                return validation::AssignError(
                    error,
                    "encode transfer cache output was stopped while writing encoded output");
            }
            const auto& record = workspace.TransferCache(index);
            const auto fieldRawBytes = record.transferCache == nullptr ||
                    bytestore::IsUnknownByteSize(record.transferCache->ByteSizeHint())
                ? 0u
                : record.transferCache->ByteSizeHint();
            const auto reportFieldProgress = [&](const std::uint64_t fieldCompletedBytes, const std::uint64_t) {
                const auto currentRawBytes = validation::SaturatingAddU64(
                    completedRawBytes,
                    std::min(fieldCompletedBytes, fieldRawBytes));
                const auto normalized = totalRawBytes == 0u
                    ? kPackageFieldProgressEnd
                    : kEncodeStageProgressEnd +
                        (kPackageFieldProgressEnd - kEncodeStageProgressEnd) *
                            static_cast<double>(currentRawBytes) /
                            static_cast<double>(totalRawBytes);
                if (normalized >= lastReportedProgress + 0.002 || currentRawBytes >= totalRawBytes) {
                    lastReportedProgress = normalized;
                    SubmitProgress(context, normalized);
                }
            };
            if (!WriteTransferCache(
                    packageWriter,
                    workspace,
                    index,
                    encodingParams,
                    reportFieldProgress,
                    error)) {
                return false;
            }
            completedRawBytes = validation::SaturatingAddU64(completedRawBytes, fieldRawBytes);
        }

        SubmitProgress(context, kPackageFinalizeProgress, "完成文件");
        if (!packageWriter.EndPackage(error)) {
            return false;
        }
        SubmitProgress(context, kEncodedOutputProgressEnd);
        if (encodedByteCount != nullptr) {
            *encodedByteCount = packageWriter.ByteCount();
        }
        return true;
    }

private:
    class LeafPackageStreamWriter final : public bytestore::IByteWriter {
    public:
        explicit LeafPackageStreamWriter(LeafPackageByteWriter& writer)
            : m_writer(writer) {}

        bool Write(
            const std::span<const std::uint8_t> bytes,
            std::string* error = nullptr) override {
            if (!m_writer.WriteStreamBytes(bytes, error)) {
                return false;
            }
            m_byteSize += static_cast<std::uint64_t>(bytes.size());
            return true;
        }

        [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override {
            return m_byteSize;
        }

    private:
        LeafPackageByteWriter& m_writer;
        std::uint64_t m_byteSize{0u};
    };

    static bool AppendEncodedLeafSegment(
        std::vector<EncodedLeafSegment>& segments,
        const EncodedLeafSegmentKind kind,
        const std::uint32_t ordinal,
        std::shared_ptr<bytestore::IByteSource> source,
        std::string* error) {
        if (source == nullptr) {
            return validation::AssignError(error, "encoded leaf segment source is null");
        }
        const auto rawBytes = source->ByteSizeHint();
        if (bytestore::IsUnknownByteSize(rawBytes)) {
            return validation::AssignError(error, "encoded leaf segment source size is unknown");
        }
        if (rawBytes != 0u && !source->CanRead()) {
            return validation::AssignError(error, "encoded leaf segment source cannot be read");
        }
        segments.push_back(EncodedLeafSegment{
            .kind = kind,
            .ordinal = ordinal,
            .rawBytes = rawBytes,
            .data = std::move(source),
        });
        return true;
    }

    static bool AppendTopologySegments(
        std::vector<EncodedLeafSegment>& segments,
        std::vector<std::shared_ptr<bytestore::IByteSource>>& backingOwners,
        EncodeLeafWorkspace& workspace,
        std::string* error) {
        if (!workspace.TransferCacheLayout().topology.has_value()) {
            return true;
        }
        auto& record = workspace.TransferCache(*workspace.TransferCacheLayout().topology);
        if (record.transferCache == nullptr || record.transferCache->ByteSizeHint() == 0u) {
            return true;
        }
        auto transferCache = std::move(record.transferCache);
        auto segmented = std::dynamic_pointer_cast<bytestore::SegmentedBinaryObject>(transferCache);
        if (segmented == nullptr) {
            return validation::AssignError(error, "encoded topology staging source is not segmented");
        }
        const auto slotCount = segmented->SegmentCount();
        if (workspace.StorageParams().topoParams.isPolyhedron) {
            constexpr EncodedLeafSegmentKind kKinds[]{
                EncodedLeafSegmentKind::PolyhedronUniqueVertexCounts,
                EncodedLeafSegmentKind::PolyhedronCellFaceCounts,
                EncodedLeafSegmentKind::PolyhedronFaceVertexCounts,
                EncodedLeafSegmentKind::PolyhedronCellUniqueVertexIds,
                EncodedLeafSegmentKind::PolyhedronLocalFaceVertexIds,
            };
            if (slotCount != std::size(kKinds)) {
                return validation::AssignError(error, "encoded polyhedron topology segment count mismatch");
            }
            for (std::size_t index = 0u; index < std::size(kKinds); ++index) {
                if (!AppendEncodedLeafSegment(
                        segments,
                        kKinds[index],
                        0u,
                        segmented->SegmentSource(index),
                        error)) {
                    return false;
                }
            }
        } else {
            const auto expectedBlockCount =
                workspace.StorageParams().topoParams.connectivityLayout.blockLayouts.size();
            if (slotCount != expectedBlockCount) {
                return validation::AssignError(error, "encoded ordinary topology segment count mismatch");
            }
            for (std::size_t index = 0u; index < slotCount; ++index) {
                if (!AppendEncodedLeafSegment(
                        segments,
                        EncodedLeafSegmentKind::OrdinaryTopologyBlockPayload,
                        static_cast<std::uint32_t>(index),
                        segmented->SegmentSource(index),
                        error)) {
                    return false;
                }
            }
        }
        backingOwners.push_back(std::move(transferCache));
        return true;
    }

    static bool AppendAttributeSegments(
        std::vector<EncodedLeafSegment>& segments,
        std::vector<std::shared_ptr<bytestore::IByteSource>>& backingOwners,
        EncodeLeafWorkspace& workspace,
        std::string* error) {
        if (!workspace.TransferCacheLayout().attributes.has_value()) {
            return true;
        }
        auto& record = workspace.TransferCache(*workspace.TransferCacheLayout().attributes);
        auto transferCache = std::move(record.transferCache);
        auto attributeOutput = std::dynamic_pointer_cast<AttributeSpooler>(transferCache);
        if (attributeOutput == nullptr) {
            return validation::AssignError(error, "encoded attribute staging output is missing");
        }
        std::vector<std::shared_ptr<bytestore::IByteSource>> recordSources;
        if (!attributeOutput->OrderedRecordSources(recordSources, error)) {
            return false;
        }
        if (recordSources.size() > std::numeric_limits<std::uint32_t>::max()) {
            return validation::AssignError(
                error,
                "encoded attribute record count exceeds uint32 ordinal range");
        }
        for (std::size_t index = 0u; index < recordSources.size(); ++index) {
            if (!AppendEncodedLeafSegment(
                    segments,
                    EncodedLeafSegmentKind::AttributePayload,
                    static_cast<std::uint32_t>(index),
                    std::move(recordSources[index]),
                    error)) {
                return false;
            }
        }
        backingOwners.push_back(std::move(transferCache));
        return true;
    }

    static bool WriteTransferCache(
        LeafPackageByteWriter& packageWriter,
        EncodeLeafWorkspace& workspace,
        const std::size_t transferCacheIndex,
        const PackageFieldEncodingParams& encodingParams,
        const std::function<void(std::uint64_t, std::uint64_t)>& progressCallback,
        std::string* error) {
        auto& record = workspace.TransferCache(transferCacheIndex);
        if (!PreflightTransferCache(record, transferCacheIndex, error)) {
            return false;
        }

        auto schedule = record.schedule;
        schedule.rawSize = record.transferCache->ByteSizeHint();
        if (schedule.fieldType != FieldType::Params && schedule.rawSize == 0u) {
            schedule.compressionType = EncodedFieldCompressionType::None;
            if (!packageWriter.BeginStream(transferCacheIndex, schedule, error) ||
                !packageWriter.EndStream(schedule, error)) {
                return false;
            }
            record.transferCache->Release();
            record.transferCache.reset();
            return true;
        }
        if (!packageWriter.BeginStream(transferCacheIndex, schedule, error)) {
            return false;
        }

        LeafPackageStreamWriter streamWriter(packageWriter);
        std::uint64_t finalRawSize = 0u;
        if (!EncodeLeafPackageFieldToWriter(
                *record.transferCache,
                schedule.fieldType,
                encodingParams,
                LeafPackageFieldEncodeRuntime{
                    .windowBudget = workspace.CacheResourcesRef().windowBudget,
                    .scratchBytePool = workspace.CacheResourcesRef().scratchBytePool,
                    .accessWindowBytes = workspace.CacheResourcesRef().accessWindowBytes,
                    .progressCallback = progressCallback,
                },
                streamWriter,
                schedule.compressionType,
                finalRawSize,
                error)) {
            return false;
        }
        schedule.rawSize = finalRawSize;
        if (!packageWriter.EndStream(schedule, error)) {
            return false;
        }
        record.transferCache->Release();
        record.transferCache.reset();
        return true;
    }

    static bool PreflightTransferCache(
        const EncodeTransferUnit& record,
        const std::size_t transferCacheIndex,
        std::string* error) {
        if (record.transferCache == nullptr) {
            return validation::AssignError(
                error,
                "encode transfer cache preflight received a null byte source");
        }
        const auto byteSize = record.transferCache->ByteSizeHint();
        if (bytestore::IsUnknownByteSize(byteSize)) {
            return validation::AssignError(
                error,
                "encode transfer cache '" + record.schedule.label + "' at index " +
                    std::to_string(transferCacheIndex) + " has unknown byte size");
        }
        if (byteSize != 0u && !record.transferCache->CanRead()) {
            return validation::AssignError(
                error,
                "encode transfer cache '" + record.schedule.label + "' at index " +
                    std::to_string(transferCacheIndex) + " does not support ranged reads");
        }
        return true;
    }

    static void SubmitProgress(
        EncodeContext& context,
        const double normalized,
        std::string text = {}) {
        context.runRecords.SubmitProgress(RunProgressRecord{
            .phase = RunProgressPhase::Update,
            .normalized = std::clamp(normalized, 0.0, kEncodedOutputProgressEnd),
            .text = std::move(text),
            .success = false,
        });
    }
};

} // namespace datacodec

#endif
