#include "iGameLsDynaReader.h"
#include "iGameExternalProcess.h"
#include "Log/iGameLogger.h"
#include "VTK XML/iGamePVDReader.h"
#include <chrono>
#include <filesystem>

IGAME_NAMESPACE_BEGIN

bool LsDynaReader::Parsing() {
    namespace fs = std::filesystem;

    // 基类 FileReader::Open() 在 Windows 上会通过 CreateFileMapping/MapViewOfFile
    // 内存映射并锁定 d3plot 文件，导致后续转换器无法正确读取。先解锁。
    this->Close();

    // 输入文件路径（d3plot 族文件的根文件，如 <dir>/d3plot）
    std::string lsDynaPath = this->GetFilePath();
    // m_FilePath 为 UTF-8，先转成本地宽字符 path，避免窄字符串按 ACP 解码产生乱码
    fs::path inputPath = FileSystem::PathFromUtf8(lsDynaPath);

    // 创建独立临时输出子目录（带唯一后缀，避免不同模型/多次运行之间文件名冲突）
    auto uniqueSuffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    fs::path tempDirName = inputPath.stem();
    tempDirName += "_" + uniqueSuffix;
    fs::path tempDir = fs::current_path() / "temp" / tempDirName;
    fs::create_directories(tempDir);

    // 定位转换器可执行文件
    std::vector<std::string> exePaths = {
        "Resources\\pyLsDynaLib\\lsdyna_to_pvd_converter.exe",
        "lsdyna_to_pvd_converter.exe"
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
        IGAME_CORE_ERROR("[LsDynaReader] Error: lsdyna_to_pvd_converter not found in any path");
        IGAME_CORE_ERROR("[LsDynaReader] Please ensure the converter executable exists in ThirdParty/Python/pyLsDynaLib/");
        return false;
    }

    IGAME_CORE_DEBUG("[LsDynaReader] Using converter: {}", exePath);

    // 运行转换器：lsdyna_to_pvd_converter --input <d3plot> --output <out.pvd>
    // 直接以宽字符命令行启动转换器（不经 cmd.exe，绕开其引号剥离规则），
    // 参数加引号，中文/空格路径均安全。
    fs::path outputFileName = inputPath.stem();
    outputFileName += ".pvd";
    fs::path outputFilePath = tempDir / outputFileName;
    // 公共 API 边界统一为 UTF-8
    std::string outputFile = FileSystem::PathToUtf8(outputFilePath);

    std::vector<std::string> arguments = {"--input", lsDynaPath, "--output", outputFile};
    IGAME_CORE_DEBUG("[LsDynaReader] Running converter: {} --input {} --output {}", exePath, lsDynaPath, outputFile);

    int returnCode = 0;
    if (!ExternalProcess::Run(exePath, arguments, returnCode)) {
        IGAME_CORE_ERROR("[LsDynaReader] Failed to start converter: {}", exePath);
        return false;
    }
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
