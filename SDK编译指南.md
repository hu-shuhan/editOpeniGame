1.通过git clone -b stable-sdk --recursive <https://gitcode.com/yanhekaiyuan/iGameVis.git> 命令拉取带所有子模块的仓库代码（即包含CGNS,CGNS作为SDK编译时的功能可选项）
2.在ThirdParty文件夹下解压libtorch
最终的项目路径相关构成如下：
-iGameVis
    -iGameCore
    -CmakeGPS
    -Script
    -ThirdParty
        -libtorch
        -cgns
3.在项目根目录运行命令
cd Script
python build_and_package.py --build-type Release
通过Python脚本执行自动编译，相关参数可直接在SDK编译脚本py文件中修改，也可通过命令行参数传递
可选子模块命令（--enable-cgns --enable-libtorch）
CGNS模块--enable-cgns  libtorch模块--enable-libtorch
4.SDK产物会被脚本按编译使用的gcc版本放置于根目录的不同文件夹下，期望的结果如下
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

附录:项目依赖获取指南
1.libtorch获取构建指南
硬件需求：Nvidia显卡
相关依赖：NVIDIA CUDA Toolkit 12.1（已测试稳定），请至少确保nvcc -V和nvidia-smi能正常输出
路径设置：请在官方网站下载对应版本的libtorch预编译包后，将libtorch放置到ThirdParty/libtorch/Windows(or Linux)/Release(or Debug)/GPU路径下

libtorch官方下载资源解压后的期望路径如下:
-libtorch
    -bin
    -include
    -lib
    -share
    -build-hash
    -build-version
作为子模块加入到项目时的期望路径如下：
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
以上仅作为一个Linux平台下的Release编译时的GPU版本libtorch路径示例，请根据运行平台以及使用需要放置到对应的路径下
编译时在主CMakeLists下打开libtorch module的编译开关即可使用libtorch相关功能，如VortexDetection
注：可以但不推荐放在Debug路径下，编译与运行速度过慢
