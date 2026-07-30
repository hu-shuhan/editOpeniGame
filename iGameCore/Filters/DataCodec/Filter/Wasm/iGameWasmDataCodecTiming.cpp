#include "DataCodec/Filter/Wasm/iGameWasmDataCodecTiming.h"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>

IGAME_NAMESPACE_BEGIN

std::string BuildiGameWasmTopologyTimingDetail(
    const std::vector<::datacodec::TelemetrySession>& sessions) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2);
    double inputLoadMs = 0.0;
    double blockDecodeMs = 0.0;
    double cellLoopTotalMs = 0.0;
    double cellLoopMaxMs = 0.0;
    double connectivityInputTotalMs = 0.0;
    double connectivityIndicesTotalMs = 0.0;
    double connectivityIndicesMaxMs = 0.0;
    double connectivityAuxiliaryTotalMs = 0.0;
    double connectivityOutputTotalMs = 0.0;
    double topologyCommitMs = 0.0;
    std::size_t cellLoopCount = 0u;
    std::size_t connectivityBlockCount = 0u;
    std::string blockDecodeScope;
    double rootElapsedMs = 0.0;
    for (const auto& session : sessions) {
        if (session.parentRunId == 0u) {
            rootElapsedMs = std::max(rootElapsedMs, session.elapsedMs);
        }
        for (const auto& stage : session.stages) {
            if (stage.name == "DecodeCommitStage.Topology") {
                topologyCommitMs = stage.elapsedMs;
                continue;
            }
            if (stage.category != ::datacodec::TelemetryStageCategory::Topology) { continue; }
            if (stage.name == "TopoDecodeCoreStage.connectivity.block_input_load") {
                inputLoadMs = stage.elapsedMs;
            } else if (stage.name == "TopoDecodeCoreStage.connectivity.block_decode") {
                blockDecodeMs = stage.elapsedMs;
                blockDecodeScope = stage.scope;
            } else if (stage.name == "TopoDecodeCoreStage.connectivity.cell_loop") {
                cellLoopTotalMs += stage.elapsedMs;
                cellLoopMaxMs = std::max(cellLoopMaxMs, stage.elapsedMs);
                ++cellLoopCount;
            } else if (stage.name == "TopoDecodeCoreStage.connectivity.input") {
                connectivityInputTotalMs += stage.elapsedMs;
            } else if (stage.name == "TopoDecodeCoreStage.connectivity.indices") {
                connectivityIndicesTotalMs += stage.elapsedMs;
                connectivityIndicesMaxMs = std::max(connectivityIndicesMaxMs, stage.elapsedMs);
                ++connectivityBlockCount;
            } else if (stage.name == "TopoDecodeCoreStage.connectivity.auxiliary") {
                connectivityAuxiliaryTotalMs += stage.elapsedMs;
            } else if (stage.name == "TopoDecodeCoreStage.connectivity.output") {
                connectivityOutputTotalMs += stage.elapsedMs;
            }
        }
    }
    if (cellLoopCount == 0u && blockDecodeMs == 0.0) { return {}; }
    output << "input=" << inputLoadMs << " ms; block-decode=" << blockDecodeMs << " ms";
    if (!blockDecodeScope.empty()) { output << " (" << blockDecodeScope << ")"; }
    output << "; cell-loop blocks=" << cellLoopCount
           << " total=" << cellLoopTotalMs << " ms"
           << " avg=" << (cellLoopCount != 0u
                ? cellLoopTotalMs / static_cast<double>(cellLoopCount)
                : 0.0) << " ms"
           << " max=" << cellLoopMaxMs << " ms"
           << "; topology-commit=" << topologyCommitMs << " ms"
           << "; session=" << rootElapsedMs << " ms";
    if (connectivityBlockCount != 0u) {
        output << "; connectivity-phases blocks=" << connectivityBlockCount
               << " input-cpu-total=" << connectivityInputTotalMs << " ms"
               << " indices-cpu-total=" << connectivityIndicesTotalMs << " ms"
               << " indices-max=" << connectivityIndicesMaxMs << " ms"
               << " auxiliary-cpu-total=" << connectivityAuxiliaryTotalMs << " ms"
               << " output-cpu-total=" << connectivityOutputTotalMs << " ms";
    }
    return output.str();
}

IGAME_NAMESPACE_END
