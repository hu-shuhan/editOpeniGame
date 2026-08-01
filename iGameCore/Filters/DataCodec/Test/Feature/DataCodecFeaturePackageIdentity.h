#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREPACKAGEIDENTITY_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREPACKAGEIDENTITY_H

#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Storage/FramePackage/FramePackageIO.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageByteWriter.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageIO.h"
#include "DataCodec/Storage/Package/PackageBinaryHeader.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace datacodec::test {

inline std::vector<std::uint8_t> MakePackageIdentityTestHeader(
    const std::uint32_t magic,
    const std::uint16_t version,
    const PackageIdentity identity) {
    std::vector<std::uint8_t> bytes;
    detail::AppendScalar(bytes, magic);
    detail::AppendScalar(bytes, version);
    detail::AppendScalar(bytes, identity.high);
    detail::AppendScalar(bytes, identity.low);
    return bytes;
}

inline bool CheckPackageIdentityTestHeader(
    const std::uint32_t magic,
    const std::uint16_t version,
    const PackageIdentity identity,
    const PackageBinaryFormat expectedFormat) {
    MemoryByteRangeReader reader(MakePackageIdentityTestHeader(magic, version, identity));
    PackageInspection inspection;
    std::string error;
    return InspectPackage(reader, inspection, &error) &&
        inspection.format == expectedFormat &&
        inspection.version == version &&
        inspection.identity == identity &&
        inspection.sourceIdentity.IsStable();
}

[[nodiscard]] inline TestResult RunDataCodecFeaturePackageIdentity() noexcept {
    TestResult result;
    PackageIdentityBuilder firstBuilder("package-identity-test");
    firstBuilder.AddString("same-name");
    firstBuilder.AddUnsigned(1234u);
    const auto first = firstBuilder.Finish();
    PackageIdentityBuilder repeatedBuilder("package-identity-test");
    repeatedBuilder.AddString("same-name");
    repeatedBuilder.AddUnsigned(1234u);
    const auto repeated = repeatedBuilder.Finish();
    PackageIdentityBuilder secondBuilder("package-identity-test");
    secondBuilder.AddString("other-name");
    secondBuilder.AddUnsigned(1234u);
    const auto second = secondBuilder.Finish();
    Require(
        result,
        first.IsValid() && second.IsValid() && first == repeated && first != second,
        "packageIdentity.deterministic",
        "package identity is not deterministic or name-sensitive");

    constexpr std::uint64_t kTenGiB = 10ull * 1024ull * 1024ull * 1024ull;
    const auto largeFileSampleRanges = BuildPackageIdentitySampleRanges(kTenGiB);
    std::uint64_t sampledBytes = 0u;
    for (const auto& range : largeFileSampleRanges) {
        sampledBytes += static_cast<std::uint64_t>(range.byteCount);
    }
    Require(
        result,
        sampledBytes <= 192u * 1024u,
        "packageIdentity.largeFileSampleBudget",
        "10 GiB sparse identity sampling exceeds the fixed budget");

    const auto makeContentLeaf = [](const std::uint8_t attributeMarker) {
        constexpr std::size_t kFieldBytes = 16u * 1024u;
        std::vector<std::uint8_t> topologyBytes(kFieldBytes, 0x5au);
        std::vector<std::uint8_t> attributeBytes(kFieldBytes, 0x3cu);
        attributeBytes.front() = attributeMarker;
        LeafPackage leaf;
        leaf.path = "content-identity-leaf";
        leaf.rawFieldBytes = leafpackagewire::ComputeRawLeafPackageSize(
            2u,
            topologyBytes.size() + attributeBytes.size());
        leaf.fields.push_back(LeafPackage::Field{
            .type = FieldType::Topology,
            .compressionType = EncodedFieldCompressionType::None,
            .rawSize = topologyBytes.size(),
            .source = std::make_shared<bytestore::VectorByteSource>(std::move(topologyBytes)),
        });
        leaf.fields.push_back(LeafPackage::Field{
            .type = FieldType::Attribute,
            .compressionType = EncodedFieldCompressionType::None,
            .rawSize = attributeBytes.size(),
            .source = std::make_shared<bytestore::VectorByteSource>(std::move(attributeBytes)),
        });
        return leaf;
    };
    auto firstContentLeaf = makeContentLeaf(0x11u);
    auto secondContentLeaf = makeContentLeaf(0x22u);
    PackageIdentity firstContentIdentity;
    PackageIdentity secondContentIdentity;
    std::string contentIdentityError;
    const auto firstContentComputed = LeafPackageIO::ComputeLeafPackageIdentity(
        firstContentLeaf,
        firstContentIdentity,
        &contentIdentityError);
    const auto secondContentComputed = LeafPackageIO::ComputeLeafPackageIdentity(
        secondContentLeaf,
        secondContentIdentity,
        &contentIdentityError);
    Require(
        result,
        firstContentComputed && secondContentComputed &&
            firstContentIdentity != secondContentIdentity,
        "packageIdentity.attributeContent",
        contentIdentityError.empty()
            ? "attribute change did not alter package identity"
            : contentIdentityError);

    const auto identityHex = PackageIdentityToHex(first);
    PackageIdentity parsed;
    Require(
        result,
        identityHex.size() == 32u && TryParsePackageIdentityHex(identityHex, parsed) && parsed == first,
        "packageIdentity.textRoundTrip",
        "package identity text round trip failed");

    Require(
        result,
        CheckPackageIdentityTestHeader(
            leafpackagewire::kLeafPackageMagic,
            leafpackagewire::kLeafPackageVersion,
            first,
            PackageBinaryFormat::LeafPackage),
        "packageIdentity.leafHeader",
        "leaf package identity header read failed");
    Require(
        result,
        CheckPackageIdentityTestHeader(
            framepackagewire::kFramePackageMagic,
            framepackagewire::kFramePackageVersion,
            second,
            PackageBinaryFormat::FramePackage),
        "packageIdentity.frameHeader",
        "frame package identity header read failed");

    auto versionMismatchBytes = MakePackageIdentityTestHeader(
        leafpackagewire::kLeafPackageMagic,
        2u,
        first);
    MemoryByteRangeReader versionMismatchReader(std::move(versionMismatchBytes));
    PackageInspection versionMismatchInspection;
    std::string versionMismatchError;
    const auto versionMismatchRead = InspectPackage(
        versionMismatchReader,
        versionMismatchInspection,
        &versionMismatchError);
    Require(
        result,
        !versionMismatchRead,
        "packageIdentity.versionMismatch",
        "mismatched package version was accepted");
    Require(
        result,
        versionMismatchError == "版本不符合",
        "packageIdentity.versionMismatchError",
        "package version rejection did not use the unified version message");

    const auto sourceIdentity = MakePackageDecodeSourceIdentity(
        first,
        leafpackagewire::kLeafPackageVersion,
        1234u);
    Require(result, sourceIdentity.IsStable(), "packageIdentity.cacheStable", "package source identity is not stable");
    Require(
        result,
        sourceIdentity.stableId.find(identityHex) != std::string::npos,
        "packageIdentity.cacheId",
        "package source identity does not contain the package identity");
    Require(
        result,
        sourceIdentity.revision == "igdc-v1:bytes:1234",
        "packageIdentity.cacheRevision",
        "package source identity revision mismatch");

    MemoryByteRangeOutput leafOutput;
    LeafPackageByteWriter leafWriter;
    std::string leafError;
    const auto leafWritten =
        leafWriter.BeginPackageToSink(
            0u,
            0u,
            leafOutput,
            &leafError,
            "identity-test") &&
        leafWriter.EndPackage(&leafError);
    Require(result, leafWritten, "packageIdentity.leafWrite", leafError.empty() ? "leaf package write failed" : leafError);
    if (leafWritten) {
        auto leafReader = std::make_shared<MemoryByteRangeReader>(leafOutput.Bytes());
        LeafPackage decodedLeaf;
        const auto leafRead = LeafPackageIO::ReadFromByteRange(
            leafReader,
            0u,
            leafReader->ByteSize(),
            decodedLeaf,
            &leafError);
        Require(result, leafRead, "packageIdentity.leafRead", leafError.empty() ? "leaf package read failed" : leafError);
        Require(result, decodedLeaf.identity.IsValid(), "packageIdentity.leafRoundTrip", "decoded leaf identity is invalid");
    }

    MemoryByteRangeOutput repeatedLeafOutput;
    LeafPackageByteWriter repeatedLeafWriter;
    const auto repeatedLeafWritten =
        repeatedLeafWriter.BeginPackageToSink(
            0u,
            0u,
            repeatedLeafOutput,
            &leafError,
            "identity-test") &&
        repeatedLeafWriter.EndPackage(&leafError);
    PackageIdentity firstLeafIdentity;
    PackageIdentity repeatedLeafIdentity;
    const auto deterministicLeafIdentity =
        leafWritten && repeatedLeafWritten &&
        TryReadPackageIdentityPrefix(leafOutput.Bytes(), firstLeafIdentity) &&
        TryReadPackageIdentityPrefix(repeatedLeafOutput.Bytes(), repeatedLeafIdentity) &&
        firstLeafIdentity == repeatedLeafIdentity;
    Require(
        result,
        deterministicLeafIdentity,
        "packageIdentity.leafDeterministic",
        "equivalent leaf packages produced different identities");

    FramePackage frame;
    frame.frameIndex = 7u;
    frame.timeValue = 1.5f;
    frame.rootName = "identity-test";
    MemoryByteRangeOutput frameOutput;
    std::string frameError;
    const auto frameWritten = FramePackageIO::WriteToSink(frame, {}, frameOutput, nullptr, &frameError);
    Require(result, frameWritten, "packageIdentity.frameWrite", frameError.empty() ? "frame package write failed" : frameError);
    if (frameWritten) {
        MemoryByteRangeReader frameReader(frameOutput.Bytes());
        FramePackage decodedFrame;
        const auto frameRead = FramePackageIO::ReadMetadata(frameReader, decodedFrame, &frameError);
        Require(result, frameRead, "packageIdentity.frameRead", frameError.empty() ? "frame package read failed" : frameError);
        Require(result, decodedFrame.identity.IsValid(), "packageIdentity.frameIdentity", "decoded frame identity is invalid");
        Require(result, decodedFrame.frameIndex == frame.frameIndex, "packageIdentity.frameIndex", "decoded frame index mismatch");
        Require(result, decodedFrame.rootName == frame.rootName, "packageIdentity.frameName", "decoded frame name mismatch");
    }

    MemoryByteRangeOutput repeatedFrameOutput;
    const auto repeatedFrameWritten = FramePackageIO::WriteToSink(
        frame,
        {},
        repeatedFrameOutput,
        nullptr,
        &frameError);
    auto renamedFrame = frame;
    renamedFrame.rootName = "identity-best";
    MemoryByteRangeOutput renamedFrameOutput;
    const auto renamedFrameWritten = FramePackageIO::WriteToSink(
        renamedFrame,
        {},
        renamedFrameOutput,
        nullptr,
        &frameError);
    PackageIdentity firstFrameIdentity;
    PackageIdentity repeatedFrameIdentity;
    PackageIdentity renamedFrameIdentity;
    const auto frameIdentityContract =
        frameWritten && repeatedFrameWritten && renamedFrameWritten &&
        TryReadPackageIdentityPrefix(frameOutput.Bytes(), firstFrameIdentity) &&
        TryReadPackageIdentityPrefix(repeatedFrameOutput.Bytes(), repeatedFrameIdentity) &&
        TryReadPackageIdentityPrefix(renamedFrameOutput.Bytes(), renamedFrameIdentity) &&
        firstFrameIdentity == repeatedFrameIdentity &&
        firstFrameIdentity != renamedFrameIdentity;
    Require(
        result,
        frameIdentityContract,
        "packageIdentity.frameDeterministic",
        "frame package identity is not deterministic or name-sensitive");
    return result;
}

} // namespace datacodec::test

#endif
