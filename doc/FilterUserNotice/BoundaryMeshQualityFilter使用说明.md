# BoundaryMeshQualityFilter 使用说明

## 功能

`BoundaryMeshQualityFilter` 用于评估**体网格 (VolumeMesh)** 的边界质量。它遍历体网格的所有边界面（Boundary Faces），对每个边界面计算其与相邻体单元之间的几何关系，并将结果以单元标量属性（Cell Scalar Attribute）的形式写回到原始输入网格中，输出可直接通过颜色云图进行可视化。

**三种评估指标：**

| 枚举值 | 含义 | 单位 |
|--------|------|------|
| `DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER` | 体单元中心到边界面中心的欧氏距离 | 与网格坐标一致 |
| `DISTANCE_FROM_CELL_CENTER_TO_FACE_PLANE` | 体单元中心到边界面所在平面的垂直距离 | 与网格坐标一致 |
| `ANGLE_FACE_NORMAL_AND_CELL_CENTER_TO_FACE_CENTER_VECTOR` | 边界面的法线方向与体单元中心指向面中心向量之间的夹角 | 度数 (°)，范围 [0, 90] |

**特性：**
- 支持直接输入 `VolumeMesh`，或通过 `UnstructuredMesh`（四面体/六面体等）自动转换
- 对角度指标自动做弧度转度数并归一化到 [0°, 90°] 范围
- 非边界面对应的单元属性值设为 NaN（不参与颜色映射）
- 支持进度报告

---

## 调用方式

### 头文件

```cpp
#include <BoundaryMeshQuality/iGameBoundaryMeshQualityFilter.h>
```

### 基本流程

```cpp
// 1. 读取体网格数据
iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile("./Models/Boundary_Mesh_Quality_Test.vtk");

// 2. 创建 Filter
auto filter = iGame::BoundaryMeshQualityFilter::New();
filter->SetInput(obj);

// 3. 选择评估指标（可选，默认第一个）
filter->SetBoundaryMetric(
    iGame::BoundaryMeshQualityFilter::DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER
);

// 4. 执行
if (!filter->Execute()) {
    std::cerr << "Filter 执行失败: " << filter->GetMessage() << "\n";
    return;
}

// 5. 输出即输入（结果以属性形式追加到原网格）
// filter->GetOutput() == obj
```

### 主要 API

| 方法 | 说明 |
|------|------|
| `SetBoundaryMetric(BoundaryMetric mode)` | 设置评估指标 |
| `GetBoundaryMetric()` | 获取当前指标 |
| `Execute()` | 执行评估 |
| `GetMessage()` | 获取错误/信息消息 |

### BoundaryMetric 枚举

```cpp
enum BoundaryMetric {
    DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER,           // 体心到面心距离
    DISTANCE_FROM_CELL_CENTER_TO_FACE_PLANE,             // 体心到面平面距离
    ANGLE_FACE_NORMAL_AND_CELL_CENTER_TO_FACE_CENTER_VECTOR // 夹角 (°)
};
```

---

## 使用示例

### 示例 1：串行展示三个指标

该 Filter 依次运行三次，每次计算一个指标，弹出一个独立窗口展示颜色云图。关闭当前窗口后进入下一个指标。

```cpp
#include <BoundaryMeshQuality/iGameBoundaryMeshQualityFilter.h>
#include <Core/iGameScene.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iostream>

int main() {
    auto baseScene = iGame::Scene::New();

    // 读取四面体体网格（自动转换为 VolumeMesh）
    const std::string fileName = "./Models/Boundary_Mesh_Quality_Test.vtk";
    iGame::DataObject::Pointer dataObj = iGame::FileIO::ReadFile(fileName);
    if (dataObj != nullptr) {
        baseScene->AddModel(dataObj);
    } else {
        std::cerr << "Read ERROR!\n";
        return -1;
    }

    auto drawObj = DynamicCast<iGame::DrawObject>(dataObj);
    if (!drawObj) {
        std::cerr << "Loaded data is not a DrawObject\n";
        return -1;
    }

    const int baseAttrCount = drawObj->GetAttributeSet()->GetNumberOfAttributes();

    iGame::BoundaryMeshQualityFilter::Pointer filter =
        iGame::BoundaryMeshQualityFilter::New();

    struct MetricEntry {
        const char* title;
        iGame::BoundaryMeshQualityFilter::BoundaryMetric metric;
    };

    const MetricEntry metrics[] = {
        {"Metric 1/3: DistanceFromCellCenterToFaceCenter",
         iGame::BoundaryMeshQualityFilter::DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER},
        {"Metric 2/3: DistanceFromCellCenterToFacePlane",
         iGame::BoundaryMeshQualityFilter::DISTANCE_FROM_CELL_CENTER_TO_FACE_PLANE},
        {"Metric 3/3: AngleFaceNormalAndCellCenterToFaceCenterVector",
         iGame::BoundaryMeshQualityFilter::ANGLE_FACE_NORMAL_AND_CELL_CENTER_TO_FACE_CENTER_VECTOR},
    };

    for (int i = 0; i < 3; ++i) {
        std::cout << ">>> " << metrics[i].title << "\n";

        filter->SetBoundaryMetric(metrics[i].metric);
        filter->SetInput(drawObj);
        if (!filter->Execute()) {
            std::cerr << "Filter execute failed: " << filter->GetMessage() << "\n";
            return -1;
        }

        // 重建 GPU 可绘制数据，使新增属性生效
        drawObj->ConvertToDrawableData();

        const int attrIndex = baseAttrCount + i;

        // 每个窗口使用独立 Scene
        auto scene = iGame::Scene::New();
        scene->AddModel(drawObj);

        iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
        window->SetSize(1920, 1080);
        window->SetScene(scene);

        auto interactor = iGame::Interactor::New();
        interactor->Initialize(scene);
        interactor->CreateDefaultStyle();
        window->SetInteractor(interactor);

        drawObj->ViewCloudPicture(scene, attrIndex, -1);
        window->Show();

        std::cout << "    window closed.\n";
    }

    std::cout << "All three boundary metrics displayed. Done.\n";
    return 0;
}
```

### 示例 2：单独计算一种指标

```cpp
    auto filter = iGame::BoundaryMeshQualityFilter::New();

    // 只需一种指标时直接设置并执行
    filter->SetBoundaryMetric(
        iGame::BoundaryMeshQualityFilter::DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER
    );
    filter->SetInput(volumeMeshObj);
    filter->Execute();
```

---

## 注意事项

1. **输入类型要求**：
   - 直接支持 `VolumeMesh` 类型输入
   - 也支持 `UnstructuredMesh`（如四面体网格），Filter 会自动调用 `ConvertToVolumeMeshFilter` 转换为体网格
   - 其他类型（表面网格、点云等）返回 `false`

2. **体网格必须有边界面**：完全封闭的体网格（如纯内腔体网格）可能无边界面，此时返回 `false` 并给出提示

3. **非边界单元属性为 NaN**：不属于任何边界面的单元，其属性值被设为 NaN，在颜色映射中不显示

4. **角度归一化**：夹角指标以度数表示，且被归一化到 [0°, 90°] 范围，值为 0° 表示面法线与体心方向完全一致，90° 表示法线与体心方向垂直

5. **重复执行会累加属性**：每次 `Execute()` 都会向 AttributeSet 追加一个新属性。若需重新计算，应在执行前清空属性或重新加载数据

6. **测试模型**：`Examples/Models/Tet_Plane.vtk`（四面体非结构化体网格，2×2×2 规则布点，约 40 个四面体单元，边界分明）

7. **依赖**：需要 `ConvertToVolumeMeshFilter` 用于将 `UnstructuredMesh` 转换为 `VolumeMesh`，确保面表和面-体邻接关系已构建

---

## 相关 Filter

- `iGameMeshQualityFilter`：评估体单元整体质量（如 Jacobian、偏度等）
- `iGameConvertToVolumeMeshFilter`：将 UnstructuredMesh 转换为 VolumeMesh
- `iGameExtractSurfaceFilter`：从体网格中提取表面网格
