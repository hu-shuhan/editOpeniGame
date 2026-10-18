#include "iGameAnsysReader.h"
#include "iGameExternalProcess.h"
#include "Log/iGameLogger.h"
#include "VTK XML/iGamePVDReader.h"
#include <chrono>
#include <filesystem>

IGAME_NAMESPACE_BEGIN

bool AnsysReader::Parsing() {
    // FileReader::OpenWithWindowsSystem might lock the file, unlock it first
    this->Close();

    namespace fs = std::filesystem;

    // Get the input file path (.rst or .rth)
    std::string ansysPath = this->GetFilePath();
    // m_FilePath 为 UTF-8，先转成本地宽字符 path，避免窄字符串按 ACP 解码产生乱码
    fs::path inputPath = FileSystem::PathFromUtf8(ansysPath);

    // 创建独立临时输出子目录（带唯一后缀，避免不同模型/多次运行之间文件名冲突）
    auto uniqueSuffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    fs::path tempDirName = inputPath.stem();
    tempDirName += "_" + uniqueSuffix;
    fs::path tempDir = fs::current_path() / "temp" / tempDirName;
    fs::create_directories(tempDir);

    // Locate the converter executable
    std::vector<std::string> exePaths = {
        "Resources\\pyAnsysLib\\ansys_to_pvd_converter.exe", 
        "ansys_to_pvd_converter.exe"
    };

    std::string exePath;
    bool exeFound = false;
    for (const auto& path: exePaths) {
        if (fs::exists(FileSystem::PathFromUtf8(path))) {
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

    // Run the converter: ansys_to_pvd_converter --input input.rst --output output.pvd
    // 直接以宽字符命令行启动转换器（不经 cmd.exe），参数加引号，中文/空格路径均安全。
    fs::path outputFileName = inputPath.stem();
    outputFileName += ".pvd";
    fs::path outputFilePath = tempDir / outputFileName;
    // 公共 API 边界统一为 UTF-8
    std::string outputFile = FileSystem::PathToUtf8(outputFilePath);

    std::vector<std::string> arguments = {"--input", ansysPath, "--output", outputFile};
    IGAME_CORE_DEBUG("[AnsysReader] Running converter: {} --input {} --output {}", exePath, ansysPath, outputFile);

    int returnCode = 0;
    // 转换器是外部 exe，无法给出内部进度；用时间驱动的进度条 + 状态栏文案代替，
    // 否则这段「启动并等待」在 GUI 线程里毫无反馈，用户会以为软件卡死。
    // 该对象活到 Parsing 结束：后面读回 PVD 仍是同一次「打开文件」操作，
    // 一并保持进度条可见。
    ExternalProcess::ConverterProgress convertProgress("正在转换 Ansys 结果文件");
    // 第 4 个参数把本次运行的临时子目录交给转换器当 TEMP/TMP：转换器是 onefile
    // 自解包程序，若系统 %TEMP% 不可写会在解包阶段失败
    // （"failed to open ... for writing"），与本仓库逻辑无关。
    if (!ExternalProcess::Run(exePath, arguments, returnCode,
                              FileSystem::PathToUtf8(tempDir),
                              convertProgress.Ticker())) {
        IGAME_CORE_ERROR("[AnsysReader] Failed to start converter: {}", exePath);
        return false;
    }
    if (returnCode != 0) {
        IGAME_CORE_ERROR("[AnsysReader] Ansys to PVD conversion failed. Return code: {}", returnCode);
        IGAME_CORE_ERROR("[AnsysReader] hint: the converter unpacks itself into its TEMP directory "
                         "on startup; if the message above says 'failed to open ... for writing', "
                         "make sure '{}' is writable",
                         FileSystem::PathToUtf8(tempDir));
        return false;
    }

    // Load the generated PVD file
    auto pvdReader = iGame::iGamePVDReader::New();
    // 转换阶段已经推进到 0.85，把 PVD 读回映射到剩余的 0.85→1.0，
    // 让整条进度条连续往前走，而不是读完再跳一次。
    pvdReader->SetProgressRange(0.85, 0.15);
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

    // 保留 PVD/VTU 临时文件，供动画切帧时懒加载；程序下次启动时会统一清理 temp 目录。
    return true;
}

IGAME_NAMESPACE_END