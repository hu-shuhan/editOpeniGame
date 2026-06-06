**过程总结**
最终结果：
wasm 构建通过，并产出：
 iGameWasmDemo.js 
 iGameWasmDemo.wasm 
 index.html 

**手动复现命令表（Windows，仓库根目录执行）**
1. 进入工程目录
cmd:
cd editOpeniGame

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
cmake --preset wasm-debug-16g-experimental (16G内存版本)
cmake --preset wasm-release-4g
cmake --preset wasm-release-16g (16G内存 Release 版本)

5. 构建 wasm
cmd:
cmake --build --preset wasm-debug
cmake --build --preset wasm-debug-16g-experimental  (16G内存版本)
cmake --build --preset wasm-release-4g
cmake --build --preset wasm-release-16g

6. 验证产物
cmd:
dir out\build\wasm-debug\Examples\Wasm

7. 本地启动静态服务（用于浏览器验证）
cmd:
cd editOpeniGame\out\build\wasm-release-16g\Examples\Wasm
python -m http.server 8000

8. 浏览器打开
http://127.0.0.1:8000/index.html

9. 上传一个 支持的文件如（.vtk） 文件，观察页面日志与渲染结果

