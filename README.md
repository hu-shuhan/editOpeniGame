# iGameVis developer version

[中文版说明](README.zh.md)

## Project Architecture

iGameVis is a CAE simulation result visualization platform built on the `iGameCore` core library and an optional `Qt` GUI module.

```mermaid
flowchart TB
    subgraph entry [EntryPoints]
        mainCpp[main.cpp]
        examples[Examples]
    end
    subgraph qt [Qt_Module]
        mainWindow[igQtMainWindow]
        fileLoader[igQtFileLoader]
        dockWidgets[DockWidgets]
    end
    subgraph core [iGameCore]
        io[IO_FileIO]
        dataModel[DataModel]
        filters[Filters]
        rendering[Rendering]
    end
    mainCpp --> mainWindow
    examples --> io
    mainWindow --> fileLoader --> io
    mainWindow --> dockWidgets --> filters
    io --> dataModel
    filters --> dataModel
    dataModel --> rendering
```

### Core Directories

| Directory | Responsibility |
|-----------|----------------|
| [`iGameCore/Core/`](iGameCore/Core/) | CellModel, Common, DataModel — base objects and mesh data structures |
| [`iGameCore/Filters/`](iGameCore/Filters/) | Feature extraction, flow/vector/tensor, deformation, visual analytics |
| [`iGameCore/IO/`](iGameCore/IO/) | VTK/CGNS/Spline format readers/writers; unified entry `FileIO::ReadFile()` |
| [`iGameCore/Rendering/`](iGameCore/Rendering/) | Scene, OpenGL rendering, Meshlet acceleration |
| [`Qt/`](Qt/) | GUI main window, file loader, visualization Dock panels |
| [`Examples/`](Examples/) | Standalone example programs (`EXAMPLE_COMPILE=ON`) |

### Invocation Paths

1. **GUI**: `main.cpp` → `igQtMainWindow` → `igQtFileLoader::OpenFile()` → `FileIO::ReadFile()` → `Scene::AddModel()` → Dock widgets call Filters / DrawObject
2. **Examples**: `Examples/<module>/main` → `FileIO::ReadFile()` → `Filter::New()->Execute()` → `Scene` → `RenderWindow::Show()`

### Open-Source Metric Modules

| Metric | Module | Documentation (ZH) | Documentation (EN) | Anchor Directory |
|--------|--------|--------------------|----------------------|------------------|
| 7.1 | High-order visualization | [README.md](iGameCore/Filters/Convert/README.md) | [README.en.md](iGameCore/Filters/Convert/README.en.md) | `iGameCore/Filters/Convert/` |
| 10.1 | Intelligent visual analysis | [README.md](iGameCore/Filters/ParallelCoordinates/README.md) | [README.en.md](iGameCore/Filters/ParallelCoordinates/README.en.md) | `iGameCore/Filters/ParallelCoordinates/` |
| 10.2 | Feature extraction | [README.md](iGameCore/Filters/FeatureExtraction/README.md) | [README.en.md](iGameCore/Filters/FeatureExtraction/README.en.md) | `iGameCore/Filters/FeatureExtraction/` |
| 10.3 | Physics field visual interaction | [README.md](iGameCore/Filters/StreamView/README.md) | [README.en.md](iGameCore/Filters/StreamView/README.en.md) | `iGameCore/Filters/StreamView/` |
| 11.2 | VTK/CGNS data interface | [README.md](iGameCore/IO/README.md) | [README.en.md](iGameCore/IO/README.en.md) | `iGameCore/IO/` |
| 11.3 | Visualization outputs | [README.md](iGameCore/Core/DataModel/README.md) | [README.en.md](iGameCore/Core/DataModel/README.en.md) | `iGameCore/Core/DataModel/` |
| 11.4 | CAE parallel visualization software | [README.md](iGameCore/README.md) | [README.en.md](iGameCore/README.en.md) | `iGameCore/` |

## File Import

File import cannot have Chinese path.

## Requirements

- QT 5.14.2

## Install

~~~shell
# Needn't to use ThirdParty's SubModule
git clone https://github.com/mky8812/editOpeniGame.git
# If you need to use ThirdParty's SubModule. SubModule's detail see target file(.gitmodules)
git clone --recurse-submodules https://github.com/mky8812/editOpeniGame.git
~~~

## Build

To Build this Project, you need to check if you have Configured Qt5 Cmake Path in your `environment variables`.

If `NOT`, you have to edit `Qt ` module's CMakeLists.txt. Replace the Qt5 cmake path with the Qt path on your computer.

~~~cmake
#UNIX
set(Qt5_DIR "/usr/lib/Qt/5.14.2/gcc_64/lib/cmake/Qt5")
#Windows
set(Qt5_DIR "D:/Qt/5.14.2/msvc2017_64/lib/cmake/Qt5")
~~~

After Edit your qt path, Clear the CMAKE cache.

~~~shell
#1. Enter the cmake build path
cd out/build/x64-debug
cd out/build/x64-release
#2 .use cmake clean command to clear
cmake --build . --target clean
~~~

Then you can open the Project correctly. Enjoy it !

ps. Use Cmake to Build in Command

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 12
cmake --build build --target install
```

use qt-cmake to build project(Only available in qt6 version)

```shell
cd build
~/projects/packages/qt/host/qt-everywhere-src-6.5.3/qtbase/bin/qt-cmake ..
cmake --build . --parallel 8
./iGameVis # run this app
```

Compile iGameVis on Web by using Wasm(Temporarily not applicable)

```shell
cd build/wasm
source ~/projects/packages/emsdk/emsdk_env.sh
emcc -v # check version, must using 3.1.25
~/projects/packages/qt/wasm/qt-everywhere-src-6.5.3/qtbase/bin/qt-cmake ../../
cmake --build . --parallel 8
python3 -m http.server # to start http-server by using python3 http module. then visit http://localhost:8000/Qt_OpenGL.html
```

## How to Use iGameVis

Detail process see `iGameVisNoticeToUsers.md`
