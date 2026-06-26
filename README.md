# iGameVis release version

## File Import

File import cannot have Chinese path.

## Requirements

- QT 5.14.2

## Install

~~~shell
# Needn't to use ThirdParty's SubModule
git clone https://gitcode.com/yanhekaiyuan/iGameVis.git
# If you need to use ThirdParty's SubModule. SubModule's detail see target file(.gitmodules)
git clone --recurse-submodules https://gitcode.com/yanhekaiyuan/iGameVis.git
~~~

## Build

To Build this Project, you need to check if you have Configured Qt5 Cmake Path in your `environment variables`.

If `NOT`, you have to edit `Qt `module's CMakeLists.txt. Replace the Qt5 cmake path with the Qt path on your computer.

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
cmake -build build --target install
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
