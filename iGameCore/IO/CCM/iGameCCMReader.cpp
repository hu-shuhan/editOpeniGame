#include "iGameCCMReader.h"
#include <filesystem>
#include <iGameFileIO.h>
#include "Log/iGameLogger.h"
#include "VTK XML/iGameVTUReader.h"

IGAME_NAMESPACE_BEGIN

bool CCMReader::Parsing() {
    namespace fs = std::filesystem;

    // Get the input .ccm file path
    std::string ccmPath = this->GetFilePath();
    fs::path inputPath(ccmPath);

    // Create a temporary directory for the converted output
    fs::path tempDir = fs::current_path() / "temp";
    if (!fs::exists(tempDir)) { fs::create_directories(tempDir); }

    std::string outputDir = tempDir.string();

    // Locate the converter executable
    std::vector<std::string> exePaths = {
        "Resources\\pyCCMLib\\ccm_to_vtu_converter.exe",
        "ccm_to_vtu_converter.exe"
    };

    std::string exePath;
    bool exeFound = false;
    for (const auto& path : exePaths) {
        std::ifstream file(path);
        if (file.good()) {
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
    fs::path outputFilePath = tempDir / (inputPath.stem().string() + ".vtu");
    std::string outputFile = outputFilePath.string();

    std::string arguments = "--input " + ccmPath + " --output " + outputFile;
    std::string fullCommand = "\"" + exePath + "\" " + arguments;
    IGAME_CORE_DEBUG("[CCMReader] Running command: {}", fullCommand);

    int returnCode = system(fullCommand.c_str());
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
            IGAME_CORE_DEBUG("[CCMReader] Removed temporary file: {}", outputFilePath.string());
        }
        if (fs::exists(tempDir) && fs::is_empty(tempDir)) {
            fs::remove(tempDir);
            IGAME_CORE_DEBUG("[CCMReader] Removed empty temp directory: {}", tempDir.string());
        }
    } catch (const std::exception& e) {
        IGAME_CORE_WARN("[CCMReader] Failed to clean up temporary files: {}", e.what());
    }

    return true;
}

IGAME_NAMESPACE_END
