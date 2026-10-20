# igvDump — 命令行 + Information 面板（GUI 模式）

> 工具位置：`Examples/IO/igvDump.cpp`（命令行）与 `Examples/IO/IGVDumpInfoWindow.cpp`（GUI）。
> 构建产物：`build/<dir>/Examples/Release/igvDump.exe`（一个 exe，两种模式）。

---

## 1. 两种模式，一个可执行文件

| 用法 | 输出 | 面向 |
|---|---|---|
| `igvDump <file.igv>` | 结构 dump（组 / dataset / dtype / chunk / attribute），可 diff、可贴 issue | 排查"文件里**有什么**" |
| `igvDump <file.igv> --data [--limit N]` | 同上 + 每个 dataset 的前 N 个值 | 核对具体数值 |
| `igvDump <file.igv> --gui` | **Information 面板**：File Properties / Data Statistics / Data Arrays | 看"这些数据**是什么**" |
| `igvDump --gui` | 空面板，拖放文件或 `文件 → 打开…` | 先开工具再选文件 |
| **双击 `igvDump.exe`（无参数）** | 同上：**空面板**（不是打一行用法就退出） | 日常"先开工具、再拖文件" |
| **双击 / 右键"打开方式" 一个 `.igv`** | 直接打开该文件的**面板** | 日常看图 |
| `igvDump <file.igv> --text` | 强制文本模式 | 脚本/批处理 |
| `igvDump --gui --screenshot out.png [file.igv]` | 渲染一帧到 PNG 后退出（UI 冒烟 / 文档截图） | 无交互环境验证布局 |
| `igvDump --self-check` | 自检 29 项断言（CTest 目标） | CI |

参数顺序随意：`igvDump --gui a.igv`、`igvDump a.igv --gui`、`igvDump a.igv --gui --screenshot x.png`
等价。

### 1.1 模式是怎么选的（重要：脚本行为不变）

只有**没有显式指定模式**时才会推断：

| 启动方式 | 有没有自己的控制台 | 结果 |
|---|---|---|
| 资源管理器双击 `igvDump.exe`（无参数） | 有（系统为它新建） | **GUI 空面板** |
| 资源管理器双击 / 右键"打开方式"一个 `.igv` | 有 | **GUI 打开该文件** |
| `cmd` / PowerShell 里 `igvDump a.igv` | 没有（共用 shell 的） | **文本 dump**（旧行为，一字未变） |
| `igvDump a.igv --data` / `--limit N` | 任意 | **文本**（这些参数本身就是文本模式的） |
| 计划任务 / 重定向 / 没有控制台 | 没有 | **文本**（不会弹窗把进程挂住） |
| 显式 `--gui` / `-g` | 任意 | **GUI** |
| 显式 `--text` / `--console` | 任意 | **文本** |

判据是 Windows 的 `GetConsoleProcessList()`：**只有本进程时**说明那个控制台是双击/右键
启动时系统为它单独创建的（若是从终端启动，列表里还有那个 shell）。GUI 模式下会顺手把这个
独占的黑窗口隐藏掉 —— 从终端启动时**不会**动你的终端窗口。

> 这也是为什么"从终端敲 `igvDump`"仍然打印用法（旧行为）而不是弹窗：
> 弹窗会让脚本、CTest、CI 里的"少写参数"变成挂住。用法文本随时可以用 `--help` 拿到。

> **UI 冒烟**（不需要人盯着屏幕，也不需要桌面会话能截屏）：
> ```powershell
> cd build\igv_msvc_release\Examples\Release
> .\igvDump.exe --gui --screenshot shot.png "..\..\..\..\Examples\Models\Bike.igv"
> ```
> 退出码 0 且生成 `shot.png` 即说明"元数据读取 + 面板构建 + 布局 + 渲染"整条链通了；
> 保存失败返回 4，便于接进脚本。截图是窗口**自己渲染**的（`QWidget::grab()`），不是截屏。

两条路径读的是**同一份元数据**，形态不同而已 —— 所以它们给出的点数/单元数/Bounds/range
永远一致；不一致就是 bug。

**向后兼容**：`--data` / `--limit` / `--self-check` 的语义与退出码一字未改；
从终端执行时的裸文件参数仍然走文本模式。需要"绝对确定是文本"的脚本加 `--text` 即可。

退出码：

| 码 | 含义 |
|---|---|
| 0 | 成功（GUI 模式下 = 正常关闭窗口） |
| 1 | 文本模式读文件失败 |
| 2 | 参数错误（含缺文件参数、或"从终端无参数启动"） |
| 3 | `--gui` 但**这个构建没有 Qt**（见 §5） |
| 4 | `--screenshot` 写 PNG 失败 |

---

## 2. Information 面板长什么样

三段，照 ParaView 的 Information 面板：

```
File Properties
  Name            can.igv
  Path            C:\data\can.igv
  Size on disk    3.86000 MB
  IGV Version     1.0
  IGVDataType     UnstructuredMesh
  Generator       iGameVis IGVWriter

Data Statistics
  Type            UnstructuredGrid  (UnstructuredMesh)
  # of Cells      4,800
  # of Points     6,724
  Connectivity ids 24,000
  Memory (data)   1.02539 MB
  Bounds          X: -12.1696 to 6.00287  (delta: 18.1725)
                  Y: -3.60042 to 5.5622   (delta: 9.16263)
                  Z: -18.3355 to -13.4565 (delta: 4.87902)
  FixedCellSize   8（定长单元，读回走快速路径）

Data Arrays                                    (过滤: ______________)
  Scope   Name              Type             Components  Ranges
  ● point f_scalar          Float32(32bit)   1           [-80080944, 47764300]
  ● point d_scalar          Float64(64bit)   1           [0.0215831, 2.89394]
  ● cell  cell_vec          Float64(64bit)   3           [1.7, 22.4], [-1, 3], [0, 4], [2, 9]
```

要点：

* **Scope 的小圆点**：绿 = `point`、蓝 = `cell`、灰 = 其它（`edge`/`point_box`/`change`…）。
  与 `PointData` / `CellData` / `FieldData` 三个组一一对应。
* **Type 列带位宽**：`Float64(64bit)`、`Int16(16bit)`、`UInt8(8bit)` ……
  类型名是**定宽**的（不用 VTK 的 `long`/`int`/`double` 这类会随平台变化的写法），
  位宽来自文件里的 `DataTypeBits` attribute。鼠标悬停可看到
  `DataType` + 位宽 + 对应的 `VTK_DOUBLE / double / float64` + `AttributeType`。
  1.0 老文件没有这两个 attribute，此时**按 HDF5 类型推断**出同样的定宽名与位宽
  （实测 P0 手写样例 `example_unstructured.igv` 显示 `Float64(64bit)` / `Float32(32bit)`），
  所以新老文件的显示口径一致；只有 HDF5 类型本身无法归类时才会显示 `?(Nbit)`。
* **Ranges** 就是文件里 `/IGV/*Data/<name>` 上的 `range` attribute，**不是现算的**。
  `components == 1` 是一对 `[min, max]`；多分量是 `[模长 min, 模长 max], [c0 min, c0 max], …`
  （与 `kia/igv分阶段实现方案.md` §3.6 / §8.2 的定义一致）。所以这个面板同时可以当
  "写出侧的 range 对不对"的检查入口。
* **Memory (data)** 是"这些数组进内存大概多大"的**估算**（点坐标 + 连通性 + 偏移 + 单元类型 +
  各属性），不等同于文件大小（gzip 后更小），也不等同于峰值内存（读出时还有副本）。

### 2.1 面板里的 dtype 覆盖哪些类型

IGV 的属性数组支持 **10 类定宽标量**（1.x 起，见 `kia/数据类型扩展实施记录.md`）：

| 面板显示 | VTK 对应 | NumPy |
|---|---|---|
| `Int8(8bit)` | `VTK_CHAR` / signed char | `int8` |
| `UInt8(8bit)` | `VTK_UNSIGNED_CHAR` | `uint8` |
| `Int16(16bit)` | `VTK_SHORT` | `int16` |
| `UInt16(16bit)` | `VTK_UNSIGNED_SHORT` | `uint16` |
| `Int32(32bit)` | `VTK_INT`（以及 4 字节平台上的 `VTK_LONG`） | `int32` |
| `UInt32(32bit)` | `VTK_UNSIGNED_INT`（以及 4 字节平台上的 `VTK_UNSIGNED_LONG`） | `uint32` |
| `Int64(64bit)` | `VTK_LONG_LONG`、`VTK_ID_TYPE`（8 字节平台上的 `VTK_LONG`） | `int64` |
| `UInt64(64bit)` | `VTK_UNSIGNED_LONG_LONG` | `uint64` |
| `Float32(32bit)` | `VTK_FLOAT` | `float32` |
| `Float64(64bit)` | `VTK_DOUBLE` | `float64` |

`VTK_BIT`（1 bit）落成 `UInt8`（值域 {0,1} 不变、体积 ×8）；`VTK_LONG` / `VTK_ID_TYPE`
不写"名字"，只写实际宽度——这正是面板要显示位宽的原因。

---

## 3. 面板能做什么

| 操作 | 入口 | 快捷键 |
|---|---|---|
| 打开文件 | 工具条 / `文件 → 打开…` | `Ctrl+O` |
| 重新读取（覆盖外部改动后刷新） | 工具条 / `文件 → 重新读取` | `F5` |
| 复制全部为文本（贴 issue 用） | 工具条 / `编辑 → 复制全部为文本` | `Ctrl+Shift+C` |
| 按属性名过滤（不区分大小写） | 工具条右侧输入框 | — |
| 拖放打开 | 把 `.igv` 拖到窗口里 | — |
| 选中/复制单元格文本 | 表格里直接拖选，或值区选中文字 | — |

「复制全部为文本」的输出与面板**同构**（三段 + 表格，Type 列同样带位宽），末尾带一行
`(metadata only: N H5Dread call(s), no dataset was decompressed)`。

---

## 4. 右键「通过 igvDump 打开」

只写 `HKEY_CURRENT_USER`（**不需要管理员**），并且只用 `SystemFileAssociations`，
也就是"给 `.igv` 增加一个右键动词"，**不改** `.igv` 的默认关联、不动已有的「打开方式」项。

### 推荐：用脚本（自动填路径）

```powershell
# 注册（默认找 build\igv_msvc_release\Examples\Release\igvDump.exe）
powershell -ExecutionPolicy Bypass -File Examples\IO\igvDump_OpenWith.ps1

# 或者指定 exe 路径
powershell -ExecutionPolicy Bypass -File Examples\IO\igvDump_OpenWith.ps1 -Exe "D:\tools\igvDump.exe"

# 先看看会写哪条命令（不碰注册表）
powershell -ExecutionPolicy Bypass -File Examples\IO\igvDump_OpenWith.ps1 -WhatIf

# 取消注册
powershell -ExecutionPolicy Bypass -File Examples\IO\igvDump_OpenWith.ps1 -Uninstall
```

`-WhatIf` 的输出会打印出将要写入的完整命令，例如：

```
What if: Performing the operation "Register the context-menu command:
  "C:\...\Release\igvDump.exe" --gui "%1"" on target "HKCU:\...\igvDumpInfo".
```

### 手工：`igvDump_OpenWith.reg`

1. 用文本编辑器打开 `Examples/IO/igvDump_OpenWith.reg`；
2. 把所有 `__IGVDUMP_EXE__` 换成 `igvDump.exe` 的**绝对路径**，反斜杠要写双份：
   `C:\\Projects\\iGameVis_Stable_Branch\\build\\igv_msvc_release\\Examples\\Release\\igvDump.exe`；
3. 双击合并。

两种方式写入的注册表项是同一条：

```
HKCU\Software\Classes\SystemFileAssociations\.igv\shell\igvDumpInfo
    (Default) = "Open with igvDump (Information)"
    Icon      = <igvDump.exe 绝对路径>
    command\(Default) = "<igvDump.exe 绝对路径>" --gui "%1"
```

> `.reg` 文件刻意写成**纯 ASCII**（注释也是英文）：regedit 对 UTF-8 的 `.reg` 不友好，
> 中文注释会变成乱码。需要中文说明就看本文件。

---

## 5. 构建前提与常见问题

| 现象 | 原因 | 处置 |
|---|---|---|
| `--gui` 打印 `GUI mode is not available in this build` 并返回 3 | 构建里没有 Qt（`ENABLE_QT_MODULE=OFF`，或 `find_package(Qt5 Widgets)` 没找到） | 用 Qt 构建重配：`cmake -DENABLE_QT_MODULE=ON ...`；命令行模式不受影响 |
| **双击 exe 完全没有窗口**，只在终端里打了一段用法（exit 2） | 这是**旧版本**的行为：当时无参数启动会直接打用法退出。现在已修：无参数 + 独占控制台 = 开空面板（见 §1.1） | 重新构建 `igvDump`；或在终端里用 `igvDump --gui` 显式打开 |
| **双击 exe / 右键打开，窗口一闪而过**，没有任何提示 | exe 同目录缺 Qt DLL（`0xC0000135`）。用 `igvDump_Package.ps1` 打的便携包里已经齐了；直接从 `build\...\Release` 复制单个 exe 出去就会这样 | 重新构建 `igvDump` 目标让 post-build 的 `windeployqt` 跑起来；或按 §8.2 手工补齐，或直接用 §8.1 的打包脚本 |
| 双击 `.igv` 弹出的是**黑窗口+文本**而不是面板 | 该机器上 `.igv` 被关联到了别的打开方式，或关联命令没带 `--gui` | 用 `igvDump_OpenWith.ps1` 注册右键项（它带的命令是 `--gui "%1"`）；显式参数请用 `igvDump --gui a.igv` |
| 从终端跑 `igvDump a.igv` 想直接看面板 | 终端启动时控制台不是独占的，按 §1.1 走文本模式 | 加 `--gui`：`igvDump a.igv --gui` |
| 从 `cmd` 里启动 GUI，终端里多出一行 "no such file"? | 不是：控制台窗口会在"本进程独占控制台"时**自动隐藏**，从终端启动时**保持不动**（判据是 `GetConsoleProcessList()`，见源码注释） | — |
| 面板显示"无法读取该文件的 IGV 元数据：…"，错误里提到 `MultiBlock` / `time series` | 该文件是多块或单文件内嵌时变，**本期不支持**（P6 的显式拒绝，不静默丢数据） | 用能识别的版本，或先用 `igvDump <file>` 文本模式看结构 |
| 面板显示"…declares DataType=… but its HDF5 type is …" | 文件的显式 dtype 声明与 HDF5 实际类型矛盾（写侧 bug 或文件被改坏） | 属文件问题；文本模式可看到 `ATTRIBUTE "DataType"` |
| 面板显示 `Bounds: (文件未写 Bounds attribute)` | 该文件不是本规范写出，或写出侧没算 Bounds | 属文件问题；文本模式可看到 `ATTRIBUTE "Bounds"` 是否存在 |
| 状态栏显示 `H5Dread: N`，N > 1 | 有人在快路径里读了数据（应为 1，仅 `Version`） | 这是回归信号；见 `kia/igv分阶段实现方案.md` §11.3 |

---

## 6. 为什么不重算 range / 不加载网格

* **不加载**：面板只调 `IGVMetadataReader::ReadFromFile`（HDF5 attribute + dataspace），
  因此 5 GiB 的 `.igv` 也是瞬时打开，也不会把内存吃满。
* **不重算**：显示文件里预计算好的 `range`，这样"面板显示的范围"与"文件声称的范围"
  是同一个数 —— 一旦写出侧算错，面板会**如实**显示出错的那个值（重算反而会掩盖它）。
* **只读**：不写文件、不改 attribute、不加锁（`H5Fopen` 只读模式）。

---

## 7. 能不能打包成**单个 exe**（静态链接所有 DLL）？—— 可行但**不推荐**

先看真实依赖（`dumpbin /DEPENDENTS`）：

```
igvDump.exe -> Qt5Widgets.dll, Qt5Gui.dll, Qt5Core.dll, hdf5.dll,
               MSVCP140.dll, VCRUNTIME140.dll, VCRUNTIME140_1.dll, <系统 DLL>
hdf5.dll    -> 只有系统 DLL + VCRUNTIME140（zlib/zstd 已静态编进 hdf5.dll）
```

也就是说非系统依赖只有 **Qt 的 3–4 个 DLL + hdf5.dll + 3 个 MSVC 运行时 DLL**。
把它们"塞进 exe"有三条路，结论如下：

| 方案 | 可行性 | 结论 |
|---|---|---|
| **Qt 静态链接**（`QT_STATIC` + `-static`） | 需要**静态构建的 Qt**。本仓库用的是官方 msvc2019_64 **共享**版，它不提供静态库 | **不做**。要为它单独编一套 Qt（数小时 + 长期维护），且 **LGPL 下静态链接**还需提供可重链接的目标文件或商业许可 |
| **HDF5 静态链接** | 若安装包里有静态库（`libhdf5.lib`）可以链进去，去掉 `hdf5.dll` | **收益低**：只省 1 个 DLL（5.3 MB），却把构建绑死在"这份 HDF5 必须是静态库"上；CI/他人机器换个 HDF5 包就编不过 |
| **MSVC 运行时 `/MT`** | 单目标改 `/MT` 与本项目 `/MD` 编出的 `iGameCore.lib` 混链会出 ODR/CRT 冲突 | **不做**：正确做法就是带上 3 个运行时 DLL（或用系统已装的 VC++ Redistributable） |

**推荐做法：一个"便携文件夹"**（下一节），而不是单文件 exe。
理由很实在：双击一个文件夹里的 `igvDump.exe` 和双击单个 exe，对用户没有区别；
而静态链接会把"构建一次、到处能跑"变成"为一台机器单独编一套工具链"。

---

## 8. 便携安装：复制哪些文件 / 一键打包

### 8.1 一条命令搞定（推荐）

```powershell
powershell -ExecutionPolicy Bypass -File Examples\IO\igvDump_Package.ps1
# 想放到别处、顺便出一张 UI 截图：
powershell -ExecutionPolicy Bypass -File Examples\IO\igvDump_Package.ps1 -OutDir D:\tools\igvDump -VerifyGui
```

它做的事（每一步都可复查）：

1. 复制 `igvDump.exe`；
2. `windeployqt --no-translations --no-opengl-sw --no-compiler-runtime` 取 Qt 运行时；
3. 用 `dumpbin /DEPENDENTS` **递归**解析导入表，把每个非系统 DLL 从构建目录/Qt 目录拷过来
   （系统 DLL 与 `api-ms-win-*` 一律不拷）；
4. 打印清单 + **验证**：把 `PATH` 临时缩到"包目录 + 系统目录"再跑 `igvDump.exe --self-check`
   —— 这一条才真正证明"没有偷偷依赖本机的 Qt/HDF5 安装"。

本机实测产物（`build/igvDump_portable`，共 **24 个文件 / 31.8 MiB**）：

```
igvDump.exe                       1,854,976
hdf5.dll                          5,280,768
Qt5Core.dll / Qt5Gui.dll / Qt5Widgets.dll / Qt5Svg.dll
platforms\qwindows.dll            1,477,104     <- 少了它 GUI 起不来
styles\qwindowsvistastyle.dll
imageformats\*.dll, iconengines\qsvgicon.dll
libEGL.dll / libGLESv2.dll        (Qt 的 ANGLE，GUI 备用)
MSVCP140.dll / MSVCP140_1.dll / VCRUNTIME140.dll / VCRUNTIME140_1.dll
```

> 首次运行后包目录下会多一个 `logs\`（iGameCore 的日志），属正常现象，删掉无影响。
> 脚本自身的验证是在临时目录里跑的，所以**刚打好的包里不会有这个目录**。

### 8.2 手工复制的最小集合

万一不想跑脚本，照这个列表拷（**目录结构要保留** `platforms\`）：

| 来源 | 文件 |
|---|---|
| `build\<dir>\Examples\Release` | `igvDump.exe`、`hdf5.dll`、`MSVCP140.dll`、`MSVCP140_1.dll`、`VCRUNTIME140.dll`、`VCRUNTIME140_1.dll` |
| `C:\Qt\5.15.2\msvc2019_64\bin` | `Qt5Core.dll`、`Qt5Gui.dll`、`Qt5Widgets.dll`、`Qt5Svg.dll` |
| `...\plugins\platforms` | `qwindows.dll` → 放到 `platforms\qwindows.dll` |
| `...\plugins\styles`（可选） | `qwindowsvistastyle.dll` → `styles\` |

> 只要这四个 Qt DLL + `platforms\qwindows.dll` 齐了，面板就能起来；
> `imageformats\*` 与 ANGLE 两个 DLL 在纯看数据时用不到（面板不加载图片），
> 想再小一点可以删，但**脚本默认保留**以避免别的机器上出现意外缺件。

### 8.3 装到"日常使用"的位置

```powershell
# 1) 打包到工具目录（例如 D:\tools\igvDump）
powershell -ExecutionPolicy Bypass -File Examples\IO\igvDump_Package.ps1 -OutDir D:\tools\igvDump

# 2) 注册右键（指向这个便携目录里的 exe）
powershell -ExecutionPolicy Bypass -File Examples\IO\igvDump_OpenWith.ps1 -Exe "D:\tools\igvDump\igvDump.exe"

# 3) 想放到桌面/开始菜单：给 exe 建快捷方式即可（无需安装程序）
```

升级时重新跑一次脚本（它会**清空重建**输出目录）＋ 顺手把 `D:\tools` 里旧的替换掉即可；
注册表项只需要注册一次（它记的是路径）。

> 卸载：`igvDump_OpenWith.ps1 -Uninstall` + 删掉那个文件夹。**不写注册表别处、不写系统目录、
> 不需要管理员权限。**

---

## 9. 相关文件

| 文件 | 作用 |
|---|---|
| `Examples/IO/igvDump.cpp` | 命令行主体 + `--gui` 分发 + 模式推断 + 自检 |
| `Examples/IO/IGVDumpInfoWindow.cpp/.h` | Qt 面板实现（无 `Q_OBJECT`，不需要 moc） |
| `Examples/IO/igvDump_Package.ps1` | **便携包一键打包 + 依赖递归解析 + PATH 隔离验证** |
| `Examples/IO/igvDump_OpenWith.ps1` | 右键注册/注销（推荐，支持 `-WhatIf` / `-Uninstall`） |
| `Examples/IO/igvDump_OpenWith.reg` | 右键注册的注册表模板（手工用） |
| `iGameCore/IO/IGV/iGameIGVMetadataReader.h` | 面板唯一的数据来源 |
| `kia/数据类型扩展实施记录.md` | 10 类定宽 dtype 与 `DataType`/`DataTypeBits` 的来龙去脉 |
| `kia/igv分阶段实现方案.md` §7 / §8 / §11.3 | 元数据快路径、range 语义、零解压断言 |
