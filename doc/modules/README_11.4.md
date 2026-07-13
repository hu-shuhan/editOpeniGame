# 指标 11.4：CAE 仿真结果高精并行可视化软件

## 模块作用

`iGameCore` 是 iGameVis 的核心库，将 IO、Filters、DataModel 与 Rendering 整合为统一的 CAE 高精并行可视化平台，支持大规模网格的高效绘制与交互分析。

主要能力：

- 统一数据对象模型（`DataObject` / `DrawObject`）
- 多线程并行（`ThreadPool::parallelFor`）
- OpenGL 场景渲染与 Meshlet GPU 加速
- 与 Qt 前端及 Examples 完整集成

## 源码路径

| 子目录 | 职责 |
|--------|------|
| `iGameCore/Core/` | 基础对象、单元模型、网格数据结构 |
| `iGameCore/Filters/` | 特征提取、场可视化、形变、可视分析 |
| `iGameCore/IO/` | 多格式读写 |
| `iGameCore/Rendering/` | Scene、OpenGL、Meshlet、交互器 |
| `Qt/` | GUI 主窗口与 Dock 面板 |

关联实现：

- `iGameCore/Core/Common/iGameThreadPool.h`
- `iGameCore/Rendering/Core/Meshleter/`
- `iGameCore/Rendering/Core/iGameScene.cpp`

## 调用方式

### GUI 路径

```
main.cpp → igQtMainWindow → igQtFileLoader::OpenFile()
         → FileIO::ReadFile() → Scene::AddModel()
         → Qt Dock 调用 Filters / DrawObject
```

### Meshlet 并行加速

```cpp
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
drawObj->SetAccelerationOption(true);
scene->AddModel(dataObj);
```

### 多线程并行

```cpp
iGame::ThreadPool::parallelFor(0, count, [&](int i) { /* ... */ });
```

## 相关示例

| 示例 Target | 说明 |
|-------------|------|
| `testMeshletRendering` | Meshlet GPU 加速 |
| `testSetRenderingPressure` | 渲染压力自适应与交互简化 |

编译示例需 `EXAMPLE_COMPILE=ON`。

## 关联指标

本模块为平台总览，各子能力详见 [doc/modules/README.md](README.md) 中 7.1、10.x、11.x 文档。
