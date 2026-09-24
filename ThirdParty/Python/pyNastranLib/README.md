# Nastran BDF/OP2 → VTK 转换器

此目录保存 `nastran_to_vtk_cli.exe` 的可维护源码、锁定依赖、PyInstaller 配置和构建记录。转换器基于 pyNastran 1.4.1 官方 Nastran GUI/VTK 管线，目标平台为 Windows x64。

## 命令行

```powershell
.\nastran_to_vtk_cli.exe -b model.bdf -p model.op2 -o model.bdf.vtu --force
```

- `-b, --bdf FILE`：必需，支持 `.bdf`、`.dat`、`.nas`。
- `-p, --op2 FILE`：可选，仅支持 `.op2`。
- `-o, --output FILE`：必需，支持 `.vtu` 和 `.vtk`。
- `-l, --log-level`：`debug`、`info`、`warning` 或 `error`。
- `-c, --compression 0..9`：默认 `0`，生成未压缩 VTU；`1..9` 对 VTU 使用 LZMA。
- `--force`：允许覆盖已有输出。
- `--validate`：转换前校验 BDF，转换后用 VTK 重新读取输出。

输出先写入目标目录中的临时文件，写入及可选校验成功后再通过原子替换发布。因此转换失败不会覆盖已有 VTU，也不会留下半成品。

默认 VTU 使用 appended base64、UInt32 header 且不压缩，供项目内 `iGameVTUReader` 使用。该 Reader 不支持 LZMA，因此主程序调用时不要传非零压缩级别。

## 可复现构建

安装官方 CPython 3.12.10 x64 后，在 PowerShell 中执行：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build_exe.ps1
```

脚本严格检查 Python 版本，按 `requirements-build.txt` 创建构建环境，并将候选文件写到 `dist\nastran_to_vtk_cli.exe`。候选文件通过源码/EXE 测试和项目 Reader 验收前，不应替换本目录的发布 EXE。

真实验收模型从 `D:\Models\Models\Nastran` 读取，不复制到仓库。

源码和候选 EXE 共用同一验收脚本：

```powershell
python .\verify_cli.py .\nastran_to_vtk_cli.py --full
python .\verify_cli.py .\dist\nastran_to_vtk_cli.exe --full
```

## 发布流程

1. 对候选 EXE 完成 BDF-only、BDF+OP2、错误输入、空格/中文路径和陈旧输出测试。
2. 使用 `ogs` 与 `wingbox_stitched_together-000` 对点、单元、连接关系、类型和关键结果数组验收。
3. 使用 `iGameVTUReader` 和 `testNastranReader` 读取输出。
4. 将当前发布 EXE 按 SHA-256 前缀备份，再原位替换并更新 `CHECKSUMS.sha256`。

CBUSH 应力映射不匹配、GridPointForce 等 pyNastran 1.4.1 不支持的结果可能产生警告并被跳过；只要其余转换及写出成功，命令仍返回 0。
