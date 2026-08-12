#include "iGameAnsysReader.h"
#include "Log/iGameLogger.h"
#include "VTK XML/iGamePVDReader.h"
#include <filesystem>
#include <fstream>
#include <iGameFileIO.h>

IGAME_NAMESPACE_BEGIN

bool AnsysReader::Parsing() {
    // FileReader::OpenWithWindowsSystem might lock the file, unlock it first
    this->Close();

    namespace fs = std::filesystem;

    // Get the input file path (.rst or .rth)
    std::string ansysPath = this->GetFilePath();
    fs::path inputPath(ansysPath);

    // Create a temporary directory for the converted output
    fs::path tempDir = fs::current_path() / "temp";
    if (!fs::exists(tempDir)) { fs::create_directories(tempDir); }

    std::string outputDir = tempDir.string();

    // Locate the converter executable
    std::vector<std::string> exePaths = {
        "Resources\\pyAnsysLib\\ansys_to_pvd_converter.exe", 
        "ansys_to_pvd_converter.exe"
    };

    std::string exePath;
    bool exeFound = false;
    for (const auto& path: exePaths) {
        std::ifstream file(path);
        if (file.good()) {
            exePath = path;
            exeFound = true;
            break;
        }
    }

    if (!exeFound) {
        IGAME_CORE_ERROR("[AnsysReader] Error: ansys_to_pvd converter not found in any path");
        IGAME_CORE_ERROR(
                "[AnsysReader] Please ensure the converter executable exists in ThirdParty/Python/pyAnsysLib/");
        return false;
    }

    IGAME_CORE_DEBUG("[AnsysReader] Using converter: {}", exePath);

    // Run the converter: ansys_to_pvd.exe --input input.rst --output output.pvd
    fs::path outputFilePath = tempDir / (inputPath.stem().string() + ".pvd");
    std::string outputFile = outputFilePath.string();

    std::string arguments = "--input " + ansysPath + " --output " + outputFile;
    std::string fullCommand = "\"" + exePath + "\" " + arguments;

    

    IGAME_CORE_DEBUG("[AnsysReader] Running command: {}", fullCommand);

    int returnCode = system(fullCommand.c_str());
    if (returnCode != 0) {
        IGAME_CORE_ERROR("[AnsysReader] Ansys to PVD conversion failed. Return code: {}", returnCode);
        return false;
    }

    // Load the generated PVD file
    auto pvdReader = iGame::iGamePVDReader::New();
    pvdReader->SetFilePath(outputFile);
    if (!pvdReader->Execute()) {
        IGAME_CORE_ERROR("[AnsysReader] Failed to read converted PVD file: {}", outputFile);
        return false;
    }

    auto obj = pvdReader->GetOutput();
    if (!obj) {
        IGAME_CORE_ERROR("[AnsysReader] Converter produced empty output");
        return false;
    }

    IGAME_CORE_DEBUG("[AnsysReader] Successfully loaded: {}", outputFile);

    this->SetOutput(obj);

    // Clean up temporary files
    try {
        if (fs::exists(outputFilePath)) {
            fs::remove(outputFilePath);
            IGAME_CORE_DEBUG("[AnsysReader] Removed temporary file: {}", outputFilePath.string());
        }
        // Remove any VTU files in the temp directory matching the base name
        for (const auto& entry: fs::directory_iterator(tempDir)) {
            std::string entryName = entry.path().string();
            if (entryName.find(inputPath.stem().string()) != std::string::npos && entry.path().extension() == ".vtu") {
                fs::remove(entry.path());
                IGAME_CORE_DEBUG("[AnsysReader] Removed temporary file: {}", entry.path().string());
            }
        }
        if (fs::exists(tempDir) && fs::is_empty(tempDir)) {
            fs::remove(tempDir);
            IGAME_CORE_DEBUG("[AnsysReader] Removed empty temp directory: {}", tempDir.string());
        }
    } catch (const std::exception& e) {
        IGAME_CORE_WARN("[AnsysReader] Failed to clean up temporary files: {}", e.what());
    }

    return true;
}

IGAME_NAMESPACE_END