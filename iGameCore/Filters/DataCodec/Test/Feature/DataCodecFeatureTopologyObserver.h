#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATURETOPOLOGYOBSERVER_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATURETOPOLOGYOBSERVER_H

#include "DataCodec/Runtime/Execution/ParallelDecodeTopologyBlockObserver.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <cstddef>
#include <memory>
#include <string>

namespace datacodec::test {

inline DecodedConnectivityTopologyBlock MakeTopologyObserverTestBlock(
    const std::size_t index) {
    return DecodedConnectivityTopologyBlock{
        .blockIndex = index,
        .cellOffset = index,
        .fixedCellSize = 4,
        .connectivity = {0u, 1u, 2u, 3u},
        .cellTypes = {10u},
    };
}

[[nodiscard]] inline TestResult RunDataCodecFeatureTopologyObserver() noexcept {
    TestResult result;
    std::size_t handledBlockCount = 0u;
    ParallelDecodeTopologyBlockObserver successObserver(
        {
            .taskRunner = std::make_shared<InlineParallelTaskRunner>(),
            .workerCount = 4u,
            .maxPendingBlockCount = 8u,
        },
        [](
            const ConnectivityTopologyDecodeInfo& info,
            const std::size_t workerCount,
            std::string*) {
            return info.blockCount == 3u && workerCount == 4u;
        },
        [&handledBlockCount](
            const std::size_t workerIndex,
            DecodedConnectivityTopologyBlock block,
            std::string*) {
            ++handledBlockCount;
            return workerIndex == block.blockIndex % 4u &&
                block.connectivity.size() == 4u;
        });
    std::string observerError;
    const auto successBegin = successObserver.BeginConnectivityTopology({
        .blockCount = 3u,
        .pointCount = 4u,
        .cellCount = 3u,
        .fixedCellSize = 4,
        .hasCellTypes = true,
    }, &observerError);
    const auto successBlock0 = successObserver.ObserveConnectivityBlock(
        MakeTopologyObserverTestBlock(0u), &observerError);
    const auto successBlock1 = successObserver.ObserveConnectivityBlock(
        MakeTopologyObserverTestBlock(1u), &observerError);
    const auto successBlock2 = successObserver.ObserveConnectivityBlock(
        MakeTopologyObserverTestBlock(2u), &observerError);
    const auto successEnd = successObserver.EndConnectivityTopology(&observerError);
    const auto successStats = successObserver.Stats();
    Require(
        result,
        successBegin && successBlock0 && successBlock1 && successBlock2 && successEnd &&
            successObserver.Succeeded(),
        "topologyObserver.success",
        observerError);
    Require(result, successObserver.Error().empty(), "topologyObserver.successError", "successful observer reported an error");
    Require(result, handledBlockCount == 3u, "topologyObserver.handled", "handled block count mismatch");
    Require(result, successStats.observedBlockCount == 3u, "topologyObserver.observed", "observed block count mismatch");
    Require(result, successStats.completedBlockCount == 3u, "topologyObserver.completed", "completed block count mismatch");

    ParallelDecodeTopologyBlockObserver failureObserver(
        {
            .taskRunner = std::make_shared<InlineParallelTaskRunner>(),
            .workerCount = 1u,
            .maxPendingBlockCount = 1u,
        },
        {},
        [](
            std::size_t,
            DecodedConnectivityTopologyBlock,
            std::string* error) {
            if (error != nullptr) {
                *error = "expected topology block failure";
            }
            return false;
        });
    observerError.clear();
    const auto failureBegin = failureObserver.BeginConnectivityTopology({
        .blockCount = 1u,
        .pointCount = 4u,
        .cellCount = 1u,
        .fixedCellSize = 4,
        .hasCellTypes = true,
    }, &observerError);
    const auto failureBlock = failureObserver.ObserveConnectivityBlock(
        MakeTopologyObserverTestBlock(0u), &observerError);
    const auto failureEnd = failureObserver.EndConnectivityTopology(&observerError);
    const auto failureStats = failureObserver.Stats();
    Require(result, !failureObserver.Succeeded(), "topologyObserver.failure", "failing observer reported success");
    Require(
        result,
        failureBegin && !failureBlock && !failureEnd,
        "topologyObserver.failureResult",
        "failing observer did not propagate failure through the observer API");
    Require(
        result,
        failureObserver.Error() == "expected topology block failure",
        "topologyObserver.failureError",
        "failing observer did not preserve the block error");
    Require(result, failureStats.observedBlockCount == 1u, "topologyObserver.failureObserved", "failure observed count mismatch");
    Require(result, failureStats.completedBlockCount == 0u, "topologyObserver.failureCompleted", "failed block was marked complete");
    return result;
}

} // namespace datacodec::test

#endif
