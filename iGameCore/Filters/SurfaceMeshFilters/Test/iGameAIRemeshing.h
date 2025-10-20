#ifndef IGAME_AI_REMESHING_H
#define IGAME_AI_REMESHING_H

#include <vector>

// Adaptively Isotropic Remeshing Based on Curvature Smoothed Field
// 算法步骤（概要，对应实现中的函数）：
// 1) 预处理与构建邻接：从 faces/points 构建顶点-边-面邻接与边表（无重复、无向、(min,max) 规范）。
// 2) 曲率估计：
//    - 用一环法线变化/二面角近似平均曲率 |H| 或用 cotan Laplacian 近似 Hn；
//    - 取每顶点曲率标量 k = |H|（或 |Hn| 的范数）。
// 3) 曲率平滑：
//    - 在一环上做若干次 Laplacian 平滑（保正值、夹取范围），得平滑曲率场 k_s。
// 4) 目标边长场：
//    - 依据 k_s 映射到目标边长 h(v) = clamp(h_min, h0 / (1 + alpha * k_s), h_max)。
//    - 边的目标长度 h(e) = min(h(va), h(vb))；阈值：分割阈 T_split = (5/3)*h(e)，收缩阈 T_col = (4/5)*h(e)。
// 5) 迭代重网格（N 轮）：
//    5.1 分割长边：若 |e| > T_split 则在中点分割并更新拓扑。
//    5.2 收缩短边：若 |e| < T_col 且合法（不引入翻转/退化/边界违规）则坍缩到中点。
//    5.3 边翻转：若翻转能提升三角质量/顶点度接近6则翻转。
//    5.4 切平面光顺：对非边界点，移动到切平面上一环重心（或 cotan 权重的拉普拉斯步），步长限制。
//    每轮后做一次垃圾回收/重建邻接。
// 6) 输出：更新后的 faces/points。

namespace iGameAI {

struct AIRemeshingOptions {
	int iterations = 5;           // 重网格迭代次数
	int curvatureSmoothIters = 5; // 曲率平滑次数
	float alpha = 1.0f;           // 曲率到边长的敏感度
	float h0 = 1.0f;              // 基准边长尺度（会按平均边长自动估计）
	float hminRatio = 0.3f;       // h_min = h0 * hminRatio
	float hmaxRatio = 2.0f;       // h_max = h0 * hmaxRatio
	float relaxStep = 0.5f;       // 光顺步长（0~1）
};

// 主入口：输入/输出均为紧凑数组
// points: [x0,y0,z0, x1,y1,z1, ...]
// faces:  [i0,j0,k0, i1,j1,k1, ...]
void AdaptivelyIsotropicRemesh(std::vector<float>& points,
								 std::vector<int>& faces,
								 const AIRemeshingOptions& opt = {});

}

#endif


