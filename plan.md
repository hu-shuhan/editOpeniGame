## Plan: iGameVis WebAssembly GLES2 落地

目标是参照 VTK 的“平台后端分层 + 编译期开关”思路，把现有项目拆出 WebAssembly 最小可用链路：浏览器上传 vtk 文件 -> wasm 导出 IO 接口读取 -> 交给 Scene 渲染。首期不做 meshlet、OIT、体渲染等高级特性，优先保障基础 GL 在 WebGL1/GLES2 下可运行。

**Steps**
1. Phase 0: 约束冻结与目标收敛
1. 明确首期目标矩阵：平台仅 Emscripten + 浏览器直调 wasm API；渲染能力仅基础网格显示（surface/wireframe）、相机重置、基本交互；明确排除 meshlet/OIT/体渲染/复杂后处理。此步产出功能边界文档。
1. 冻结输入输出契约：JS 侧只负责文件选择与字节传输；wasm 侧提供 `LoadVtkFromBytes`、`RenderFrame`、`ResetCamera` 等最小 API。*后续步骤依赖本步*

2. Phase 1: 构建系统分层（参照 VTK 后端策略）
1. 在顶层 CMake 增加平台选项：`IGAME_PLATFORM_WEB`、`IGAME_RENDER_BACKEND`（DesktopGL/GLES2）。通过编译定义向核心库传播，避免直接在业务代码散落 `#ifdef EMSCRIPTEN`。建议在 `CMakeLists.txt` 和 `iGameCore/CMakeLists.txt` 统一下发。
1. 在渲染模块建立后端宏：将当前固定的 `IGAME_OPENGL_VERSION_460` 改为可切换定义（DesktopGL 使用现有宏，Web 使用 GLES2 宏）。重点改 `iGameCore/Rendering/CMakeLists.txt`。*depends on step 1*
1. 新增 wasm 专用 target（独立于 Qt），例如 `Examples/Wasm`，避免 Qt 和桌面入口污染最小 demo。配置 Emscripten 链接参数（导出函数、文件系统、WebGL1 兼容参数、内存初值）。*parallel with Phase 2 部分准备工作*

3. Phase 2: 渲染后端抽象与 GLES2 降级
1. 把窗口/循环与 Scene 解耦：当前 `RenderWindow::Show()` 是 GLFW while 循环，需要抽出“单帧渲染入口”供浏览器 `requestAnimationFrame` 驱动。改造点在 `iGameCore/Rendering/Core/RenderWindow/iGameRenderWindow.cpp` 与 Scene 对外接口。*depends on Phase 1*
1. 按 VTK 的“同一上层 API + 多后端实现”思想，保留现有 Scene/Model API，不同后端仅替换底层 GL 能力实现：
1. DesktopGL 路径维持现状。
1. GLES2 路径禁用或替换高级能力（OIT、atomic counter、SSBO、mesh shader）。关键入口在 `iGameCore/Rendering/Core/iGameScene.cpp` 的 `Initialize()/InitOIT()` 及 OpenGL 封装层。
1. 着色器分支化：为 GLES2 新建 shader 版本目录或宏分支，保证 attribute/varying/precision 与 WebGL1 兼容。*depends on Phase 2 step 2*

4. Phase 3: Web 端 IO 与 wasm 服务接口
1. 为 IO 增加“内存读取”路径：当前 `FileIO::ReadFile(path)` 以路径为主，新增 `ReadVTKFromMemory(buffer,size)` 或等效接口，内部复用 VTKReader 解析逻辑，避免浏览器虚拟文件系统耦合。改造 `iGameCore/IO/iGameFileIO.cpp`、`iGameCore/IO/VTK/iGameVTKReader.cpp` 相关输入流接口。*depends on Phase 1*
1. 新建 wasm C API/embind 包装层（建议 `Examples/Wasm` 下单独包装文件），至少导出：
1. 初始化渲染上下文
1. 上传 vtk 字节并创建 DataObject
1. 调用 `Scene::AddModel`
1. 单帧渲染触发
1. 相机重置
1. JS Demo 页面完成文件上传 -> wasm API -> Scene 渲染的闭环。*depends on Phase 2 + Phase 3 step 1*

5. Phase 4: 最小 Demo 验证与回归防护
1. 构建最小网页 demo（input file + canvas + 日志区）。选取小体量 vtk 样例，验证“上传成功、解析成功、首帧可见、可旋转缩放”。
1. 增加最小自动化检查：
1. 原生构建不回归（DesktopGL target 继续可编译）
1. wasm target 可编译并产出 `.wasm/.js/.html`
1. IO 单测或烟雾测试（至少覆盖 vtk 读取成功路径）
1. 记录后续增强清单：WebGL2/GLES3 升级、渐进恢复透明/OIT、meshlet 专项 Web 路径。

**Relevant files**
- `CMakeLists.txt` — 新增平台/渲染后端选项并控制子模块编译范围
- `iGameCore/CMakeLists.txt` — 向核心库传播 Web/GLES2 编译定义，裁剪不可用模块
- `iGameCore/Rendering/CMakeLists.txt` — 从固定 OpenGL 460 改为后端可选定义
- `iGameCore/Rendering/Core/RenderWindow/iGameRenderWindow.cpp` — 拆出单帧渲染接口，弱化对 GLFW while-loop 的硬依赖
- `iGameCore/Rendering/Core/iGameScene.cpp` — 初始化流程按后端能力分支，禁用 OIT/高级 GL 特性
- `iGameCore/IO/iGameFileIO.cpp` — 新增内存输入读取入口并保持原路径 API 向后兼容
- `iGameCore/IO/VTK/iGameVTKReader.cpp` — 复用解析逻辑，支持字节流/内存输入
- `Examples/Rendering/ResetCameraView.cpp` — 作为“读取 vtk -> AddModel -> 渲染”调用链参考模板
- `Examples/Wasm/CMakeLists.txt`（新建）— wasm demo target 与 Emscripten 链接参数
- `Examples/Wasm/main_wasm.cpp`（新建）— wasm 导出 API 与浏览器帧循环接入
- `Examples/Wasm/index.html`（新建）— 文件上传与 wasm API 调用最小页面

**Verification**
1. 本地桌面回归：保持现有示例可编译运行，验证 `ReadFile(vtk)` + `Scene::AddModel` 未退化。
2. wasm 构建验证：使用 Emscripten 工具链构建 `Examples/Wasm` 目标，确认产物完整并可在本地静态服务打开。
3. 端到端功能验证：在页面上传 vtk，观察 wasm 返回解析成功状态，场景首帧渲染成功，`ResetCamera` 生效。
4. 兼容性验证：在不支持高级扩展场景下运行，确认降级路径稳定（无 OIT/meshlet 相关崩溃）。
5. 性能与内存烟雾检查：连续多次上传与切换模型，无明显内存泄漏或上下文丢失。

**Decisions**
- 已确认优先目标为 GLES2/WebGL1，非 WebGL2。
- 已确认网页端服务形态为浏览器内直调 wasm API，不额外建设 HTTP 服务层。
- 首期范围包含 vtk IO + Scene 基础渲染闭环；明确不包含 meshlet、体渲染、OIT 与复杂后处理。
- 参照 VTK 的核心原则：上层渲染 API 稳定，平台后端在底层分流，能力差异通过特性开关与降级处理。

**Further Considerations**
1. 推荐尽早预留 WebGL2 升级路径：若后续需要恢复部分高级渲染，可在不破坏 GLES2 分支的前提下增加 GLES3 后端。
2. 若 `ReadVTKFromMemory` 改造成本偏高，可短期采用 Emscripten MEMFS 写入临时文件再复用 `ReadFile(path)`，但中期建议回归内存流接口。