# Tube 复杂测试

打开 curves.vtp，Apply，再创建 Tube。输入有 605 点、5 条折线：三圈螺旋、波浪线、闭合圆环、带折角曲线、空间波浪线。
包含点数组 Radius（0.04～0.16）、Speed、CurveID、三分量 Velocity，以及单元数组 LineID。
圆环用重复首点编号闭合，末尾有一个未被线使用的孤立点，用于检查孤立点不生成管子。

## 测试矩阵

| 用例 | 设置 | 预期 |
|---|---|---|
| 固定半径 | Vary Radius=Off，Radius=0.1，Number of Sides=24，Capping=Off | 五条中心线均生成同样粗细的管面，开口曲线两端敞开 |
| 封口 | 同上，Capping=On | 开口曲线两端产生端盖；检查闭合圆环接缝是否有伪影，具体闭合处理依版本记录 |
| 截面精度 | 固定半径，Sides 分别为 3、6、32 | 截面分别接近三角形、六边形、圆形；面数量随边数增加 |
| 按标量变化 | Scalars=Radius，Vary Radius=By Scalar，Radius=0.05，Radius Factor=4 | 标量较小位置较细，较大位置较粗；目标半径范围为 0.05～0.2 |
| 绝对半径 | Scalars=Radius，Vary Radius=By Absolute Scalar | 半径直接取 Radius 数值，范围 0.04～0.16；不要用固定 Radius 推算结果 |
| 粗细与颜色独立 | 绝对半径用 Radius，Color By=Speed | 管粗细随 Radius，颜色随 Speed，二者不应混淆 |
| 向量模式 | Vectors=Velocity，Vary Radius=By Vector | 使用通量保持规则；不能把结果理解为半径与速度大小线性正比 |
| 分叉 | 打开 branches.vtp，固定半径 0.1，Sides=24 | 除五条曲线外生成三条共用中心点的管子；检查交界重叠/开口，不能要求自动焊接成水密分叉体 |
| 时间更新 | 打开 animated.pvd，绝对半径，Color By=Speed，逐帧播放 | 几何和 Radius、Speed 同时更新，五条线保持存在；回到 t=0 应恢复初始状态 |

## 时间数值检查

animated.pvd 有 t=0、1、2、3、4 五帧，每帧都是 605 点、5 条折线。
输入点 0（螺旋起点）：

- 坐标：(cos(0.2*t), sin(0.2*t), 0)。
- Radius：0.1+0.06*sin(0.5*t)。
- Speed：1+t；Velocity：(1+t,0,0)。

用绝对半径时，在螺旋起点附近检查管子半径随时间变化。输出表面点与输入点编号不一一对应，不能直接用输出第 0 点当作中心线第 0 点。
可同时显示输入线与输出管子，或对选定位置做截面查看。判断固定/绝对半径时应测量垂直于局部中心线的截面。

Tube 输出是多边形表面，不是填满内部的体网格。不要用体单元数作为成功标准。
记录 ParaView 版本、参数、输出点/单元数量及接缝截图；不同设置会改变输出规模，因此这里不硬编码输出数量。

## 文件验证

运行 `python generate.py` 重新生成文件。脚本检查 XML、属性元组数、线连接索引、offsets 和正半径范围。
已进行文件检查，尚未在 ParaView 实际执行 Tube。
