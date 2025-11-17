#include "iGameCASReader.h"
#include <filesystem>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include "Log/iGameLogger.h"

IGAME_NAMESPACE_BEGIN

bool CASReader::Parsing() {
    namespace fs = std::filesystem;

    // ??? .cas ???·??
    std::string casPath = this->GetFilePath();
    fs::path inputPath(casPath);

    // ===== ????????·?? =====
    fs::path tempDir = fs::current_path() / "temp";
    if (!fs::exists(tempDir)) { fs::create_directories(tempDir); }

    // ????¼??? temp ?????
    std::string outputDir = tempDir.string();

    // ????¼?????????????¼
    //std::string outputDir = inputPath.parent_path().string();

    // cas_converter.exe ??·??
    fs::path exePath = "D:/XuJiangjie/editOpeniGame/ThirdParty/Python/pyFluentLib/cas_converter.exe";
    exePath = fs::absolute(exePath);
    //std::string exePath = "./ThirdParty/Python/pyFluentLib/cas_converter.exe";
    //std::string exePath = "cas_converter.exe";


    // ?????????????

    std::string arguments = "--input " + casPath + " --output " + outputDir;
    std::string fullCommand = "\"" + exePath.string() + "\" " + arguments;
    IGAME_CORE_DEBUG("[CASReader] Running command: {}", fullCommand);

    // ???????????
    int returnCode = system(fullCommand.c_str());
    if (returnCode != 0) {
        IGAME_CORE_ERROR("CAS to VTK conversion failed. Return code: {}", returnCode);
        return false;
    }


    // ???????????·??
    fs::path outputFilePath = tempDir / (inputPath.stem().string() + ".vtk");
    std::string outputFile = outputFilePath.string();

    /*std::string outputFile = "";
    fs::path outputFilePath = inputPath.parent_path() / (inputPath.stem().string() + ".vtk");
    outputFile = outputFilePath.string();*/


    // ??????????? VTK ???
    auto obj = iGame::FileIO::ReadFile(outputFile);
    if (!obj) {
        IGAME_CORE_ERROR("Failed to load generated VTK file: {}", outputFile);
        return false;
    }
    IGAME_CORE_DEBUG("DataObjectType: {}", obj->GetDataObjectType());
    auto pc = DynamicCast<PointSet>(obj);
    IGAME_CORE_DEBUG("Points: {}", pc->GetNumberOfPoints());
    // ??????????

    this->SetOutput(obj);
    IGAME_CORE_DEBUG("Successfully converted and loaded: {}", outputFile);

    // ===== ?????????? =====
    try {
        if (fs::exists(outputFilePath)) {
            fs::remove(outputFilePath);
            IGAME_CORE_DEBUG("[Cleanup] Removed temporary file: {}", outputFilePath.string());
        }

        // ??? temp ??????????????
        if (fs::exists(tempDir) && fs::is_empty(tempDir)) {
            fs::remove(tempDir);
            IGAME_CORE_DEBUG("[Cleanup] Removed empty temp directory: {}", tempDir.string());
        }
    } catch (const std::exception& e) {
        IGAME_CORE_WARN("Failed to clean temporary files: {}", e.what());
    }


    return true;
}

IGAME_NAMESPACE_END
