# AngularPeriodic 使用说明

角度周期复制（Angular Periodic Copy）过滤器：把一个网格绕指定轴，以给定周期角度（相邻两份之间的旋转角）旋转复制 N 份并合并成一个 UnstructuredMesh 输出。第 0 份为原始网格（不旋转），第 i 份旋转 i×周期角度。参数定义与 ParaView 的 `vtkAngularPeriodicFilter` 保持一致。

## 一、功能

- 输入：单个网格（SurfaceMesh / UnstructuredMesh / VolumeMesh / StructuredMesh / 点云 PointSet 均可）。
- 输出：合并后的 UnstructuredMesh，包含原始网格 + 各旋转副本。
- PointData / CellData 属性按份复制到输出，并按属性语义同步旋转：3 分量向量/法向量（IG_VECTOR / IG_NORMAL）随几何旋转，张量（IG_TENSOR，9 分量或 6 分量对称）按 R·T·Rᵀ 旋转；标量、纹理坐标、RGB 及整型数组原样复制。
- 支持任意大小单元：>16 点多边形、多面体（按核心面编码处理）都不会被丢弃或溢出。
- 复制结果会继承输入模型的可视化状态：颜色映射表/数值范围、视图样式，以及**当前正在着色的属性**（按属性名匹配），因此热力图/云图在新模型上保持不变。

## 二、界面入口

主界面 →【算法处理】菜单 → **角度周期复制 (Angular Periodic)**，弹出参数对话框：

| 参数 | 含义 | 默认 |
| --- | --- | --- |
| 轴预设 (Axis) | X 轴 / Y 轴 / Z 轴 / 自定义；选择后自动回填到轴输入框 | Z 轴 |
| 原点预设 (Origin) | 世界原点 / 模型中心 / 自定义；选择后自动回填到原点输入框 | 世界原点 |
| origin_x/y/z | 旋转轴经过的点 | 0 / 0 / 0 |
| axis_x/y/z | 旋转轴方向（自动归一化） | 0 / 0 / 1 |
| 周期数量 (Periods) | 总份数（含原始网格） | 4 |
| 周期角度 (Angle°) | 相邻两份之间的旋转角度：第 i 份旋转 i×angle | 90 |
| 份数模式 (Iteration) | `指定份数 (Direct)`：用上面的周期数量；`自动填满一周 (Max)`：自动取 floor(360/angle) 份（对齐 vtkPeriodicFilter 的 IterationMode） | 指定份数 |
| 要求整周闭合 (Require Full Period) | 勾选后，若 份数×角度 ≠ 360° 则执行失败 | 不勾选 |
| 显示旋转轴 (Show Axis) | 打开对话框即显示橙色旋转轴预览，随轴/原点/勾选实时更新；关闭对话框自动移除（不加入模型树） | 勾选 |

对话框会实时显示预计覆盖情况：`整周闭合` / `缺口 x°` / `重叠 x°`，以及各份角度（如 `0°、90°、180°、270°`）。默认只提示不拦截。

点击【应用】执行；失败会弹窗显示原因（如轴为零向量、输入为空、勾选整周闭合但未闭合）。

## 三、调用方式（代码）

```cpp
#include "Periodic/iGameAngularPeriodicFilter.h"

auto filter = iGame::AngularPeriodicFilter::New();
filter->SetInput(dataObj);                                   // 输入网格
filter->SetRotationAxis(Point(ox, oy, oz), Vector3d(ax, ay, az)); // 轴过 (ox,oy,oz)、方向 (ax,ay,az)
filter->SetNumberOfCopies(copies);
filter->SetAngle(angleDeg);                                  // 周期角度（相邻两份间隔）
filter->SetIterationMode(AngularPeriodicFilter::ITERATION_MODE_MAX); // 可选：自动填满一周
filter->SetRequireFullPeriod(true);                          // 可选：要求整周闭合，否则失败
if (!filter->Execute()) {
    std::cerr << filter->GetMessage() << "\n";               // 失败原因
    return 1;
}
std::cout << filter->GetCoverageInfo() << "\n";              // 整周闭合/缺口/重叠 + 各份角度
std::cout << filter->GetEffectiveNumberOfCopies() << "\n";   // 实际份数（MAX 下自动计算）
auto output = filter->GetOutput();                           // 合并后的 UnstructuredMesh
```

## 四、使用示例（自动测试，无需手动输入）

仓库自带两个程序化/AI 生成模型（`Examples/Models/`）：

- `AIGen_Surface_RingSector.obj`：120° 环形扇区，绕 Z 轴 3 份 360° 可拼满整圈（示例默认用它）。
- `AIGen_Surface_FinBlade.obj`：离轴单叶闭式桨叶，旋转复制后呈扇叶效果。

运行（构建后工作目录为 `Examples`，路径写死、自动读取）：

```powershell
# Windows（Visual Studio 生成器）
cd cmake-build-sync\Examples
.\Release\testAngularPeriodic.exe

# CTest
ctest -R testAngularPeriodic
```

无 GUI 自动回归（程序内构造网格，断言全部 PASS 返回 0）：

```powershell
.\Release\testAngularPeriodicSelfCheck.exe   # 73 项检查 PASS
ctest -R testAngularPeriodicSelfCheck
```

## 五、注意事项

- `angle` 是**周期角度**（相邻两份之间的旋转角），第 i 份旋转 `i×angle` 度，不是"总角度"；`copies` 含原始网格。例如 angle=90、copies=4 得到 0°/90°/180°/270° 四份。
- 份数模式（对齐 `vtkPeriodicFilter::IterationMode`）：`Max` 取 `floor(360/angle)`，只可能产生缺口、不会重叠；`Direct` 下缺口/重叠都可能出现。`GetCoverageInfo()` 返回覆盖描述，默认只提示不使执行失败，仅当 `SetRequireFullPeriod(true)` 时才因未闭合而失败。
- 旋转轴为零向量、输入为空、`angle<=0`、Direct 下份数 < 1 会执行失败，通过 `GetMessage()` 获取原因。
- 属性按语义同步旋转：3 分量向量/法向量随几何旋转，张量按 R·T·Rᵀ 旋转，标量等原样复制（与 ParaView vtkAngularPeriodicFilter 一致）。
- 对话框打开时即**实时预览旋转轴**（橙色中轴），跟随轴/原点/显示勾选更新；关闭对话框自动移除，不会残留在模型树。复制结果继承输入的颜色映射与当前着色属性，热力图无需手动重设。
- 多面体/StructuredMesh 输入在过滤器内部会先物化为规范单元，顶点号偏移由过滤器处理，使用者无需关心。
- 运行时工作目录必须含 `Models`（构建阶段 `iGameCopyExampleAssets` 会把 `Examples/Models` 拷到构建目录）；直接双击 `Release` 下的 exe 而工作目录不对会读不到模型。

## 六、相关文件

| 文件 | 说明 |
| --- | --- |
| `iGameCore/Filters/Periodic/iGameAngularPeriodicFilter.h/.cpp` | 过滤器实现 |
| `Examples/Filter/Periodic/TestAngularPeriodic.cpp` | GUI 示例（默认读 `AIGen_Surface_RingSector.obj`，自动运行） |
| `Examples/Filter/Periodic/TestAngularPeriodicSelfCheck.cpp` | 无 GUI 自动回归（73 项断言） |
| `Examples/Models/AIGen_Surface_RingSector.obj` | 120° 环形扇区测试模型（程序化/AI 生成） |
| `Examples/Models/AIGen_Surface_FinBlade.obj` | 桨叶测试模型（程序化/AI 生成） |
