#include "DataCodec/Filter/Wasm/iGameWasmDataCodecDiagnostics.h"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>

IGAME_NAMESPACE_BEGIN

::datacodec::RunRecordMask iGameWasmDataCodecDiagnosticsSink::Interests() const noexcept {
    return ::datacodec::RunRecordKind::RunEnd |
        ::datacodec::RunRecordKind::StageTiming;
}

void iGameWasmDataCodecDiagnosticsSink::Submit(
    const ::datacodec::RunRecord& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (const auto* stage = std::get_if<::datacodec::RunStageTimingRecord>(&record)) {
        m_stages.push_back(stage->stage);
        return;
    }
    const auto* runEnd = std::get_if<::datacodec::RunEndRecord>(&record);
    if (runEnd != nullptr && runEnd->run.parentRunId == 0u) {
        m_rootElapsedMs = std::max(m_rootElapsedMs, runEnd->elapsedMs);
    }
}

std::string iGameWasmDataCodecDiagnosticsSink::BuildTopologyTimingDetail() const {
    std::lock_guard<std::mutex> lock(m_mutex);
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
    for (const auto& stage : m_stages) {
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
           << "; session=" << m_rootElapsedMs << " ms";
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
