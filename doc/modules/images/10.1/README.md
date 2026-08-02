# 指标 10.1 文档截图

本目录存放 [README_10.1.md](../../README_10.1.md) / [README_10.1.en.md](../../README_10.1.en.md) 引用的截图。

## 待补充清单

| 文件名 | 对应图号 | 内容说明 |
|--------|---------|---------|
| `local-region-chart.png` | 图 1-1 | 3D 视图中选点 / 框选，右侧同步显示局部区域的径向图与探针线图表 |
| `multi-chart-linkage.png` | 图 1-2 | 同一选区下并行坐标 / 变量相关性 / 变量密度四类图表的联动效果 |
| `entropy-seeding-global.png` | 图 2-1 | 未框选状态下，整模型范围内按全局熵排名生成的流线分布 |
| `entropy-seeding-selection.png` | 图 2-2 | 框选局部区域后，熵排序限制在选区内生成的流线分布（与图 2-1 对比） |
| `entropy-seeding-selectbox.png` | 图 2-3 | 3D 视图中拖拽选择盒限定分析区域的交互过程 |
| `simplifier-before.png` | 图 3-1 | 一次布种生成的全部流线，密集遮挡状态 |
| `simplifier-after.png` | 图 3-2 | 聚类筛选后保留的代表流线，按 ClusterLabel 着色 |
| `simplifier-params.png` | 图 3-3 | clusterSpin / perClusterSpin 参数设置与 Cluster 按钮 | 


## 截图规范建议

- **格式**：PNG（界面截图，避免 JPEG 压缩导致文字发虚）
- **宽度**：1200~1600 px，保证文档中缩放后细节仍可辨认
- **主题**：统一使用软件深色主题，保持视觉一致
- **对比图**（图 2-1 / 2-2、图 3-1 / 3-2）：使用**同一模型、同一视角、同一相机距离**，仅改变待说明的变量，便于读者直接对比
- **界面元素**：涉及参数面板的截图请包含完整的参数控件，必要时用红框标注关键控件

## 插入方式

补充截图后，将文档中对应位置的占位块替换为标准 Markdown 图片语法：

```markdown
![全局信息熵种子生成结果](images/10.1/entropy-seeding-global.png)

> **图 2-1　全局信息熵种子生成结果**
```
