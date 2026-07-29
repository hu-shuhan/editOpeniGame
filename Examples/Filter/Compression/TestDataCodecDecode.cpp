#include <IGDC/iGameIGDCReader.h>
#include <DataCodec/Log/Telemetry/Sinks/ConsoleTelemetrySink.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace {

constexpr const char* kEncodedFile = "./Models/DataCodec/Tet_Plane.igc";

void PrintMessages(const iGame::IGDCReader::Pointer& reader) {
    if (reader == nullptr) {
        return;
    }
    for (const auto& message : reader->GetMessages()) {
        std::cerr << message.text << '\n';
    }
}

} // 匿名命名空间

int main(const int argc, char** argv) {
    if (argc > 3) {
        std::cerr << "usage: testDataCodecDecode [input] [timing]\n";
        return 2;
    }
    const std::filesystem::path encodedFile = argc >= 2 ? argv[1] : kEncodedFile;
    const bool timingEnabled = argc >= 3 && std::string(argv[2]) == "timing";
    if (argc >= 3 && !timingEnabled) {
        std::cerr << "unknown DataCodec decode option: " << argv[2] << '\n';
        return 2;
    }

    if (!std::filesystem::is_regular_file(encodedFile)) {
        std::cerr << "encoded DataCodec file is missing: " << encodedFile.string() << '\n';
        std::cerr << "run the DataCodec encode example before this decode example\n";
        return 1;
    }

    auto reader = iGame::IGDCReader::New();
    if (timingEnabled) {
        auto configuration = ::datacodec::MakeDecodeConfigurationParams(
            ::datacodec::DataCodecDecodeOptions{
                .tier = ::datacodec::DataCodecDecodeTier::Balanced,
            });
        reader->SetDecodeControls(configuration);
        reader->SetRunRecordSink(std::make_shared<::datacodec::ConsoleTelemetrySink>());
    }
    auto object = reader->ReadFile(encodedFile.string());
    if (object == nullptr) {
        std::cerr << "failed to decode DataCodec package: " << encodedFile.string() << '\n';
        PrintMessages(reader);
        return 1;
    }

    std::cout << "DataCodec decoded file: " << encodedFile.string() << '\n';
    return 0;
}
