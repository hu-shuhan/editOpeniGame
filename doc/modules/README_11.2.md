# 指标 11.2：vtk/CGNS 数据接口模块

## 模块作用

提供 CAE 仿真数据的 VTK 与 CGNS 格式导入（及 VTK 写出），通过统一 `FileIO::ReadFile()` 按扩展名分发读取器。

## 源码路径

| 路径 | 说明 |
|------|------|
| `iGameCore/IO/iGameFileIO.*` | 统一入口 `FileIO::ReadFile()` |
| `iGameCore/IO/iGameFileReader.*` | 读取器基类 |
| `iGameCore/IO/VTK/iGameVTKReader.*` | Legacy VTK |
| `iGameCore/IO/VTK XML/iGameVTUReader.*` 等 | VTU/VTS/VTP/VTM/PVD |
| `iGameCore/IO/CGNS/iGameCGNSReader.*` | CGNS（可选模块） |

## 调用方式

### 统一读取（推荐）

```cpp
iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(filePath);
```

支持扩展名包括：`.vtk`、`.vtu`、`.vts`、`.vtp`、`.vtm`、`.pvd`、`.cgns` 等。

### CGNS 直接读取

```cpp
auto reader = iGame::iGameCGNSReader::New();
auto obj = reader->ReadFile(filePath);
```

> CGNS 需编译时开启 `ENABLE_CGNS_MODULE=ON`（定义 `CGNS_ENABLE`）。

### GUI

```
igQtFileLoader::OpenFile(filePath)
  → FileIO::ReadFile(filePath)
  → Scene::AddModel(obj)
```

## 相关示例

| 示例 Target | 条件 |
|-------------|------|
| `testCGNS` | `ENABLE_CGNS_MODULE=ON` |

## 说明

本指标聚焦 VTK/CGNS。Nastran、Fluent CAS、Abaqus 等接口见 `iGameCore/IO/` 其他子目录。
