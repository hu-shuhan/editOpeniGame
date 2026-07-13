# 指标 11.4：CAE 仿真结果高精并行可视化软件

## 模块作用

`iGameCore` 是 iGameVis 的核心库，面向 CAE 仿真结果提供高精并行可视化能力。它将数据导入（IO）、算法处理（Filters）、数据模型（Core/DataModel）与渲染管线（Rendering）整合为统一的可视化平台，支持大规模网格的高效绘制与交互分析。

主要能力包括：

- 统一的数据对象模型（`DataObject` / `DrawObject`）与属性管理
- 多线程并行计算（`ThreadPool::parallelFor`）
- OpenGL 场景渲染与 Meshlet GPU 加速绘制
- 与 Qt 前端及 Examples 示例程序的完整集成

## 本目录结构

| 子目录 | 职责 |
|--------|------|
| `Core/` | 基础对象、单元模型、网格数据结构 |
| `Filters/` | 特征提取、流场/矢量/张量、形变、可视分析等算法 |
| `IO/` | VTK/CGNS/Spline 等格式读写 |
| `Rendering/` | Scene、OpenGL 渲染、Meshlet 加速、交互器 |

## 关联源码路径

- 并行线程池：[`Core/Common/iGameThreadPool.h`](Core/Common/iGameThreadPool.h)
- Meshlet 加速：[`Rendering/Core/Meshleter/`](Rendering/Core/Meshleter/)
- 场景渲染：[`Rendering/Core/iGameScene.cpp`](Rendering/Core/iGameScene.cpp)

## 调用方式

### GUI 应用路径

```
main.cpp → igQtMainWindow → igQtFileLoader::OpenFile()
         → FileIO::ReadFile() → Scene::AddModel()
         → Qt Dock 面板调用 Filters / DrawObject
```

### 编程接口：启用 Meshlet 并行加速

```cpp
auto dataObj = iGame::FileIO::ReadFile(fileName);
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
drawObj->SetAccelerationOption(true);
scene->AddModel(dataObj);
```

### 编程接口：多线程并行

```cpp
iGame::ThreadPool::parallelFor(0, count, [&](int i) {
    // 并行处理逻辑
});
```

## 相关示例

| 示例 Target | 说明 |
|-------------|------|
| `testMeshletRendering` | Meshlet GPU 加速渲染 |
| `testSetRenderingPressure` | 渲染压力自适应与网格简化 |

编译示例需开启 `EXAMPLE_COMPILE=ON`。