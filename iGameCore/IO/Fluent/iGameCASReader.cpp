#include "iGameCASReader.h"
#include <filesystem>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include <iostream>

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
    std::cout << "[CASReader] Running command: " << fullCommand << std::endl;

    // ???????????
    int returnCode = system(fullCommand.c_str());
    if (returnCode != 0) {
        std::cerr << "[Error] CAS to VTK conversion failed. Return code: " << returnCode << std::endl;
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
        std::cerr << "[Error] Failed to load generated VTK file: " << outputFile << std::endl;
        return false;
    }
    std::cout << obj->GetDataObjectType() << '\n';
    auto pc = DynamicCast<PointSet>(obj);
    std::cout << pc->GetNumberOfPoints() << std::endl;
    // ??????????

    this->SetOutput(obj);
    std::cout << "[Info] Successfully converted and loaded: " << outputFile << std::endl;

    // ===== ?????????? =====
    try {
        if (fs::exists(outputFilePath)) {
            fs::remove(outputFilePath);
            std::cout << "[Cleanup] Removed temporary file: " << outputFilePath << std::endl;
        }

        // ??? temp ??????????????
        if (fs::exists(tempDir) && fs::is_empty(tempDir)) {
            fs::remove(tempDir);
            std::cout << "[Cleanup] Removed empty temp directory: " << tempDir << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[Warning] Failed to clean temporary files: " << e.what() << std::endl;
    }


    return true;
}

IGAME_NAMESPACE_END
