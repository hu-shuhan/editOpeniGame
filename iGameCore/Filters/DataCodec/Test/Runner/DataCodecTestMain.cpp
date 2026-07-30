#include "DataCodec/Filter/Test/Feature/iGameDataCodecFeaturePlaybackSession.h"
#include "DataCodec/Filter/Test/Feature/iGameDataCodecFeaturePreparedSurfaceAttributes.h"
#include "DataCodec/Filter/Test/Feature/iGameDataCodecFeatureRemap.h"
#include "DataCodec/Filter/Adapter/iGameDataCodecAttributeCatalog.h"
#include "DataCodec/Test/Suite/DataCodecTestSuite.h"
#include "DataCodec/Filter/Telemetry/iGameDataCodecTelemetryCapture.h"
#include "IGDC/iGameIGDCWriter.h"
#include "iGameFilterIncludes.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

void PrintResult(const datacodec::test::TestResult& result) {
    for (const auto& diagnostic : result.diagnostics) {
        std::cout << diagnostic << '\n';
    }
    for (const auto& failure : result.failures) {
        std::cerr << failure.check << ": " << failure.message << '\n';
    }
}

int WriteBrowserFixture(const std::filesystem::path& outputPath) {
    const auto source = iGame::datacodec_test::BuildSyntheticSurfaceSmokeObject();
    auto writer = iGame::IGDCWriter::New();
    std::vector<iGame::DataCodecEncodeAttributeDescriptor> descriptors;
    if (source == nullptr || writer == nullptr ||
        !iGame::CollectDataCodecEncodeRepresentativeAttributeCatalog(source, descriptors)) {
        std::cerr << "failed to collect browser fixture attributes\n";
        return 1;
    }

    std::vector<datacodec::AttributeTarget> targets;
    targets.reserve(descriptors.size());
    for (const auto& descriptor : descriptors) { targets.push_back(descriptor.target); }
    if (targets.empty()) {
        std::cerr << "browser fixture has no attributes\n";
        return 1;
    }

    writer->SetAttributeTargets(std::move(targets));
    writer->SetFilePath(outputPath.string());
    iGame::iGameDataCodecTelemetryCapture recordSinks;
    recordSinks.CaptureSessions(
        ::datacodec::kRunLifecycleRecordMask |
        ::datacodec::RunRecordKind::Message);
    writer->SetTelemetrySink(recordSinks.Sink());
    if (!writer->WriteToFile(source, outputPath.string())) {
        for (const auto& message : recordSinks.SnapshotMessages()) {
            std::cerr << message.text << '\n';
        }
        std::cerr << "failed to write browser fixture\n";
        return 1;
    }
    for (const auto& path : writer->GetWrittenFilePaths()) {
        std::cout << path << '\n';
    }
    return 0;
}

}

int main(const int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--write-browser-fixture") {
        return WriteBrowserFixture(argv[2]);
    }

    const auto coreResult = datacodec::test::RunDataCodecSelfTest();
    PrintResult(coreResult);
    if (!coreResult.passed) {
        std::cerr << "DataCodec core tests failed\n";
        return 1;
    }

    if (iGame::datacodec_test::RunDataCodecFeatureRemap() != 0) {
        std::cerr << "iGame DataCodec remap tests failed\n";
        return 1;
    }

    if (iGame::datacodec_test::RunDataCodecFeaturePlaybackSession(argc, argv) != 0) {
        std::cerr << "iGame DataCodec integration tests failed\n";
        return 1;
    }

    if (iGame::datacodec_test::RunDataCodecFeaturePreparedSurfaceAttributes() != 0) {
        std::cerr << "iGame prepared surface attribute tests failed\n";
        return 1;
    }

    std::cout << "DataCodec tests passed\n";
    return 0;
}
