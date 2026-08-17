#include "iGameLsDynaReader.h"
#include "Log/iGameLogger.h"
#include "VTK XML/iGamePVDReader.h"
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

    // 创建临时输出目录
    fs::path tempDir = fs::current_path() / "temp";
    if (!fs::exists(tempDir)) { fs::create_directories(tempDir); }

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

    // 清理临时文件（PVD + 各时间步 VTU）
    try {
        if (fs::exists(outputFilePath)) {
            fs::remove(outputFilePath);
            IGAME_CORE_DEBUG("[LsDynaReader] Removed temporary file: {}", outputFilePath.string());
        }
        // 删除与本模型相关的 VTU 文件（命名约定为 <stem>_<idx>.vtu 或 <stem><idx>.vtu）
        const std::string stem = inputPath.stem().string();
        for (const auto& entry : fs::directory_iterator(tempDir)) {
            if (!entry.is_regular_file()) continue;
            const std::string name = entry.path().filename().string();
            if (name.rfind(stem, 0) == 0 && entry.path().extension() == ".vtu") {
                fs::remove(entry.path());
                IGAME_CORE_DEBUG("[LsDynaReader] Removed temporary file: {}", entry.path().string());
            }
        }
        if (fs::exists(tempDir) && fs::is_empty(tempDir)) {
            fs::remove(tempDir);
            IGAME_CORE_DEBUG("[LsDynaReader] Removed empty temp directory: {}", tempDir.string());
        }
    } catch (const std::exception& e) {
        IGAME_CORE_WARN("[LsDynaReader] Failed to clean up temporary files: {}", e.what());
    }

    return true;
}

IGAME_NAMESPACE_END
