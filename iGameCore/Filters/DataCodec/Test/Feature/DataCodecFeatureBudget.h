#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREBUDGET_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREBUDGET_H

#include <DataCodec/Storage/ByteIO/ByteSource.h>
#include <DataCodec/Storage/ByteIO/Window/WindowBudget.h>
#include <DataCodec/Storage/ByteIO/Window/WindowedCopy.h>
#include <DataCodec/Runtime/Cache/DecodeCache/DecodedGeometryCache.h>
#include <DataCodec/Runtime/Cache/DecodeCache/DecodedIndexCache.h>
#include <DataCodec/Codec/Topology/Polyhedron/PolyhedronTopologyStreamDecode.h>
#include <DataCodec/API/Params/CodecPerformancePresetParams.h>
#include <DataCodec/Test/Common/DataCodecTestResult.h>
#include <DataCodec/Test/Assertions/WindowBudgetAssertions.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
namespace datacodec::test::feature_budget {

using datacodec::test::Require;
using datacodec::test::CaptureWindowBudget;
using datacodec::test::RequireWindowBudgetLimit;
using datacodec::test::RequireWindowBudgetSnapshot;
using datacodec::test::TestResult;
using datacodec::ScratchByteBufferPool;
using datacodec::CodecControlParamsFactory;
using datacodec::DataCodecDecodeTier;
using datacodec::DataCodecRuntimeProfile;
using datacodec::DecodeStorageMode;
using datacodec::EncodeStorageMode;
using datacodec::DecodedGeometryCache;
using datacodec::DecodedIndexCache;
using datacodec::bytestore::IByteWriter;
using datacodec::bytestore::ByteStoreSession;
using datacodec::bytestore::VectorByteSource;
using datacodec::window::WindowBudget;
using datacodec::window::WindowedByteSourceReader;
using datacodec::window::CopyByteSourceByWindow;
using datacodec::window::CopyByteSourceRangeByWindow;

class VectorByteWriter final : public IByteWriter {
public:
    bool Write(const std::span<const std::uint8_t> bytes, std::string*) override {
        bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
        return true;
    }

    [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override {
        return static_cast<std::uint64_t>(bytes_.size());
    }

    [[nodiscard]] const std::vector<std::uint8_t>& Bytes() const noexcept {
        return bytes_;
    }

private:
    std::vector<std::uint8_t> bytes_;
};

inline void PrintResult(const TestResult& result) {
    for (const auto& failure : result.failures) {
        std::cerr << failure.check << ": " << failure.message << '\n';
    }
}

inline void RequireBytes(
    TestResult& result,
    const std::span<const std::uint8_t> actual,
    const std::vector<std::uint8_t>& expected,
    const std::string& path) {
    Require(result, actual.size() == expected.size(), path + ".size", "byte count mismatch");
    Require(
        result,
        std::equal(actual.begin(), actual.end(), expected.begin(), expected.end()),
        path + ".bytes",
        "bytes mismatch");
}

inline void RequireBytes(
    TestResult& result,
    const std::vector<std::uint8_t>& actual,
    const std::vector<std::uint8_t>& expected,
    const std::string& path) {
    RequireBytes(
        result,
        std::span<const std::uint8_t>(actual.data(), actual.size()),
        expected,
        path);
}

inline bool TestWindowBudgetLeaseLifecycle() {
    TestResult result;
    WindowBudget budget(128u);
    RequireWindowBudgetLimit(result, CaptureWindowBudget(budget), "budget.initial", 128u);

    {
        auto firstLease = budget.TryAcquire(64u);
        Require(result, firstLease.has_value(), "budget.firstAcquire", "64 bytes should fit into 128 byte budget");
        RequireWindowBudgetSnapshot(
            result,
            CaptureWindowBudget(budget),
            "budget.afterFirstAcquire",
            64u,
            64u);

        auto secondLease = budget.TryAcquire(64u);
        Require(result, secondLease.has_value(), "budget.secondAcquire", "second 64 byte lease should fit");
        RequireWindowBudgetSnapshot(
            result,
            CaptureWindowBudget(budget),
            "budget.afterSecondAcquire",
            128u,
            128u);

        auto rejectedLease = budget.TryAcquire(1u);
        Require(result, !rejectedLease.has_value(), "budget.rejectWhenFull", "budget should reject an over-limit lease");
    }

    RequireWindowBudgetSnapshot(
        result,
        CaptureWindowBudget(budget),
        "budget.afterRelease",
        0u,
        128u);
    budget.Reset(32u);
    RequireWindowBudgetSnapshot(
        result,
        CaptureWindowBudget(budget),
        "budget.afterReset",
        0u,
        0u);
    RequireWindowBudgetLimit(result, CaptureWindowBudget(budget), "budget.afterReset", 32u);

    auto oversizedLease = budget.TryAcquire(33u);
    Require(result, !oversizedLease.has_value(), "budget.rejectOversizedLease", "lease larger than max should be rejected");

    {
        auto zeroLease = budget.TryAcquire(0u);
        Require(result, zeroLease.has_value(), "budget.zeroAcquire", "zero byte lease should normalize to one byte");
        RequireWindowBudgetSnapshot(
            result,
            CaptureWindowBudget(budget),
            "budget.afterZeroAcquire",
            1u,
            1u);
    }

    PrintResult(result);
    return result.passed;
}

inline bool TestActiveBudgetRejectsOversizedBlockingAcquire() {
    TestResult result;
    WindowBudget budget(8u);
    bool rejected = false;
    try {
        auto lease = budget.Acquire(9u);
        (void)lease;
    } catch (const std::length_error&) {
        rejected = true;
    }
    Require(
        result,
        rejected,
        "budget.blockingAcquire.rejectOversized",
        "blocking acquire should reject a request larger than the configured limit");
    PrintResult(result);
    return result.passed;
}

inline bool TestByteStoreResidentBudgetAndSealLifecycle() {
    TestResult result;
    ByteStoreSession session;
    session.ConfigureResidentLimit(8u);
    auto first = session.CreateMemoryStore();
    auto second = session.CreateMemoryStore();
    std::string error;
    Require(
        result,
        first != nullptr && first->ResizeBytes(8u, &error),
        "budget.byteStore.firstResidentAllocation",
        error.empty() ? "first resident allocation failed" : error);
    error.clear();
    Require(
        result,
        second != nullptr && !second->ResizeBytes(1u, &error),
        "budget.byteStore.rejectResidentOverflow",
        "resident allocation should fail after the session limit is exhausted");
    error.clear();
    Require(
        result,
        first->ResizeBytes(4u, &error) && second->ResizeBytes(4u, &error),
        "budget.byteStore.releaseOnShrink",
        error.empty() ? "resident bytes released by shrink were not reusable" : error);
    const auto storeStats = session.SnapshotStats();
    Require(
        result,
        storeStats.residentBytes == 8u &&
            storeStats.peakResidentBytes == 8u &&
            storeStats.residentLimitBytes == 8u,
        "budget.byteStore.stats",
        "byte store resident statistics do not match the configured limit");

    for (const bool useMemoryStore : {true, false}) {
        ByteStoreSession appendSession;
        auto appendStore = bytestore::CreateAppendableByteStore(
            appendSession,
            useMemoryStore ? "append_memory_test" : "append_managed_test",
            useMemoryStore,
            &error);
        const std::array<std::uint8_t, 3u> input{1u, 2u, 3u};
        std::array<std::uint8_t, 3u> output{};
        error.clear();
        Require(
            result,
            appendStore != nullptr && appendStore->AppendBytes(input, &error),
            useMemoryStore ? "budget.append.memory.write" : "budget.append.managed.write",
            error.empty() ? "append write failed" : error);
        error.clear();
        Require(
            result,
            !appendStore->Read(0u, output, &error),
            useMemoryStore ? "budget.append.memory.readBeforeSeal" : "budget.append.managed.readBeforeSeal",
            "append store should reject reads before seal");
        error.clear();
        Require(
            result,
            appendStore->Seal(&error) && appendStore->Read(0u, output, &error) && output == input,
            useMemoryStore ? "budget.append.memory.readAfterSeal" : "budget.append.managed.readAfterSeal",
            error.empty() ? "sealed append store did not replay its bytes" : error);
        error.clear();
        Require(
            result,
            !appendStore->AppendBytes(input, &error),
            useMemoryStore ? "budget.append.memory.rejectAfterSeal" : "budget.append.managed.rejectAfterSeal",
            "append store should reject writes after seal");
    }

    PrintResult(result);
    return result.passed;
}

inline bool TestScratchPoolRetentionLimits() {
    TestResult result;
    ScratchByteBufferPool pool;
    pool.Configure(1u, 8u, 8u);
    {
        auto first = pool.Acquire(4u);
    }
    {
        auto reused = pool.Acquire(4u);
    }
    {
        auto oversized = pool.Acquire(16u);
    }
    const auto stats = pool.SnapshotStats();
    Require(
        result,
        stats.retainedBytes <= 8u &&
            stats.reusedBlockCount == 1u &&
            stats.allocationCount == 2u,
        "budget.scratchPool.retention",
        "scratch pool retention and reuse statistics do not match the configured limits");
    PrintResult(result);
    return result.passed;
}

inline bool TestWindowedByteSourceReaderChunks() {
    TestResult result;
    VectorByteSource source(std::vector<std::uint8_t>{0u, 1u, 2u, 3u, 4u, 5u, 6u});
    WindowBudget budget(3u);
    ScratchByteBufferPool scratchBytePool;
    WindowedByteSourceReader reader(source, budget, scratchBytePool, 4u);
    std::string error;

    std::span<const std::uint8_t> bytes;
    bool hasBytes = false;
    bool read = reader.Next(bytes, hasBytes, &error);
    Require(result, read && hasBytes, "budget.reader.first.next", error.empty() ? "first window missing" : error);
    RequireBytes(result, bytes, {0u, 1u, 2u}, "budget.reader.first");
    RequireWindowBudgetSnapshot(result, CaptureWindowBudget(budget), "budget.reader.first", 3u, 3u);

    read = reader.Next(bytes, hasBytes, &error);
    Require(result, read && hasBytes, "budget.reader.second.next", error.empty() ? "second window missing" : error);
    RequireBytes(result, bytes, {3u, 4u, 5u}, "budget.reader.second");
    RequireWindowBudgetSnapshot(result, CaptureWindowBudget(budget), "budget.reader.second", 3u, 3u);

    read = reader.Next(bytes, hasBytes, &error);
    Require(result, read && hasBytes, "budget.reader.tail.next", error.empty() ? "tail window missing" : error);
    RequireBytes(result, bytes, {6u}, "budget.reader.tail");
    RequireWindowBudgetSnapshot(result, CaptureWindowBudget(budget), "budget.reader.tail", 1u, 3u);

    read = reader.Next(bytes, hasBytes, &error);
    Require(result, read && !hasBytes, "budget.reader.end", error.empty() ? "reader should reach end" : error);
    RequireWindowBudgetSnapshot(result, CaptureWindowBudget(budget), "budget.reader.afterEnd", 0u, 3u);

    PrintResult(result);
    return result.passed;
}

inline bool TestWindowedCopyRangeAndBudget() {
    TestResult result;

    {
        VectorByteSource source(std::vector<std::uint8_t>{10u, 11u, 12u, 13u});
        VectorByteWriter writer;
        WindowBudget budget(1u);
        ScratchByteBufferPool scratchBytePool;
        std::string error;
        const bool copied = CopyByteSourceByWindow(source, writer, budget, scratchBytePool, 0u, &error);
        Require(result, copied, "budget.copy.tinyWindow", error.empty() ? "tiny window copy failed" : error);
        RequireBytes(result, writer.Bytes(), {10u, 11u, 12u, 13u}, "budget.copy.tinyWindow");
        RequireWindowBudgetSnapshot(result, CaptureWindowBudget(budget), "budget.copy.tinyWindow", 0u, 1u);
    }

    {
        VectorByteSource source(std::vector<std::uint8_t>{0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u});
        VectorByteWriter writer;
        WindowBudget budget(2u);
        ScratchByteBufferPool scratchBytePool;
        std::string error;
        const bool copied =
            CopyByteSourceRangeByWindow(source, 2u, 7u, writer, budget, scratchBytePool, 8u, &error);
        Require(result, copied, "budget.copy.range", error.empty() ? "range copy failed" : error);
        RequireBytes(result, writer.Bytes(), {2u, 3u, 4u, 5u, 6u, 7u, 8u}, "budget.copy.range");
        RequireWindowBudgetSnapshot(result, CaptureWindowBudget(budget), "budget.copy.range", 0u, 2u);
    }

    PrintResult(result);
    return result.passed;
}

inline bool TestGeometryDecodeBudgetControlsCacheMode() {
    TestResult result;

    const auto fastConfiguration = CodecControlParamsFactory::MakeDecodeConfiguration(
        DataCodecDecodeOptions{.tier = DataCodecDecodeTier::Fast});
    const auto& fastParams = fastConfiguration.controlParams;
    Require(
        result,
        fastParams.resourceBudget.GeometryDecodeCacheStorageMode() == DecodeStorageMode::Memory,
        "budget.geometryDecode.fast.mode",
        "fast decode should use memory geometry cache");
    Require(
        result,
        fastParams.resourceBudget.GeometryDecodeMemoryCacheLimitBytes() > 0u,
        "budget.geometryDecode.fast.limit",
        "fast decode should set a geometry memory cache limit");
    Require(
        result,
        fastParams.resourceBudget.GeometryDecodeReferenceCacheStorageMode() == DecodeStorageMode::Memory &&
            fastParams.resourceBudget.GeometryDecodeMemoryReferenceLimitBytes() > 0u,
        "budget.geometryDecode.fast.reference",
        "fast decode should configure a separate geometry reference cache limit");
    Require(
        result,
        fastParams.resourceBudget.TopologyDecodeCacheStorageMode() == DecodeStorageMode::Memory,
        "budget.topologyDecode.fast.mode",
        "fast decode should use memory topology cache");
    Require(
        result,
        fastParams.resourceBudget.TopologyDecodeMemoryCacheLimitBytes() > 0u,
        "budget.topologyDecode.fast.limit",
        "fast decode should set a topology memory cache limit");
    Require(
        result,
        fastParams.resourceBudget.TopologyDecodeMemoryInputLimitBytes() > 0u &&
            fastParams.resourceBudget.TopologyDecodeReferenceCacheStorageMode() == DecodeStorageMode::Memory &&
            fastParams.resourceBudget.TopologyDecodeMemoryReferenceLimitBytes() > 0u,
        "budget.topologyDecode.fast.inputAndReference",
        "fast decode should configure separate topology input and reference limits");

    const auto balancedConfiguration = CodecControlParamsFactory::MakeDecodeConfiguration(
        DataCodecDecodeOptions{.tier = DataCodecDecodeTier::Balanced});
    const auto& balancedParams = balancedConfiguration.controlParams;
    Require(
        result,
        fastParams.resourceBudget.AttributeDecodeLaneCount() >
                balancedParams.resourceBudget.AttributeDecodeLaneCount() &&
            fastParams.resourceBudget.AttributeCommitLaneCount() >
                balancedParams.resourceBudget.AttributeCommitLaneCount() &&
            fastParams.resourceBudget.AccessWindowBytes() >
                balancedParams.resourceBudget.AccessWindowBytes() &&
            fastParams.resourceBudget.ActiveWindowBytes() >
                balancedParams.resourceBudget.ActiveWindowBytes() &&
            fastParams.resourceBudget.ScratchRetainedCapacityBytes() >
                balancedParams.resourceBudget.ScratchRetainedCapacityBytes() &&
            fastParams.resourceBudget.DecodeReferenceResidentLimitBytes() >
                balancedParams.resourceBudget.DecodeReferenceResidentLimitBytes() &&
            fastParams.resourceBudget.DecodeReferenceFrameLimit() >
                balancedParams.resourceBudget.DecodeReferenceFrameLimit(),
        "budget.decode.fast.resourceAxis",
        "fast decode should use the aggressive resource profile");

    const auto lowMemoryConfiguration = CodecControlParamsFactory::MakeDecodeConfiguration(
        DataCodecDecodeOptions{.tier = DataCodecDecodeTier::LowMemory});
    const auto& lowMemoryParams = lowMemoryConfiguration.controlParams;
    Require(
        result,
        lowMemoryParams.resourceBudget.GeometryDecodeCacheStorageMode() == DecodeStorageMode::Managed,
        "budget.geometryDecode.lowMemory.mode",
        "low-memory decode should keep managed geometry cache");
    Require(
        result,
        lowMemoryParams.resourceBudget.TopologyDecodeCacheStorageMode() == DecodeStorageMode::Managed,
        "budget.topologyDecode.lowMemory.mode",
        "low-memory decode should keep managed topology cache");
    Require(
        result,
        fastConfiguration.execution.enableParallelStages &&
            !lowMemoryConfiguration.execution.enableParallelStages,
        "budget.decode.performanceExecution",
        "decode performance configurations did not apply their execution profiles");

    const auto auditConfiguration = CodecControlParamsFactory::MakeDecodeConfiguration(
        DataCodecDecodeOptions{
            .tier = DataCodecDecodeTier::Balanced,
            .validationProfile = DataCodecDecodeValidationProfile::Audit,
        });
    Require(
        result,
        auditConfiguration.controlParams.validation.decodeMode == DecodeValidationMode::Strict &&
            auditConfiguration.controlParams.validation.validateTopologyReferences &&
            auditConfiguration.controlParams.validation.validateFloatingPointValues,
        "budget.decode.auditOrthogonal",
        "decode audit configuration was not composed with the performance configuration");

    ByteStoreSession session;
    DecodedGeometryCache cache;
    std::string error;
    Require(
        result,
        cache.Initialize(2u, 3u, session, DecodeStorageMode::Memory, 1024u, &error),
        "budget.geometryDecode.memory.allocate",
        error.empty() ? "memory geometry cache should allocate" : error);
    cache.Release();
    error.clear();
    Require(
        result,
        !cache.Initialize(2u, 3u, session, DecodeStorageMode::Memory, 1u, &error),
        "budget.geometryDecode.memory.reject",
        "memory geometry cache should reject over-limit allocation");
    DecodedIndexCache indexCache;
    error.clear();
    Require(
        result,
        indexCache.Initialize(2u, session, DecodeStorageMode::Memory, 1024u, &error),
        "budget.indexDecode.memory.allocate",
        error.empty() ? "memory index cache should allocate" : error);
    indexCache.Release();
    error.clear();
    Require(
        result,
        !indexCache.Initialize(2u, session, DecodeStorageMode::Memory, 1u, &error),
        "budget.indexDecode.memory.reject",
        "memory index cache should reject over-limit allocation");

    PrintResult(result);
    return result.passed;
}

inline bool TestWasmRuntimeProfileControlsStorageModules() {
    TestResult result;

    const auto encode4GiB = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{.tier = DataCodecEncodeTier::TimePriority},
        DataCodecRuntimeProfile::Wasm4GiB);
    Require(
        result,
        encode4GiB.controlParams.resourceBudget.GeometryEncodeTransferCacheStorageMode() ==
                EncodeStorageMode::Managed &&
            encode4GiB.controlParams.resourceBudget.GeometryEncodeStagingStorageMode() ==
                EncodeStorageMode::Managed &&
            encode4GiB.controlParams.resourceBudget.AttributeEncodeTransferCacheStorageMode() ==
                EncodeStorageMode::Managed &&
            encode4GiB.controlParams.resourceBudget.AttributeEncodeStagingStorageMode() ==
                EncodeStorageMode::Managed &&
            encode4GiB.controlParams.resourceBudget.RemapEncodeStorageMode() ==
                EncodeStorageMode::Managed &&
            encode4GiB.pipelineControl.packageFields.workerCount == 1u &&
            !encode4GiB.execution.enableParallelStages,
        "budget.wasm.encode4GiB.modules",
        "4 GiB Wasm encode should select managed storage modules");

    const auto encode16GiB = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{.tier = DataCodecEncodeTier::TimePriority},
        DataCodecRuntimeProfile::Wasm16GiB);
    Require(
        result,
        encode16GiB.controlParams.resourceBudget.GeometryEncodeTransferCacheStorageMode() ==
                EncodeStorageMode::Memory &&
            encode16GiB.controlParams.resourceBudget.GeometryEncodeStagingStorageMode() ==
                EncodeStorageMode::Memory &&
            encode16GiB.controlParams.resourceBudget.AttributeEncodeTransferCacheStorageMode() ==
                EncodeStorageMode::Memory &&
            encode16GiB.controlParams.resourceBudget.AttributeEncodeStagingStorageMode() ==
                EncodeStorageMode::Memory &&
            encode16GiB.controlParams.resourceBudget.RemapEncodeStorageMode() ==
                EncodeStorageMode::Memory &&
            encode16GiB.pipelineControl.packageFields.workerCount == 4u,
        "budget.wasm.encode16GiB.modules",
        "16 GiB Wasm encode should select memory storage modules");

    const auto decode4GiB = CodecControlParamsFactory::MakeDecodeConfiguration(
        DataCodecDecodeOptions{.tier = DataCodecDecodeTier::Fast},
        DataCodecRuntimeProfile::Wasm4GiB).controlParams;
    Require(
        result,
        decode4GiB.resourceBudget.AttributeDecodePayloadStorageMode() ==
                AttributeDecodePayloadMode::Managed &&
            decode4GiB.resourceBudget.AttributeDecodeCacheStorageMode() == DecodeStorageMode::Managed &&
            decode4GiB.resourceBudget.GeometryDecodeCacheStorageMode() == DecodeStorageMode::Managed &&
            decode4GiB.resourceBudget.TopologyDecodeInputStorageMode() == DecodeStorageMode::Managed &&
            decode4GiB.resourceBudget.TopologyDecodeCacheStorageMode() == DecodeStorageMode::Managed,
        "budget.wasm.decode4GiB.modules",
        "4 GiB Wasm decode should select managed payload and cache modules");

    const auto decode16GiB = CodecControlParamsFactory::MakeDecodeConfiguration(
        DataCodecDecodeOptions{.tier = DataCodecDecodeTier::Fast},
        DataCodecRuntimeProfile::Wasm16GiB).controlParams;
    Require(
        result,
        decode16GiB.resourceBudget.AttributeDecodePayloadStorageMode() ==
                AttributeDecodePayloadMode::OneShotZstd &&
            decode16GiB.resourceBudget.AttributeDecodeCacheStorageMode() == DecodeStorageMode::Memory &&
            decode16GiB.resourceBudget.GeometryDecodeCacheStorageMode() == DecodeStorageMode::Memory &&
            decode16GiB.resourceBudget.TopologyDecodeInputStorageMode() == DecodeStorageMode::Memory &&
            decode16GiB.resourceBudget.TopologyDecodeCacheStorageMode() == DecodeStorageMode::Memory,
        "budget.wasm.decode16GiB.modules",
        "16 GiB Wasm decode should select one-shot payload and memory cache modules");

    const auto wasmLowMemoryConfiguration = CodecControlParamsFactory::MakeDecodeConfiguration(
        DataCodecDecodeOptions{.tier = DataCodecDecodeTier::LowMemory},
        DataCodecRuntimeProfile::Wasm4GiB);
    Require(
        result,
        !wasmLowMemoryConfiguration.decodedFrameCachePolicy.enabled &&
            wasmLowMemoryConfiguration.encodedInputCachePolicy.enabled &&
            wasmLowMemoryConfiguration.encodedInputCachePolicy.residentInputLimit == 3u &&
            wasmLowMemoryConfiguration.encodedInputCachePolicy.residentLimitBytes > 0u,
        "budget.wasm.decodeLowMemory.cachePolicy",
        "Wasm low-memory decode should retain encoded input instead of decoded frames");

    const auto nativeLowMemoryConfiguration = CodecControlParamsFactory::MakeDecodeConfiguration(
        DataCodecDecodeOptions{.tier = DataCodecDecodeTier::LowMemory},
        DataCodecRuntimeProfile::Native);
    Require(
        result,
        nativeLowMemoryConfiguration.decodedFrameCachePolicy.enabled &&
            !nativeLowMemoryConfiguration.encodedInputCachePolicy.enabled,
        "budget.native.decodeLowMemory.cachePolicy",
        "native low-memory decode should rely on file mapping and page cache");

    std::string runtimeError;
    Require(
        result,
        CodecControlParamsFactory::ValidateEncodeRuntimeConstraint(
            encode4GiB.controlParams.resourceBudget,
            DataCodecRuntimeProfile::Wasm4GiB,
            &runtimeError) &&
            CodecControlParamsFactory::ValidateEncodeRuntimeConstraint(
                encode16GiB.controlParams.resourceBudget,
                DataCodecRuntimeProfile::Wasm16GiB,
                &runtimeError) &&
            CodecControlParamsFactory::ValidateDecodeRuntimeConstraint(
                decode4GiB.resourceBudget,
                DataCodecRuntimeProfile::Wasm4GiB,
                &runtimeError) &&
            CodecControlParamsFactory::ValidateDecodeRuntimeConstraint(
                decode16GiB.resourceBudget,
                DataCodecRuntimeProfile::Wasm16GiB,
                &runtimeError),
        "budget.wasm.runtimeBounds",
        runtimeError.empty() ? "Wasm runtime resource bounds are invalid" : runtimeError);

    auto invalid4GiB = encode4GiB.controlParams.resourceBudget;
    invalid4GiB.SetActiveWindowMiB(4096u);
    runtimeError.clear();
    Require(
        result,
        !CodecControlParamsFactory::ValidateEncodeRuntimeConstraint(
            invalid4GiB,
            DataCodecRuntimeProfile::Wasm4GiB,
            &runtimeError),
        "budget.wasm.rejectRuntimeOverflow",
        "Wasm runtime validation should reject an over-capacity resource plan");

    PrintResult(result);
    return result.passed;
}

inline bool TestPolyhedronDecodeBudgetControlsAggregateIndexCaches() {
    TestResult result;
    using datacodec::polyhedron::PolyhedronTopologyStreamCodec;
    using datacodec::polyhedron::PolyhedronTopologyStreamKind;
    using datacodec::polyhedron::PolyhedronTopologyStreamSchedule;
    using datacodec::polyhedron::ValidatePolyhedronDecodedTopologyCacheBudget;

    std::array<PolyhedronTopologyStreamSchedule, 5> schedules{
        PolyhedronTopologyStreamSchedule{
            .kind = PolyhedronTopologyStreamKind::UniqueVertexCounts,
            .codec = PolyhedronTopologyStreamCodec::Varint,
            .elementCount = 2u,
        },
        PolyhedronTopologyStreamSchedule{
            .kind = PolyhedronTopologyStreamKind::CellFaceCounts,
            .codec = PolyhedronTopologyStreamCodec::Varint,
            .elementCount = 3u,
        },
        PolyhedronTopologyStreamSchedule{
            .kind = PolyhedronTopologyStreamKind::FaceVertexCounts,
            .codec = PolyhedronTopologyStreamCodec::Varint,
            .elementCount = 5u,
        },
        PolyhedronTopologyStreamSchedule{
            .kind = PolyhedronTopologyStreamKind::CellUniqueVertexIds,
            .codec = PolyhedronTopologyStreamCodec::Varint,
            .elementCount = 7u,
        },
        PolyhedronTopologyStreamSchedule{
            .kind = PolyhedronTopologyStreamKind::LocalFaceVertexIds,
            .codec = PolyhedronTopologyStreamCodec::SegmentedBitpack,
            .elementCount = 11u,
        },
    };
    const auto aggregateBytes = static_cast<std::uint64_t>(28u * sizeof(datacodec::IndexType));
    std::string error;
    Require(
        result,
        ValidatePolyhedronDecodedTopologyCacheBudget(
            std::span<const PolyhedronTopologyStreamSchedule>(schedules.data(), schedules.size()),
            DecodeStorageMode::Memory,
            aggregateBytes,
            &error),
        "budget.polyhedronDecode.aggregate.allocate",
        error.empty() ? "aggregate polyhedron cache should allocate" : error);
    error.clear();
    Require(
        result,
        !ValidatePolyhedronDecodedTopologyCacheBudget(
            std::span<const PolyhedronTopologyStreamSchedule>(schedules.data(), schedules.size()),
            DecodeStorageMode::Memory,
            aggregateBytes - 1u,
            &error),
        "budget.polyhedronDecode.aggregate.reject",
        "aggregate polyhedron cache should reject over-limit allocation");
    error.clear();
    Require(
        result,
        ValidatePolyhedronDecodedTopologyCacheBudget(
            std::span<const PolyhedronTopologyStreamSchedule>(schedules.data(), schedules.size()),
            DecodeStorageMode::Managed,
            0u,
            &error),
        "budget.polyhedronDecode.managed.noMemoryLimit",
        error.empty() ? "managed polyhedron cache should ignore memory limit" : error);

    PrintResult(result);
    return result.passed;
}

} // namespace datacodec::test::feature_budget

namespace datacodec::test {

inline int RunDataCodecFeatureBudget() {
    if (!feature_budget::TestWindowBudgetLeaseLifecycle() ||
        !feature_budget::TestActiveBudgetRejectsOversizedBlockingAcquire() ||
        !feature_budget::TestByteStoreResidentBudgetAndSealLifecycle() ||
        !feature_budget::TestScratchPoolRetentionLimits() ||
        !feature_budget::TestWindowedByteSourceReaderChunks() ||
        !feature_budget::TestWindowedCopyRangeAndBudget() ||
        !feature_budget::TestGeometryDecodeBudgetControlsCacheMode() ||
        !feature_budget::TestWasmRuntimeProfileControlsStorageModules() ||
        !feature_budget::TestPolyhedronDecodeBudgetControlsAggregateIndexCaches()) {
        return 1;
    }
    std::cout << "DataCodec budget feature tests passed\n";
    return 0;
}

} // namespace datacodec::test

#endif
