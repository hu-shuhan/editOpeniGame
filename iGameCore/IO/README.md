# 指标 11.2：vtk/CGNS 数据接口模块

## 模块作用

提供 CAE 仿真数据的 VTK 与 CGNS 格式导入能力，通过统一的 `FileIO` 入口按文件扩展名自动分发到对应读取器，将外部仿真结果转换为 iGameCore 内部的 `DataObject` 数据结构，供后续可视化与分析使用。

主要能力包括：

- Legacy VTK 格式读取（`.vtk`）
- VTK XML 格式读取（`.vtu`、`.vts`、`.vtp`、`.vtm`、`.pvd`）
- CGNS 格式读取（`.cgns`）
- VTK 格式写出
- 统一文件类型识别与读取调度

## 本目录核心实现

| 文件 | 类 | 说明 |
|------|-----|------|
| `iGameFileIO.h/.cpp` | `FileIO` | 统一读写入口，`ReadFile()` 按扩展名分发 |
| `iGameFileReader.h/.cpp` | `FileReader` | 读取器基类 |

## 关联源码路径

- Legacy VTK：[`VTK/iGameVTKReader.h`](VTK/iGameVTKReader.h)、[`iGameVTKWriter.h`](VTK/iGameVTKWriter.h)
- VTK XML：[`VTK XML/iGameVTUReader.h`](VTK%20XML/iGameVTUReader.h)、[`iGameVTSReader.h`](VTK%20XML/iGameVTSReader.h)、[`iGameVTPReader.h`](VTK%20XML/iGameVTPReader.h)、[`iGameVTMReader.h`](VTK%20XML/iGameVTMReader.h)、[`iGamePVDReader.h`](VTK%20XML/iGamePVDReader.h)
- CGNS：[`CGNS/iGameCGNSReader.h`](CGNS/iGameCGNSReader.h)

## 调用方式

### 编程接口：统一读取（推荐）

```cpp
iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(filePath);
```

`FileIO::ReadFile()` 根据文件扩展名自动选择读取器，支持的格式包括 `.vtk`、`.vtu`、`.vts`、`.vtp`、`.vtm`、`.pvd`、`.cgns` 等。

### 编程接口：CGNS 直接读取

```cpp
auto reader = iGame::iGameCGNSReader::New();
auto obj = reader->ReadFile(filePath);
```

### GUI 调用

```
igQtFileLoader::OpenFile(filePath)
  → iGame::FileIO::ReadFile(filePath)
  → Scene::AddModel(obj)
```

用户通过 Qt 主窗口「文件 → 打开」或拖拽文件触发，由 `igQtFileLoader` 调用 `FileIO::ReadFile()` 完成导入。

## 相关示例

| 示例 Target | 说明 |
|-------------|------|
| `testCGNS` | CGNS 文件读取 |