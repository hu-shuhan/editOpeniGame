# ShrinkFilter（单元向质心收缩）使用说明

## 一、功能

ShrinkFilter 参照 VTK 的 vtkShrinkFilter 实现：把网格中每个单元（面片 / 体单元）的所有顶点
向该单元的质心（几何中心）方向移动，使相邻单元彼此分开，形成"爆炸图"效果，便于查看网格结构
和单元之间的连接关系。

支持三类网格：

- 表面网格（SurfaceMesh）
- 体网格（VolumeMesh，注意其继承自 SurfaceMesh，须优先判断）
- 非结构化网格（UnstructuredMesh）

## 二、调用方式

### 1. 命令行 / 代码调用

```cpp
#include <Shrink/iGameShrinkFilter.h>

auto filter = iGame::ShrinkFilter::New();
filter->SetShrinkFactor(0.5);   // 收缩比例：0~1，0 收缩到质心，1 不变，默认 0.5
filter->SetInput(0, mesh);
if (filter->Execute()) {
    // 成功后可经 GetOutput() 取得结果（结果与输入为同一网格对象）
}
```

### 2. 可视化界面调用

1. 打开 iGameVis，加载模型（表面 / 体 / 非结构化网格均可）；
2. 菜单：**算法处理 → 单元收缩 (Shrink)**；
3. 在弹出窗口中输入收缩比例（0~1，如 0.5），点击应用；
4. 模型各单元向质心收缩，呈现爆炸图效果；可反复修改比例重新生成（每次基于原始模型计算）。

## 三、使用示例

`Examples/Filter/Shrink/TestShrinkModel.cpp` 提供了自动测试示例，无需手动输入：

```powershell
cmake --build build --config Release --target testShrinkModel --parallel
F:\igamevis\build\Examples\Release\testShrinkModel.exe
```

示例会自动读取 `Examples/Models/` 下的两个测试模型并核对结果：

- `Shrink_Cube.vtk`：三角面片立方体，收缩 0.5 后顶点由 8 变为 36（每个面片独立顶点），面片数不变；
- `Shrink_TwoTets.vtk`：两个四面体并带 `Pressure` 点标量，收缩 0.5 后顶点由 5 变为 8，
  且点标量被正确复制到全部新顶点。

运行结束应输出 `ALL TESTS PASSED`。

## 四、注意事项

1. `SetShrinkFactor` 会把输入自动限制在 0~1：1 表示不收缩，0 表示收缩到质心一点。
2. 收缩会为每个单元复制一份顶点，因此顶点数会明显增多（相邻单元不再共享顶点），这是预期行为。
3. 点属性（包括多分量属性）会随顶点一起复制并保持数值一致；单元属性不受影响。
4. 模型文件需要放在仓库的 `Examples/Models/` 目录下，示例使用相对路径自动定位，无需手动输入路径。
