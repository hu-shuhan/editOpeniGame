# SDK 编译指南

## 1. 拉取仓库

```bash
git clone -b stable-sdk --recursive https://gitcode.com/yanhekaiyuan/iGameVis.git
```

> 该命令会拉取带所有子模块的仓库代码（包含 CGNS，CGNS 作为 SDK 编译时的功能可选项）。

## 2. 依赖安装(各依赖的获取详见附录)

### 2.1 libtorch路径
在 `ThirdParty` 文件夹下解压 libtorch至iGameVis/ThirdParty/libtorch/Windows(or Linux)/Release(or Debug)/GPU，最终可能的项目路径构成如下：

```
-iGameVis
    -iGameCore
    -ThirdParty
        -libtorch
            -Windows
                -Release
                    -CPU
                    -GPU
                -Debug
                    -CPU
                    -GPU
            -Linux
                -Release
                    -CPU
                    -GPU
                        -bin
                        -include
                        -lib
                        -share
                        -build-hash
                        -build-version
                -Debug
                    -CPU
                    -GPU
```

### 2.2 Wayland依赖构建(仅Linux端)

`wayland-scanner` 由 Ubuntu 的 `libwayland-bin` 软件包提供。

安装：

```
sudo apt update
sudo apt install \
  pkg-config \
  libwayland-bin \
  libwayland-dev \
  libxkbcommon-dev \
  libx11-dev \
  libxrandr-dev \
  libxinerama-dev \
  libxcursor-dev \
  libxi-dev \
  libxext-dev
```

检查版本：

```
wayland-scanner --version
dpkg-query -W -f='${Package} ${Version}\n' libwayland-bin
```

## 3. 修改CGNS模块CMakeLists.txt（仅在拉取CGNS模块时需要）

CGNS模块的期望路径为iGameVis\ThirdParty\cgns，路径构成如下：
```
-iGameVis
	-iGameCore
    -ThirdParty
    	-cgns
        	-CMakeLists.txt
```
修改CMakeLists.txt中的
```
ADD_CUSTOM_TARGET(uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
```
为
```
ADD_CUSTOM_TARGET(cgns_uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
```
解决uninstall的重名冲突

## 4. 运行编译脚本

在项目根目录执行：

```bash
cd Script
python build_and_package.py --build-type Release
```

通过 Python 脚本执行自动编译，相关参数可直接在 SDK 编译脚本 `.py` 文件中修改，也可通过命令行参数传递。

可选编译命令：
| GCC版本 | 参数 |
|------|------|
| GCC11 | `--gcc-versions 11` |
| GCC13 | `--gcc-versions 13` |
| GCC15 | `--gcc-versions 15` |

多版本编译参数直接记为--gcc-versions [gcc_version1] [gcc_version2]
如--gcc-versions 11 13即可同时编译两个版本，参数为空时默认全编译，即GCC11、13、15三个版本

可选子模块命令：

| 模块 | 参数 |
|------|------|
| CGNS | `--enable-cgns` |
| libtorch | `--enable-libtorch` |

## 5. SDK 产物

SDK 产物会被脚本按编译使用的 gcc 版本放置于根目录的不同文件夹下，期望的结果如下：

```
-iGameVis
    -iGameCore
    -cmake-autobuild-release-gcc11（对应Ubuntu22.04）
        -iGameCore
        -CMakeFiles
        -Resource
        -ThirdParty
        -iGameCore.tar.gz（由脚本整理压缩可直接交付的SDK）
    -cmake-autobuild-release-gcc13（对应Ubuntu24.04）
    -cmake-autobuild-release-gcc15（对应Ubuntu26.04）
```

---

# 附录：项目依赖获取指南

## 1. libtorch 获取构建指南

- **硬件需求**：Nvidia 显卡
- **相关依赖**：NVIDIA CUDA Toolkit 12.1（已测试稳定），请至少确保 `nvcc -V` 和 `nvidia-smi` 能正常输出
- **路径设置**：请在官方网站(https://pytorch.org/get-started)下载对应版本的 libtorch 预编译包后，将 libtorch 放置到 `ThirdParty/libtorch/Windows(or Linux)/Release(or Debug)/GPU` 路径下

### libtorch 官方下载资源解压后的期望路径

```
-libtorch
    -bin
    -include
    -lib
    -share
    -build-hash
    -build-version
```

### 作为子模块加入到项目时的期望路径

```
-iGameVis
    -iGameCore
    -ThirdParty
        -libtorch
            -Windows
                -Release
                    -CPU
                    -GPU
                -Debug
                    -CPU
                    -GPU
            -Linux
                -Release
                    -CPU
                    -GPU
                        -bin
                        -include
                        -lib
                        -share
                        -build-hash
                        -build-version
                -Debug
                    -CPU
                    -GPU
```

> 以上仅作为一个 Linux 平台下的 Release 编译时的 GPU 版本 libtorch 路径示例，请根据运行平台以及使用需要放置到对应的路径下。

编译时在主 `CMakeLists` 下打开 libtorch module 的编译开关即可使用 libtorch 相关功能，如 `VortexDetection`。

> **注意**：可以但不推荐放在 Debug 路径下，编译与运行速度过慢。
