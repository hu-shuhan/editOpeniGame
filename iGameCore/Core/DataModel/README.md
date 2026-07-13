# 指标 11.3：云图 / 自适应矢量场 / 张量场 / 结构形变 / 时序流场 / 动画输出可视化功能模块

## 模块作用

提供 CAE 仿真结果的多类型可视化输出能力，涵盖标量云图、自适应矢量场、张量场、结构形变、时序流场切换和动画视频导出，是连接数据模型与渲染管线的核心可视化层。

主要能力包括：

- 标量云图（Color Map）切换与绘制
- 时序流场帧切换（`UpdateAnimation`）
- 渲染加速选项控制
- 与矢量/张量/形变 Filter 的数据模型绑定

## 本目录核心实现

| 文件 | 类 | 关键方法 | 说明 |
|------|-----|----------|------|
| `iGameDrawObject.h/.cpp` | `DrawObject` | `ViewCloudPicture()` | 标量云图绘制 |
| `iGameDataObject.h/.cpp` | `DataObject` | `UpdateAnimation()` | 时序帧切换 |
| `iGameDrawObject.h/.cpp` | `DrawObject` | `SetAccelerationOption()` | 渲染加速控制 |

## 关联源码路径

- 自适应矢量场：[`../../Filters/VectorView/iGameVectorBase.h`](../../Filters/VectorView/iGameVectorBase.h)
- 张量场：[`../../Filters/TensorView/iGameTensorFilter.h`](../../Filters/TensorView/iGameTensorFilter.h)
- 结构形变：[`../../Filters/Deformation/iGameStressDeformationFilter.h`](../../Filters/Deformation/iGameStressDeformationFilter.h)、[`../../Core/Common/iGameDeformationData.h`](../../Core/Common/iGameDeformationData.h)
- 时序数据读取：[`../../IO/VTK XML/iGamePVDReader.h`](../../IO/VTK%20XML/iGamePVDReader.h)
- 动画视频导出：[`../../IO/FFMPEG/iGameFFMPEGVideoWriter.h`](../../IO/FFMPEG/iGameFFMPEGVideoWriter.h)
- Qt 面板：[`../../../Qt/src/IQWidgets/igQtScalarViewWidget.cpp`](../../../Qt/src/IQWidgets/igQtScalarViewWidget.cpp)、`igQtAnimationWidget`

## 调用方式

### 编程接口：标量云图

```cpp
auto drawObj = iGame::DynamicCast<iGame::DrawObject>(dataObj);
drawObj->ViewCloudPicture(scene, attributeIndex);
```

### 编程接口：时序流场切换

```cpp
// 通过 PVD 读入时序数据后
drawObj->GetTimeFrames()->UpdateAnimation(frameIndex);
vectorView.DrawVector("Velocity", dataObj);
```

### 编程接口：结构形变 + 云图联动

```cpp
drawObj->GetTimeFrames()->UpdateAnimation(keyframeIdx);
if (obj->GetDeformationData()->GetEnableStatus()) {
    auto deformFilter = iGame::StressDeformationFilter::New();
    deformFilter->SetInput(drawObj);
    deformFilter->Execute();
}
drawObj->ViewCloudPicture(scene, drawObj->GetAttributeIndex());
```

### 编程接口：动画导出

```cpp
// 逐帧更新 → 截图 → 写入视频
for (int i = 0; i < frameCount; ++i) {
    drawObj->UpdateAnimation(i);
    scene->Draw();
    scene->CaptureScreen(imagePath);
}
ffmpegWriter->SaveMP4(outputPath);
```

### GUI 调用

| Dock 面板 | 功能 |
|-----------|------|
| `dockWidget_ScalarField` | 标量云图 |
| `dockWidget_VectorField` | 自适应矢量场 |
| `dockWidget_TensorField` | 张量场 |
| 形变面板 | 结构形变 |
| `dockWidget_Animation` | 时序动画播放与导出 |

## 相关示例

| 示例 Target | 说明 |
|-------------|------|
| `testSetScalarField` | 标量云图 |
| `testTimeVaryingVector` | 时序流场 |
| `testTensorView` | 张量场 |
| `testDeformation` | 结构形变 |
| `testAnimation` | 动画播放 |
| `testSaveAnimation` | 动画视频导出 |