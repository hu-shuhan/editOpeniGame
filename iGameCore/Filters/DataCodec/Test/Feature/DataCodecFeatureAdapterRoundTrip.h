#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREADAPTERROUNDTRIP_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREADAPTERROUNDTRIP_H

#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/API/Entry/DataCodecEncodeEntry.h"
#include "DataCodec/API/Entry/DataCodecDecodeEntry.h"
#include "DataCodec/Test/Adapter/DataCodecTestAdapter.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace datacodec::test {

inline void AppendCodecMessages(
    TestResult& result,
    const std::vector<TelemetryMessageRecord>& messages) {
    for (const auto& message : messages) {
        result.AddDiagnostic(message.origin + ": " + message.text);
    }
}

inline TestResult RunDataCodecFeatureAdapterRoundTrip() noexcept {
    TestResult result;
    try {
        const auto dataset = MakeAdapterRoundTripDataset();
        TestEncodeAdapter encodeAdapter(dataset);

        std::vector<AttributeTarget> targets;
        targets.reserve(dataset.pointFields.size());
        for (std::size_t attrIndex = 0u; attrIndex < dataset.pointFields.size(); ++attrIndex) {
            targets.push_back(AttributeTarget{
                .frameIndex = 0u,
                .blockPath = {},
                .attrIndex = attrIndex,
            });
        }

        auto encodeResult = Encode(EncodeRequest{
            .input = EncodeInput::LeafAdapter(
                &encodeAdapter,
                {},
                dataset.name,
                "PointSet"),
            .output = EncodeOutput::Memory(EncodePackageKind::LeafPackage),
            .attributeSelection = AttributeSelectionMode::AllAvailable,
        });
        AppendCodecMessages(result, encodeResult.messages);
        if (!Require(
                result,
                encodeResult.success && encodeResult.hasEncodedOutput,
                "adapterRoundTrip.encode",
                "DataCodec failed to encode the test adapter")) {
            return result;
        }
        if (!Require(
                result,
                !encodeResult.encodedBytes.empty(),
                "adapterRoundTrip.encodedBytes",
                "DataCodec returned an empty encoded package")) {
            return result;
        }

        auto encodedBytes = encodeResult.encodedBytes;
        TestDecodeAdapter decodeAdapter;
        auto decodeResult = DecodePackage(DecodePackageRequest{
            .inputReader = std::make_shared<MemoryByteRangeReader>(std::move(encodedBytes)),
            .leafAdapter = &decodeAdapter,
            .attributeSelection = AttributeSelectionMode::Explicit,
            .attributeTargets = targets,
        });
        AppendCodecMessages(result, decodeResult.messages);
        if (!Require(
                result,
                decodeResult.success && decodeAdapter.Committed(),
                "adapterRoundTrip.decode",
                "DataCodec failed to decode into the test adapter")) {
            return result;
        }

        Require(
            result,
            decodeAdapter.Mesh() == MeshType::PointSet,
            "adapterRoundTrip.meshType",
            "decoded mesh type does not match the test dataset");
        Require(
            result,
            decodeAdapter.Points() == dataset.points,
            "adapterRoundTrip.geometry",
            "decoded point coordinates do not match the test dataset");

        const auto& decodedAttributes = decodeAdapter.Attributes();
        Require(
            result,
            decodedAttributes.size() == dataset.pointFields.size(),
            "adapterRoundTrip.attributeCount",
            "decoded attribute count does not match the test dataset");
        const auto comparableAttributeCount =
            std::min(decodedAttributes.size(), dataset.pointFields.size());
        for (std::size_t attrIndex = 0u;
             attrIndex < comparableAttributeCount;
             ++attrIndex) {
            const auto& expected = dataset.pointFields[attrIndex];
            const auto& actual = decodedAttributes[attrIndex];
            const auto check = "adapterRoundTrip.attribute[" +
                std::to_string(attrIndex) + "]";
            Require(result, actual.complete, check + ".complete", "attribute was not completed");
            Require(result, actual.metadata.name == expected.name,
                    check + ".name", "attribute name does not match");
            Require(result, actual.metadata.type == expected.role,
                    check + ".role", "attribute role does not match");
            Require(result, actual.metadata.attachmentType == expected.attachment,
                    check + ".attachment", "attribute attachment does not match");
            Require(result, actual.metadata.dataType == DataType::Float32,
                    check + ".dataType", "attribute scalar type does not match");
            Require(result,
                    actual.metadata.dimension == static_cast<std::int32_t>(expected.componentCount),
                    check + ".componentCount",
                    "attribute component count does not match");
            Require(result,
                    static_cast<std::size_t>(actual.metadata.elementCount) == expected.ElementCount(),
                    check + ".elementCount",
                    "attribute element count does not match");
            const auto expectedByteSize = expected.values.size() * sizeof(float);
            const bool bytesMatch = actual.bytes.size() == expectedByteSize &&
                (expectedByteSize == 0u ||
                 std::memcmp(actual.bytes.data(), expected.values.data(), expectedByteSize) == 0);
            Require(result, bytesMatch, check + ".values", "attribute values do not match");
        }

        auto malformedBytes = encodeResult.encodedBytes;
        if (malformedBytes.size() > 1u) {
            malformedBytes.resize(malformedBytes.size() / 2u);
        } else {
            malformedBytes.clear();
        }
        TestDecodeAdapter malformedAdapter;
        const auto malformedResult = DecodePackage(DecodePackageRequest{
            .inputReader = std::make_shared<MemoryByteRangeReader>(std::move(malformedBytes)),
            .leafAdapter = &malformedAdapter,
            .attributeSelection = AttributeSelectionMode::Explicit,
            .attributeTargets = targets,
        });
        AppendCodecMessages(result, malformedResult.messages);
        Require(
            result,
            !malformedResult.success,
            "adapterRoundTrip.malformedPackage",
            "truncated package was accepted by the decoder");
        Require(
            result,
            !malformedResult.messages.empty(),
            "adapterRoundTrip.malformedDiagnostic",
            "truncated package did not produce diagnostic information");
    } catch (const std::exception& exception) {
        result.AddFailure("adapterRoundTrip.exception", exception.what());
    } catch (...) {
        result.AddFailure("adapterRoundTrip.exception", "unknown exception");
    }
    return result;
}

} // datacodec::test命名空间

#endif
