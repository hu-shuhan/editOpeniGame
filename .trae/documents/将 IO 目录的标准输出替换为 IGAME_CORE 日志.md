## 目标
- 将 `/iGameCore/IO/` 中实际代码里的 `std::cout/std::cerr/std::endl` 全部替换为 `IGAME_CORE_TRACE`、`IGAME_CORE_DEBUG`、`IGAME_CORE_ERROR`。
- 保留注释中的示例/被注释的旧代码，不做改动。

## 映射规则
- `std::cerr` → `IGAME_CORE_ERROR`。
- `std::cout` → 默认 `IGAME_CORE_DEBUG`；若为高频、逐帧/逐包/循环内的详细打印，则改为 `IGAME_CORE_TRACE`。
- `std::endl` 的换行与强制刷新由日志系统接管；使用格式化字符串输出（`"... {} ..."`）。

## 必要包含
- 在使用上述宏的文件顶部统一引入 `iGameCore/Core/Common/Log/iGameLogger.h`（若尚未引入）。

## 典型改造示例
- `std::cout << "Loaded " << path << std::endl;` → `IGAME_CORE_DEBUG("Loaded {}", path);`
- `std::cerr << "Error reading file: " << filename << std::endl;` → `IGAME_CORE_ERROR("Error reading file: {}", filename);`
- 高频循环内（如 FFMPEG 编码过程的进度/帧信息）：`std::cout << "frame " << i << std::endl;` → `IGAME_CORE_TRACE("frame {}", i);`

## 重点文件与级别策略
- `IO/Fluent/iGameCASReader.cpp`：`std::cerr` 全改 `ERROR`；普通流程信息改 `DEBUG`，循环/进度改 `TRACE`。
- `IO/FFMPEG/iGameFFMPEGVideoWriter.cpp`：大量逐帧/逐包日志使用 `TRACE`；阶段性状态使用 `DEBUG`；异常使用 `ERROR`。
- 其余 Reader/Writer（`CGNS/`、`Abaqus/`、`INP/`、`IMAGE/`、`VTK XML/`、`PVD/` 等）按以上规则统一替换。

## 实施步骤
1. 扫描并标注所有非注释的 `std::cout/std::cerr/std::endl` 位置。
2. 在每个文件顶部检查并补充 `iGameLogger.h` 引入。
3. 逐处替换为对应 `IGAME_CORE_*` 宏，改为格式化字符串写法，去除 `std::endl`。
4. 对高频打印统一降级到 `TRACE`，避免运行时日志噪音与性能影响。
5. 编译并运行基本场景，验证日志输出内容与行为（尤其是 FFMPEG 与各 Reader）。
6. 若需即时刷新，统一改为通过日志配置控制（如对 `ERROR` 级别即时 flush）。

## 验证与回滚
- 在不改变逻辑的前提下，保持原有消息文本内容；如遇到复杂串接，使用格式化拼装。
- 编译并运行关键模块，比对日志行为与原有输出含义；如有异常，逐文件回滚到最近修改点。

## 交付范围
- 仅改动 `/iGameCore/IO/` 目录下实际代码中的标准输出；不影响注释和其他模块。请确认后我将开始逐文件替换与验证。