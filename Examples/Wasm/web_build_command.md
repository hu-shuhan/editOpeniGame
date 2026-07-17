**产物目录**
wasm 构建通过，并产出：
 iGameWasm.js
 iGameWasm.wasm
 index.html

**手动复现命令表（Windows，仓库根目录执行）**

0. 安装emsdk(请确保拉取了emsdk子模块)
cd ThirdParty\emsdk
 .\emsdk.bat install latest
 emsdk activate latest

1. 进入工程根目录
cd iGameVis

2. 检查基础工具
cmd:
where cmake
where ninja

3. 检查可用 preset
cmd:
cmake --list-presets

4. 配置 wasm（核心步骤）
cmd:
cmake --preset wasm-debug
cmake --preset wasm-debug-memory4g
cmake --preset wasm-debug-memory16g      (16G 内存 Debug 版本，MEMORY64=1)
cmake --preset wasm-release-memory4g
cmake --preset wasm-release-memory16g    (16G 内存 Release 版本，MEMORY64=1)

5. 构建 wasm
cmd:
cmake --build --preset wasm-debug
cmake --build --preset wasm-debug-memory4g
cmake --build --preset wasm-debug-memory16g
cmake --build --preset wasm-release-memory4g
cmake --build --preset wasm-release-memory16g

6. 验证产物
cmd:
dir out\build\wasm-release-memory16g\Examples\Wasm

7. 本地启动静态服务（用于浏览器验证）
cmd:
cd out\build\wasm-release-memory16g\Examples\Wasm
python -m http.server 8000

8. 浏览器打开
<http://127.0.0.1:8000/index.html>

9. 上传一个支持的文件（如 .vtk），观察页面日志与渲染结果
