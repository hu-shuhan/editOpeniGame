# 指标 11.3：云图 / 自适应矢量场 / 张量场 / 结构形变 / 时序流场 / 动画输出可视化功能模块

## 模块作用

提供 CAE 仿真结果的多类型场可视化输出：标量云图、自适应矢量场、张量场、结构形变、流线时序流场与动画导出。

## 源码路径

### 数据模型与云图（核心层）

| 路径 | 关键 API | 说明 |
|------|----------|------|
| `iGameCore/Core/DataModel/iGameDrawObject.*` | `ViewCloudPicture()` | 标量云图 |
| `iGameCore/Core/DataModel/iGameDataObject.*` | `UpdateAnimation()` | 时序帧切换 |
| `iGameCore/Core/DataModel/iGameDrawObject.*` | `SetAccelerationOption()` | Meshlet 加速 |

### 场可视化 Filters

| 路径 | 说明 |
|------|------|
| `iGameCore/Filters/VectorView/iGameVectorBase.*` | 自适应矢量场（AllCell / CellInRange / EveryNth） |
| `iGameCore/Filters/TensorView/iGameTensorFilter.*` | 张量场 |
| `iGameCore/Filters/Deformation/iGameStressDeformationFilter.*` | 结构形变 |
| `iGameCore/Filters/StreamView/iGameStreamTracer.*` | 流线追踪 |
| `iGameCore/Filters/Contour/iGameContourFilter.*` | 等值面 |

### IO 与动画

| 路径 | 说明 |
|------|------|
| `iGameCore/IO/VTK XML/iGamePVDReader.*` | 时序 PVD |
| `iGameCore/IO/FFMPEG/iGameFFMPEGVideoWriter.*` | 动画视频导出（需 FFMPEG） |

## 调用方式

### 标量云图

```cpp
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
drawObj->ViewCloudPicture(scene, attributeIndex);
```

### 时序流场切换

```cpp
drawObj->UpdateAnimation(keyframeIdx);
vectorView.DrawVector("Velocity", dataObj);
```

### 流线追踪

```cpp
auto streamBase = iGame::StreamBase::New();
auto streamtracer = streamBase->streamFilter;
streamtracer->initStreamTracer(dataObj);
streamtracer->SetInput(seeds, vectorName, lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
streamtracer->Execute();
streamBase->SetUpdate(true);
scene->AddModel(streamBase);
```

### 自适应矢量场

```cpp
iGame::iGameVectorBase vectorView;
vectorView.SetDrawMode(iGame::iGameVectorBase::AllCell);
vectorView.DrawVector("Velocity", dataObj);
```

### 结构形变

```cpp
auto deformFilter = iGame::StressDeformationFilter::New();
deformFilter->SetInput(drawObj);
deformFilter->Execute();
```

### 动画导出

```cpp
for (int i = 0; i < frameCount; ++i) {
    drawObj->UpdateAnimation(i);
    scene->Draw();
    scene->CaptureScreen(imagePath);
}
// FFMPEG writer → MP4
```

### GUI

| Dock / 面板 | 功能 |
|-------------|------|
| `dockWidget_ScalarField` | 云图 |
| `dockWidget_VectorField` | 矢量场 |
| `dockWidget_TensorField` | 张量场 |
| `dockWidget_FlowField` | 流线 |
| `dockWidget_Animation` | 时序动画 |
| `DeformationDockWidget`（代码动态创建） | 结构形变 |
| `dockWidget_ContourExtract` | 等值面 |

## 相关示例

| 示例 Target | 说明 | 条件 |
|-------------|------|------|
| `testSetScalarField` | 云图 | 默认 |
| `testTimeVaryingVector` | 时序流场 | 默认 |
| `testStreamline` | 流线 | 默认 |
| `testVector` / `testVectorAllCell` / `testVectorCellInRange` / `testVectorEveryNth` | 矢量场 | 默认 |
| `testTensorView` | 张量场 | 默认 |
| `testDeformation` | 结构形变 | 默认 |
| `testContourLine` | 等值面 | 默认 |
| `testAnimation` | 动画播放 | 默认 |
| `testSaveAnimation` | 动画导出 | `FFMPEG_FOUND` |
