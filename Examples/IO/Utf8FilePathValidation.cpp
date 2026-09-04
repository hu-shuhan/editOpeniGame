#include <iGameFileIO.h>
#include <iGameFileSystem.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>

int main() {
    namespace fs = std::filesystem;

    const fs::path testDirectory =
            fs::temp_directory_path() / iGame::FileSystem::PathFromUtf8("iGameVis_中文路径回归");
    const fs::path testFile = testDirectory / iGame::FileSystem::PathFromUtf8("中文模型.vtk");

    std::error_code error;
    fs::create_directories(testDirectory, error);
    if (error) {
        std::cerr << "Could not create UTF-8 test directory: " << error.message() << '\n';
        return 1;
    }

    {
        std::ofstream output(testFile, std::ios::binary | std::ios::trunc);
        output << "# vtk DataFile Version 3.0\n"
                  "UTF-8 path validation\n"
                  "ASCII\n"
                  "DATASET POLYDATA\n"
                  "POINTS 3 float\n"
                  "0 0 0\n"
                  "1 0 0\n"
                  "0 1 0\n"
                  "POLYGONS 1 4\n"
                  "3 0 1 2\n";
        if (!output) {
            std::cerr << "Could not create UTF-8 path test file\n";
            return 1;
        }
    }

    const std::string utf8Path = iGame::FileSystem::PathToUtf8(testFile);
    const auto object = iGame::FileIO::ReadFile(utf8Path);

    fs::remove_all(testDirectory, error);
    if (!object) {
        std::cerr << "FileIO failed to read a VTK file from a UTF-8 path\n";
        return 1;
    }

    std::cout << "UTF-8 file path validation passed\n";
    return 0;
}
