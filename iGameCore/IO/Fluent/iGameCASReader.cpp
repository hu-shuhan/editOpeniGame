#include "iGameCASReader.h"
#include <filesystem>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include <iostream>

IGAME_NAMESPACE_BEGIN

bool CASReader::Parsing() {
    namespace fs = std::filesystem;

    // 获取 .cas 文件路径
    std::string casPath = this->GetFilePath(); 
    fs::path inputPath(casPath);

    // ===== 临时文件夹路径 =====
    fs::path tempDir = fs::current_path() / "temp";
    if (!fs::exists(tempDir)) { fs::create_directories(tempDir); }

    // 输出目录设为 temp 文件夹
    std::string outputDir = tempDir.string();

    // 输出目录为输入文件所在目录
    //std::string outputDir = inputPath.parent_path().string();

    // cas_converter.exe 的路径
    fs::path exePath = "../../../ThirdParty/Python/pyFluentLib/cas_converter.exe";
    exePath = fs::absolute(exePath);
    //std::string exePath = "./ThirdParty/Python/pyFluentLib/cas_converter.exe";
    //std::string exePath = "cas_converter.exe";


    // 构造命令行参数

    std::string arguments = "--input " + casPath + " --output " + outputDir;
    std::string fullCommand = "\"" + exePath.string() + "\" " + arguments;
    std::cout << "[CASReader] Running command: " << fullCommand << std::endl;

    // 调用转换程序
    int returnCode = system(fullCommand.c_str());
    if (returnCode != 0) {
        std::cerr << "[Error] CAS to VTK conversion failed. Return code: " << returnCode << std::endl;
        return false;
    }


    // 临时文件的输出路径
    fs::path outputFilePath = tempDir / (inputPath.stem().string() + ".vtk");
    std::string outputFile = outputFilePath.string();

    /*std::string outputFile = "";
    fs::path outputFilePath = inputPath.parent_path() / (inputPath.stem().string() + ".vtk");
    outputFile = outputFilePath.string();*/
    

    // 尝试读取生成的 VTK 文件
    auto obj = iGame::FileIO::ReadFile(outputFile);
    if (!obj) {
        std::cerr << "[Error] Failed to load generated VTK file: " << outputFile << std::endl;
        return false;
    }
    std::cout << obj->GetDataObjectType() << '\n';
    auto pc = DynamicCast<PointSet>(obj);
    std::cout << pc->GetNumberOfPoints() << std::endl;
    // 设置输出结果

    this->SetOutput(obj);
    std::cout << "[Info] Successfully converted and loaded: " << outputFile << std::endl;

    // ===== 清理临时文件 =====
    try {
        if (fs::exists(outputFilePath)) {
            fs::remove(outputFilePath);
            std::cout << "[Cleanup] Removed temporary file: " << outputFilePath << std::endl;
        }

        // 如果 temp 文件夹为空，则删除
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




