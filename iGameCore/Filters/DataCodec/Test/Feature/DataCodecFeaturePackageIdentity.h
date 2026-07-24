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
    PackageIdentity first;
    PackageIdentity second;
    std::string generationError;
    const auto firstGenerated = GeneratePackageIdentity(first, &generationError);
    const auto secondGenerated = GeneratePackageIdentity(second, &generationError);
    Require(
        result,
        firstGenerated && secondGenerated && first.IsValid() && second.IsValid() && first != second,
        "packageIdentity.generate",
        generationError.empty() ? "generated package identities are invalid or duplicated" : generationError);

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
        leafWriter.BeginPackageToSink(0u, 0u, leafOutput, &leafError) &&
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
    return result;
}

} // namespace datacodec::test

#endif
