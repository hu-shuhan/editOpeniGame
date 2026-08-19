#include "iGameLsDynaReader.h"
#include "Log/iGameLogger.h"
#include "VTK XML/iGamePVDReader.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iGameFileIO.h>

IGAME_NAMESPACE_BEGIN

bool LsDynaReader::Parsing() {
    namespace fs = std::filesystem;

    // 基类 FileReader::Open() 在 Windows 上会通过 CreateFileMapping/MapViewOfFile
    // 内存映射并锁定 d3plot 文件，导致后续转换器无法正确读取。先解锁。
    this->Close();

    // 输入文件路径（d3plot 族文件的根文件，如 <dir>/d3plot）
    std::string lsDynaPath = this->GetFilePath();
    fs::path inputPath(lsDynaPath);

    // 创建独立临时输出子目录（带唯一后缀，避免不同模型/多次运行之间文件名冲突）
    auto uniqueSuffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    fs::path tempDir = fs::current_path() / "temp" / (inputPath.stem().string() + "_" + uniqueSuffix);
    fs::create_directories(tempDir);

    // 定位转换器可执行文件
    std::vector<std::string> exePaths = {
        "Resources\\pyLsDynaLib\\lsdyna_to_pvd_converter.exe",
        "lsdyna_to_pvd_converter.exe"
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
        IGAME_CORE_ERROR("[LsDynaReader] Error: lsdyna_to_pvd_converter not found in any path");
        IGAME_CORE_ERROR("[LsDynaReader] Please ensure the converter executable exists in ThirdParty/Python/pyLsDynaLib/");
        return false;
    }

    IGAME_CORE_DEBUG("[LsDynaReader] Using converter: {}", exePath);

    // 运行转换器：lsdyna_to_pvd_converter --input <d3plot> --output <out.pvd>
    // 注意：system() 走 cmd.exe /c，命令行里不能出现多对引号（会触发 cmd.exe 的引号剥离规则），
    // 因此这里与 AnsysReader/CCMReader 保持一致：只给 exe 路径加引号，参数不加引号。
    fs::path outputFilePath = tempDir / (inputPath.stem().string() + ".pvd");
    std::string outputFile = outputFilePath.string();

    std::string arguments = "--input " + lsDynaPath + " --output " + outputFile;
    std::string fullCommand = "\"" + exePath + "\" " + arguments;
    IGAME_CORE_DEBUG("[LsDynaReader] Running command: {}", fullCommand);

    int returnCode = system(fullCommand.c_str());
    if (returnCode != 0) {
        IGAME_CORE_ERROR("[LsDynaReader] LS-DYNA d3plot to PVD conversion failed. Return code: {}", returnCode);
        return false;
    }

    // 读取生成的 PVD 文件
    auto pvdReader = iGame::iGamePVDReader::New();
    pvdReader->SetFilePath(outputFile);
    if (!pvdReader->Execute()) {
        IGAME_CORE_ERROR("[LsDynaReader] Failed to read converted PVD file: {}", outputFile);
        return false;
    }

    auto obj = pvdReader->GetOutput();
    if (!obj) {
        IGAME_CORE_ERROR("[LsDynaReader] Converter produced empty output");
        return false;
    }

    IGAME_CORE_DEBUG("[LsDynaReader] Successfully loaded: {}", outputFile);

    this->SetOutput(obj);

    // 保留 PVD/VTU 临时文件，供动画切帧时懒加载；程序下次启动时会统一清理 temp 目录。
    return true;
}

IGAME_NAMESPACE_END
