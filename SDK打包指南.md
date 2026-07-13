1.通过git clone -b stable-sdk --recursive <https://gitcode.com/yanhekaiyuan/iGameVis.git> 命令拉取带所有子模块的仓库代码（即包含CGNS,CGNS作为SDK打包时的功能可选项）
2.将CMakeGPS解压到项目根目录下，即与iGameCore同级，在ThirdParty文件夹下解压libtorch
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
通过Python脚本执行自动打包
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
        -iGameCore.tar.gz（由脚本整理打包可直接交付的SDK）
    -cmake-autobuild-release-gcc13（对应Ubuntu24.04）
    -cmake-autobuild-release-gcc15（对应Ubuntu26.04）
