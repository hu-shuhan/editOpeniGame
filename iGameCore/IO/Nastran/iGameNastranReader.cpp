#include "iGameNastranReader.h"
#include "iGameFileSystem.h"

#include <VTK XML/iGameVTUReader.h>

#include <filesystem>
#include <string_view>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

#ifdef _WIN32
std::wstring ToWidePath(const std::string& value) {
    if (value.empty()) return {};
    auto convert = [&value](UINT codePage, DWORD flags) -> std::wstring {
        const int length = MultiByteToWideChar(
                codePage, flags, value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (length <= 0) return {};
        std::wstring result(static_cast<size_t>(length), L'\0');
        if (MultiByteToWideChar(codePage, flags, value.data(),
                               static_cast<int>(value.size()), result.data(), length) <= 0) {
            return {};
        }
        return result;
    };
    std::wstring result = convert(CP_UTF8, MB_ERR_INVALID_CHARS);
    return result.empty() ? convert(CP_ACP, 0) : result;
}

std::wstring QuoteWindowsArgument(std::wstring_view argument) {
    if (argument.empty()) return L"\"\"";
    if (argument.find_first_of(L" \t\n\v\"") == std::wstring_view::npos) {
        return std::wstring(argument);
    }
    std::wstring quoted = L"\"";
    size_t backslashes = 0;
    for (const wchar_t character : argument) {
        if (character == L'\\') {
            ++backslashes;
            continue;
        }
        if (character == L'\"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(L'\"');
        } else {
            quoted.append(backslashes, L'\\');
            quoted.push_back(character);
        }
        backslashes = 0;
    }
    quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'\"');
    return quoted;
}

std::filesystem::path ApplicationDirectory() {
    std::vector<wchar_t> buffer(32768);
    const DWORD length = GetModuleFileNameW(
            nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) return {};
    return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
}

bool RunConverter(const std::filesystem::path& executable,
                  const std::vector<std::wstring>& arguments,
                  DWORD& exitCode) {
    std::wstring commandLine = QuoteWindowsArgument(executable.native());
    for (const auto& argument : arguments) {
        commandLine.push_back(L' ');
        commandLine += QuoteWindowsArgument(argument);
    }
    std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
    mutableCommand.push_back(L'\0');

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    const BOOL created = CreateProcessW(
            executable.c_str(), mutableCommand.data(), nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, executable.parent_path().c_str(),
            &startupInfo, &processInfo);
    if (!created) return false;

    const DWORD waitResult = WaitForSingleObject(processInfo.hProcess, INFINITE);
    const BOOL gotExitCode = waitResult == WAIT_OBJECT_0 &&
                             GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return gotExitCode == TRUE;
}
#endif

} // namespace

IGAME_NAMESPACE_BEGIN

NastranReader::NastranReader() = default;
NastranReader::~NastranReader() = default;

void NastranReader::SetBDFFileName(const std::string& filename) {
    m_BDFFilePath = filename;
    SetFilePath(filename);
}

void NastranReader::SetOP2FileName(const std::string& filename) {
    m_OP2FilePath = filename;
}

bool NastranReader::Parsing() {
    m_Output = nullptr;
    if (m_BDFFilePath.empty()) {
        IGAME_ERROR("[NastranReader] Error: BDF file path is not set");
        return false;
    }

#ifdef _WIN32
    // C/S entry points provide UTF-8 paths; retain ANSI fallback for existing local callers.
    std::filesystem::path bdfPath = m_RemoteConversionEnabled
            ? FileSystem::PathFromUtf8(m_BDFFilePath)
            : std::filesystem::path(ToWidePath(m_BDFFilePath));
    std::filesystem::path op2Path = m_RemoteConversionEnabled
            ? FileSystem::PathFromUtf8(m_OP2FilePath)
            : std::filesystem::path(ToWidePath(m_OP2FilePath));
    if (!bdfPath.empty()) bdfPath = std::filesystem::absolute(bdfPath);
    if (!op2Path.empty()) op2Path = std::filesystem::absolute(op2Path);
    if (bdfPath.empty() || !std::filesystem::is_regular_file(bdfPath)) {
        IGAME_ERROR("[NastranReader] Error: BDF file does not exist");
        return false;
    }
    if (!m_OP2FilePath.empty() &&
        (op2Path.empty() || !std::filesystem::is_regular_file(op2Path))) {
        IGAME_ERROR("[NastranReader] Error: OP2 file does not exist");
        return false;
    }

    const std::filesystem::path appDirectory = ApplicationDirectory();
    const std::filesystem::path currentDirectory = std::filesystem::current_path();
    const std::vector<std::filesystem::path> candidates = {
            appDirectory / L"Resources" / L"pyNastranLib" / L"nastran_to_vtk_cli.exe",
            currentDirectory / L"Resources" / L"pyNastranLib" / L"nastran_to_vtk_cli.exe",
            appDirectory / L"nastran_to_vtk_cli.exe",
            currentDirectory / L"nastran_to_vtk_cli.exe",
    };

    std::filesystem::path executable;
    for (const auto& candidate : candidates) {
        if (!candidate.empty() && std::filesystem::is_regular_file(candidate)) {
            executable = std::filesystem::absolute(candidate);
            break;
        }
    }
    if (executable.empty()) {
        IGAME_ERROR("[NastranReader] Error: nastran_to_vtk_cli.exe not found");
        return false;
    }

    std::filesystem::path outputPath = bdfPath;
    outputPath += L".vtu";
    std::vector<std::wstring> arguments = {
            L"--force", L"--bdf", bdfPath.native(), L"--output", outputPath.native()};
    if (!m_OP2FilePath.empty()) {
        arguments.emplace_back(L"--op2");
        arguments.emplace_back(op2Path.native());
    } else {
        IGAME_WARN("[NastranReader] OP2 file is not set; converting geometry only");
    }

    DWORD exitCode = 0;
    if (!RunConverter(executable, arguments, exitCode)) {
        IGAME_ERROR("[NastranReader] Error: failed to start or wait for converter process");
        return false;
    }
    if (exitCode != 0) {
        IGAME_ERROR("[NastranReader] Error: converter process failed");
        return false;
    }
    if (!std::filesystem::is_regular_file(outputPath) ||
        std::filesystem::file_size(outputPath) == 0) {
        IGAME_ERROR("[NastranReader] Error: converter did not produce a valid VTU file");
        return false;
    }

    auto vtuReader = iGame::iGameVTUReader::New();
    vtuReader->SetFilePath(FileSystem::PathToUtf8(outputPath));
    if (!vtuReader->Execute()) {
        IGAME_ERROR("[NastranReader] Error: failed to read converted VTU file");
        return false;
    }
    m_Output = vtuReader->GetOutput();
    if (m_Output == nullptr) {
        IGAME_ERROR("[NastranReader] Error: VTU reader returned no output");
        return false;
    }
    IGAME_CORE_DEBUG("[NastranReader] Nastran to VTU conversion succeeded");
    return true;
#else
    IGAME_ERROR("[NastranReader] Error: packaged converter is supported on Windows only");
    return false;
#endif
}

bool NastranReader::CreateDataObject() {
    return m_Output != nullptr;
}

DataObject::Pointer NastranReader::GetOutput() {
    return m_Output;
}

bool NastranReader::Execute() {
    m_Output = nullptr;
    if (!Parsing()) return false;
    return CreateDataObject();
}

void NastranReader::SetFilePath(const std::string& filePath) {
    m_FilePath = filePath;
    const size_t separator = filePath.find_last_of("/\\");
    m_FileName = separator == std::string::npos ? filePath : filePath.substr(separator + 1);
    m_BDFFilePath = filePath;
}

IGAME_NAMESPACE_END
