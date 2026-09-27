# L04 VTM Debug 验证与修复记录

日期：2026-08-22
配置：Windows x64 Debug、MSVC 14.39、Qt 5.14.2、Desktop OpenGL 4.6
输入：`D:/1000000000_surface/drivaer/L04/DRIVAER-SURF-L04.vtm`

## 数据基准

- VTM 子块：512，引用连续且无缺失。
- 三角形 Cell：347,193,344。
- 点记录：175,135,610。
- Connectivity 索引项：1,041,580,032（每个三角形 3 项）。
- 单块规模：677,888～678,144 个三角形，340,313～344,470 个点。
- 全局包围盒：`[-7.69736, -4.4572, -0.12720084]` ～ `[12.58664, 4.4572, 4.4104]`。
- 全部单元类型为 VTK_TRIANGLE；512 个 VTU 的 inline-zlib 数组均可完整解码，连接索引均在各自局部点范围内。

## 已定位问题及修复

1. VTM 路径解析只按 `/` 截取，Windows 反斜杠入口不可靠；改用 `std::filesystem` 解析、拼接和规范化相对路径。
2. 子文件不存在、格式不支持或读取失败时会被静默跳过，空/不完整 MultiBlock 仍可能返回成功；现在任一子块失败即明确报错并终止，同时校验最终块数。
3. 纯二维 VTU 仍经过通用几何抽取并深复制点、面和属性；增加纯表面快速路径，点和 Cell 安全共享，属性容器独立而底层数组共享。
4. 每个小于 100 万面的 VTM 子块仍无条件生成并上传不会被交互路径使用的 20% 简化网格；现在只对超过交互阈值的表面建立简化 LOD。
5. Surface 模式也无条件建立全部边和 Line EBO；改为按需建立。DesktopGL 的不透明 `Surface + Wireframe` 使用三角形 edge mask 单遍绘制，纯 Wireframe 等显式线框路径才建立边。
6. 表示模式状态向子对象重复递归并错误地把源壳对象标脏；修正传播与脏标记，在 Surface/Wireframe、透明度、opacity mapping 和 acceleration 状态切换时只重建必要的可渲染叶对象。
7. VTU 单元转换为每个 Cell 反复创建临时 `std::vector`，Debug 堆开销很高；改为循环外复用缓冲区，并保留异常多面体输入的初始化语义。
8. VTM 日志现在输出总文件数、逐文件进度，以及最终 piece、点记录和 Cell 汇总，便于定位读取与渲染阶段。

## 自动回归

新增 `testVTMReaderPathValidation`，覆盖：

- 相对 `pieces/...` 路径；
- Windows 原生反斜杠入口路径；
- 缺失子件必须失败；
- 纯二维共享数据与独立 AttributeSet owner；
- 默认 Surface 不建立显式边；
- DesktopGL `Surface + Wireframe` 单遍路径；
- 切换为纯 Wireframe 后按需建立 3 条边。

直接运行通过；相关 CTest 组合也全部通过：

- `testVTKXMLDataArrayDecoderValidation`
- `testVTMReaderPathValidation`
- `testModelGeometryParallelValidation`

结果为 3/3 passed，0 failed。

## L04 实测结果

- 完整读取成功：512/512。
- UI 识别为多块网格，块数 512；坐标范围与独立解码基准一致。
- 优化版完整加载后进程仍响应，无 VTM/VTU 解析错误、OpenGL OOM 或上下文丢失日志。
- 一次完整实测峰值约：Working Set 23.9 GiB，Private Bytes 33.2 GiB；RTX 3090 显存相对加载前增加约 6.36 GiB，仍有约 11.8 GiB 空闲。
- 人工视觉验收：当前实现的 L04 Surface 已正常显示，整体位置与 Outline 对齐。

## 已知边界

- 本次验收使用 Solid Color。不要在全量 L04 上直接启用 `PatchId` Cell Coloring；当前 Cell 颜色路径会把每个三角形展开为 3 个位置和颜色记录，额外显存需求可超过 24 GiB。
- L04 是约 3.47 亿个三角形 Cell，但 connectivity 已约 10.42 亿项。它验证的是“十亿 connectivity 级”，不是“十亿 Cell 级”。
- 本修复保证 L04 的读取与默认 Surface 显示；FINAL 的 10 亿三角形仍需要分块驻留/LOD 或其他显存预算机制，不能由本次 L04 成功直接外推。

## 构建产物

- Debug 可执行文件：`D:/iGameVis-main/out/build/x64-Debug/iGameVis.exe`
- SHA-256：`2E9E6F7676D15C149F84CD562972314380993470A9725661C6D76F1F6B8695EC`
