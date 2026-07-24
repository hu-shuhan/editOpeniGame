#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATURECELLGRAPHTOPOLOGY_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATURECELLGRAPHTOPOLOGY_H

#include "DataCodec/Codec/Topology/Connectivity/ConnectivityTopologyCodec.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace datacodec::test {

inline bool RequireTopologyGrammarRoundTrip(
    TestResult& result,
    const std::string& name,
    const std::span<const IndexType> connectivity,
    const std::span<const IndexType> cellSizes,
    const std::size_t cellCount,
    const int fixedCellSize,
    const std::size_t pointCount,
    std::vector<std::uint8_t>& encoded) {
    std::string error;
    if (!Require(
            result,
            topocodec::blockcodec::EncodeConnectivity(
                connectivity,
                cellSizes,
                cellCount,
                fixedCellSize,
                pointCount,
                encoded,
                &error),
            name + ".encode",
            error.empty() ? "topology grammar encode failed" : error)) {
        return false;
    }
    std::vector<IndexType> decoded;
    error.clear();
    return Require(
        result,
        topocodec::blockcodec::DecodeConnectivity(
            encoded,
            cellSizes,
            pointCount,
            cellCount,
            connectivity.size(),
            fixedCellSize,
            decoded,
            &error) && decoded.size() == connectivity.size() &&
            std::equal(decoded.begin(), decoded.end(), connectivity.begin()),
        name + ".roundTrip",
        error.empty() ? "topology grammar round-trip values do not match" : error);
}

[[nodiscard]] inline TestResult RunDataCodecFeatureCellGraphTopology() noexcept {
    TestResult result;
    try {
        constexpr std::array<IndexType, 3u> goldenConnectivity{{0u, 1u, 2u}};
        constexpr std::array<std::uint8_t, 24u> goldenBytes{{
            'U', 'I', 'T', 'P', 1u, 4u,
            1u, 3u, 0u, 1u, 0u,
            2u, 2u, 1u, 2u, 0u, 0u, 0u, 0u, 0u,
            0u, 0xf0u, 0u, 0u,
        }};
        std::vector<std::uint8_t> encoded;
        if (RequireTopologyGrammarRoundTrip(
                result,
                "topologyGrammar.golden",
                goldenConnectivity,
                {},
                1u,
                3,
                3u,
                encoded)) {
            Require(
                result,
                encoded.size() == goldenBytes.size() &&
                    std::equal(encoded.begin(), encoded.end(), goldenBytes.begin()),
                "topologyGrammar.golden.bytes",
                "topology grammar golden bytes changed");
        }

        constexpr std::array<IndexType, 6u> sharedEdgeConnectivity{{
            0u, 1u, 2u,
            0u, 1u, 3u,
        }};
        topocodec::blockcodec::ResetTopologyEncodeStats();
        if (RequireTopologyGrammarRoundTrip(
                result,
                "topologyGrammar.sharedEdge",
                sharedEdgeConnectivity,
                {},
                2u,
                3,
                4u,
                encoded)) {
            const auto stats = topocodec::blockcodec::SnapshotTopologyEncodeStats();
            Require(
                result,
                stats.seedEdgeHitCount == 1u && stats.seedEdgeMissCount == 1u,
                "topologyGrammar.sharedEdge.stats",
                "topology grammar did not reuse the shared seed edge");
        }

        constexpr std::array<IndexType, 11u> variableConnectivity{{
            100u,
            100u, 101u,
            100u, 101u, 105u,
            105u, 101u, 100u, 200u, 201u,
        }};
        constexpr std::array<IndexType, 4u> variableCellSizes{{1u, 2u, 3u, 5u}};
        if (RequireTopologyGrammarRoundTrip(
                result,
                "topologyGrammar.variable",
                variableConnectivity,
                variableCellSizes,
                variableCellSizes.size(),
                0,
                256u,
                encoded)) {
            std::vector<std::uint8_t> deterministic;
            std::string error;
            Require(
                result,
                topocodec::blockcodec::EncodeConnectivity(
                    variableConnectivity,
                    variableCellSizes,
                    variableCellSizes.size(),
                    0,
                    256u,
                    deterministic,
                    &error) && deterministic == encoded,
                "topologyGrammar.deterministic",
                error.empty() ? "topology grammar output is not deterministic" : error);
        }

        std::vector<std::uint8_t> byteSymbols(8192u, 0u);
        for (std::size_t index = 0u; index < 1024u; ++index) {
            byteSymbols[index] = static_cast<std::uint8_t>(index & 0xffu);
        }
        std::vector<std::uint8_t> byteStream;
        std::string byteStreamError;
        if (Require(
                result,
                topocodec::blockcodec::EncodeSymbolStream<256u>(
                    byteSymbols,
                    byteStream,
                    &byteStreamError) &&
                    !byteStream.empty() &&
                    byteStream.front() == static_cast<std::uint8_t>(
                        topocodec::blockcodec::SymbolStreamCodec::CanonicalHuffman),
                "topologyGrammar.byteStream.encode",
                byteStreamError.empty()
                    ? "topology byte-symbol stream did not select huffman"
                    : byteStreamError)) {
            topocodec::blockcodec::SymbolStreamReader<256u> byteReader;
            byteStreamError.clear();
            bool decodedByteStream = byteReader.Reset(
                byteStream,
                byteSymbols.size(),
                &byteStreamError);
            for (std::size_t index = 0u; decodedByteStream && index < byteSymbols.size(); ++index) {
                std::uint8_t value = 0u;
                decodedByteStream = byteReader.Read(value, &byteStreamError) &&
                    value == byteSymbols[index];
            }
            decodedByteStream = decodedByteStream && byteReader.Finish(&byteStreamError);
            Require(
                result,
                decodedByteStream,
                "topologyGrammar.byteStream.roundTrip",
                byteStreamError.empty()
                    ? "topology byte-symbol stream round-trip failed"
                    : byteStreamError);
        }

        auto malformed = encoded;
        malformed.pop_back();
        std::vector<IndexType> decoded;
        std::string error;
        Require(
            result,
            !topocodec::blockcodec::DecodeConnectivity(
                malformed,
                variableCellSizes,
                256u,
                variableCellSizes.size(),
                variableConnectivity.size(),
                0,
                decoded,
                &error),
            "topologyGrammar.malformed.truncated",
            "topology grammar decoder accepted a truncated payload");

        malformed = encoded;
        malformed[0] = 0u;
        error.clear();
        Require(
            result,
            !topocodec::blockcodec::DecodeConnectivity(
                malformed,
                variableCellSizes,
                256u,
                variableCellSizes.size(),
                variableConnectivity.size(),
                0,
                decoded,
                &error),
            "topologyGrammar.malformed.magic",
            "topology grammar decoder accepted an invalid magic");

        error.clear();
        Require(
            result,
            !topocodec::blockcodec::EncodeConnectivity(
                goldenConnectivity,
                {},
                1u,
                3,
                2u,
                malformed,
                &error),
            "topologyGrammar.range",
            "topology grammar encoder accepted an out-of-range point id");
    } catch (const std::exception& exception) {
        result.AddFailure("topologyGrammar.exception", exception.what());
    } catch (...) {
        result.AddFailure("topologyGrammar.exception", "unknown exception");
    }
    return result;
}

} // namespace datacodec::test

#endif
