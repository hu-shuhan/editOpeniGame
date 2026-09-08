#include "iGameCCMReader.h"
#include "iGameExternalProcess.h"
#include "Log/iGameLogger.h"
#include "VTK XML/iGameVTUReader.h"
#include <filesystem>

IGAME_NAMESPACE_BEGIN

bool CCMReader::Parsing() {
    namespace fs = std::filesystem;

    // Get the input .ccm file path
    std::string ccmPath = this->GetFilePath();
    // m_FilePath 为 UTF-8，先转成本地宽字符 path，避免窄字符串按 ACP 解码产生乱码
    fs::path inputPath = FileSystem::PathFromUtf8(ccmPath);

    // Create a temporary directory for the converted output
    fs::path tempDir = fs::current_path() / "temp";
    if (!fs::exists(tempDir)) { fs::create_directories(tempDir); }

    // Locate the converter executable
    std::vector<std::string> exePaths = {
        "Resources\\pyCCMLib\\ccm_to_vtu_converter.exe",
        "ccm_to_vtu_converter.exe"
    };

    std::string exePath;
    bool exeFound = false;
    for (const auto& path : exePaths) {
        if (fs::exists(FileSystem::PathFromUtf8(path))) {
            exePath = path;
            exeFound = true;
            break;
        }
    }

    if (!exeFound) {
        IGAME_CORE_ERROR("[CCMReader] Error: ccm_to_vtu_converter not found in any path");
        IGAME_CORE_ERROR("[CCMReader] Please ensure the converter executable exists in ThirdParty/Python/pyCCMLib/");
        return false;
    }

    IGAME_CORE_DEBUG("[CCMReader] Using converter: {}", exePath);

    // Run the converter: ccm_to_vtu_converter --input input.ccm --output output.vtu
    // 直接以宽字符命令行启动转换器（不经 cmd.exe），参数加引号，中文/空格路径均安全。
    fs::path outputFileName = inputPath.stem();
    outputFileName += ".vtu";
    fs::path outputFilePath = tempDir / outputFileName;
    // 公共 API 边界统一为 UTF-8
    std::string outputFile = FileSystem::PathToUtf8(outputFilePath);

    std::vector<std::string> arguments = {"--input", ccmPath, "--output", outputFile};
    IGAME_CORE_DEBUG("[CCMReader] Running converter: {} --input {} --output {}", exePath, ccmPath, outputFile);

    int returnCode = 0;
    if (!ExternalProcess::Run(exePath, arguments, returnCode)) {
        IGAME_CORE_ERROR("[CCMReader] Failed to start converter: {}", exePath);
        return false;
    }
    if (returnCode != 0) {
        IGAME_CORE_ERROR("[CCMReader] CCM to VTU conversion failed. Return code: {}", returnCode);
        return false;
    }

    // Load the generated VTU file
    auto vtuReader = iGame::iGameVTUReader::New();
    vtuReader->SetFilePath(outputFile);
    if (!vtuReader->Execute()) {
        IGAME_CORE_ERROR("[CCMReader] Failed to read converted VTU file: {}", outputFile);
        return false;
    }

    auto obj = vtuReader->GetOutput();
    if (!obj) {
        IGAME_CORE_ERROR("[CCMReader] Converter produced empty output");
        return false;
    }

    IGAME_CORE_DEBUG("[CCMReader] Successfully loaded: {}", outputFile);

    this->SetOutput(obj);

    // Clean up temporary files
    try {
        if (fs::exists(outputFilePath)) {
            fs::remove(outputFilePath);
            IGAME_CORE_DEBUG("[CCMReader] Removed temporary file: {}", FileSystem::PathToUtf8(outputFilePath));
        }
        if (fs::exists(tempDir) && fs::is_empty(tempDir)) {
            fs::remove(tempDir);
            IGAME_CORE_DEBUG("[CCMReader] Removed empty temp directory: {}", FileSystem::PathToUtf8(tempDir));
        }
    } catch (const std::exception& e) {
        IGAME_CORE_WARN("[CCMReader] Failed to clean up temporary files: {}", e.what());
    }

    return true;
}

IGAME_NAMESPACE_END
