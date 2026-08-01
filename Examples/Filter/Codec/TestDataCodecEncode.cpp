#include <DataCodec/API/Entry/DataCodecEncodeEntry.h>
#include <DataCodec/Filter/Adapter/iGameBlockTreeAdapter.h>
#include <DataCodec/Filter/Adapter/iGameEncodeAdapter.h>
#include <DataCodec/Filter/Adapter/iGameFileByteRangeIO.h>
#include <DataCodec/Filter/Execution/iGameDataCodecThreadPoolTaskRunner.h>
#include <iGameFileIO.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace {

constexpr const char* kSourceFile = "./Models/Tet_Plane.vtk";
constexpr const char* kEncodedFile = "./Models/comp.igc";

} // 匿名命名空间

int main(const int argc, char** argv) {
    if (argc > 3) {
        std::cerr << "usage: testDataCodecEncode [source] [output]\n";
        return 2;
    }
    const std::filesystem::path sourceFile = argc >= 2 ? argv[1] : kSourceFile;
    const std::filesystem::path encodedFile = argc >= 3 ? argv[2] : kEncodedFile;

    // FileIO只负责把外部模型加载成iGame DataObject
    auto object = iGame::FileIO::ReadFile(sourceFile.string());
    if (object == nullptr) {
        std::cerr << "failed to read source model: " << sourceFile.string() << '\n';
        return 1;
    }
    const auto timeFrames = object->PeekTimeFrames();
    if (timeFrames != nullptr && timeFrames->GetTimeNum() > 1u) {
        std::cerr << "this example accepts one resident frame; use the frame-sequence API for time series\n";
        return 1;
    }

    // iGame adapter把DataObject转换成DataCodec可读取的视图
    const auto packageKind = object->HasSubDataObject()
        ? ::datacodec::EncodePackageKind::FramePackage
        : ::datacodec::EncodePackageKind::LeafPackage;
    std::unique_ptr<iGame::iGameEncodeAdapter> leafAdapter;
    std::unique_ptr<iGame::iGameBlockTreeAdapter> blockTreeAdapter;
    ::datacodec::EncodeInput encodeInput;
    if (packageKind == ::datacodec::EncodePackageKind::LeafPackage) {
        if (!iGame::CanCreateiGameEncodeAdapter(object)) {
            std::cerr << "source object is not supported by iGameEncodeAdapter\n";
            return 1;
        }
        leafAdapter = std::make_unique<iGame::iGameEncodeAdapter>(object);
        encodeInput = ::datacodec::EncodeInput::LeafAdapter(leafAdapter.get());
    } else {
        blockTreeAdapter = std::make_unique<iGame::iGameBlockTreeAdapter>(object);
        if (blockTreeAdapter->GetLeafRecords().empty()) {
            std::cerr << "source object has no encodable leaves\n";
            return 1;
        }
        encodeInput = ::datacodec::EncodeInput::BlockTreeAdapter(blockTreeAdapter.get());
    }

    if (!encodedFile.parent_path().empty()) {
        std::error_code directoryError;
        std::filesystem::create_directories(encodedFile.parent_path(), directoryError);
        if (directoryError) {
            std::cerr << "failed to create output directory: "
                      << directoryError.message() << '\n';
            return 1;
        }
    }

    // Options经过工厂展开为codec、pipeline、execution和来源参数组
    auto configuration = ::datacodec::MakeEncodeConfigurationParams({
        .tier = ::datacodec::DataCodecEncodeTier::Balanced,
        .enableCompressionEnhancement = false,
    });

    // ByteRangeOutput和执行资源分别接入文件系统与iGame线程池
    iGame::iGameFileByteRangeOutput output(encodedFile);
    auto result = ::datacodec::Encode({
        .input = std::move(encodeInput),
        .output = ::datacodec::EncodeOutput::ByteRange(output, packageKind),
        .attributeSelection = ::datacodec::AttributeSelectionMode::AllAvailable,
        .configuration = std::move(configuration),
        .executionResources = iGame::MakeDataCodecExecutionResources(),
    });
    if (!result.success || !result.hasEncodedOutput) {
        std::cerr << "DataCodec encode failed\n";
        for (const auto& message : result.messages) {
            std::cerr << message.text << '\n';
        }
        return 1;
    }

    std::cout << "DataCodec source file: " << sourceFile.string() << '\n';
    std::cout << "DataCodec encoded file: " << encodedFile.string() << '\n';
    std::cout << "DataCodec package kind: "
              << (result.packageKind == ::datacodec::EncodePackageKind::FramePackage
                      ? "FramePackage"
                      : "LeafPackage")
              << '\n';
    std::cout << "DataCodec encoded bytes: " << result.encodedByteCount << '\n';
    return 0;
}
