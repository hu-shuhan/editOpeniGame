#include "iGamePoints.h"
#include "iGameTetra.h"
#include "iGameDelaunayTetrahedralization.h"

IGAME_NAMESPACE_BEGIN
// 计算四面体外接球的中心和半径
void iGameDelaunayTetrahedralization::ComputeCircumcenter(const Point& p0, const Point& p1, const Point& p2, const Point& p3, Point& center, double& radius) {
	// 通过求解四面体外接球方程得到球心和半径
	double A = p0[0], B = p0[1], C = p0[2];
	double D = p1[0], E = p1[1], F = p1[2];
	double G = p2[0], H = p2[1], I = p2[2];
	double J = p3[0], K = p3[1], L = p3[2];

	// 计算相关矩阵的行列式
	double a = A * (E * (I - L) + F * (K - H) + G * (L - K)) -
		B * (D * (I - L) + F * (J - G) + G * (L - J)) +
		C * (D * (K - H) + E * (J - G) + F * (G - H));

	double b = -(A * (E * (I - K) + F * (K - G) + G * (H - K)) -
		B * (D * (I - K) + F * (J - G) + G * (H - J)) +
		C * (D * (H - G) + E * (J - G) + F * (G - H)));

	double c = A * (E * (J - G) + F * (H - J) + G * (F - H)) -
		B * (D * (J - G) + F * (K - F) + G * (E - K)) +
		C * (D * (F - H) + E * (K - H) + F * (K - F));

	double d = -(A * (D * (J - G) + E * (K - H) + F * (L - J)) -
		B * (D * (I - G) + F * (K - J) + G * (L - K)) +
		C * (D * (K - F) + E * (L - G) + F * (I - K)));

	// 计算球心坐标
	double det = 2 * (A * (B * (I - L) + C * (K - H)) -
		B * (C * (I - L) + A * (K - H)) +
		C * (A * (I - L) - B * (K - H)));

	center = Point(a / det, b / det, c / det);

	// 计算半径
	radius = (center - p0).length();
}

// 判断点是否在四面体外接球内
bool iGameDelaunayTetrahedralization::IsPointInCircumball(const Point& p, const Point& center, double radius) {
	return (center - p).length() < radius;
}

// 创建初始四面体
std::vector<Tetra::Pointer> iGameDelaunayTetrahedralization::InitTetras() {
	// 假设四个初始点已经定义为凸四面体
	std::vector<Tetra::Pointer> initial_Tetra;
	Tetra::Pointer Tetra = Tetra::New();

	//todo 创建初始四面体
	initial_Tetra.push_back(Tetra); // 假设点 0, 1, 2, 3 已经初始化
	return initial_Tetra;
}

void iGameDelaunayTetrahedralization::AddPointAndUpdate(std::vector<Point>& points, std::vector<Tetra::Pointer>& tetrahedra, const Point& new_point) {
	// 步骤1: 查找包含新点的"坏的"四面体
	std::vector<Tetra::Pointer> bad_tetrahedra;

	for (const auto& tetra : tetrahedra) {
		Point p0 = points[tetra->m_PointIds->GetId(0)];
		Point p1 = points[tetra->m_PointIds->GetId(1)];
		Point p2 = points[tetra->m_PointIds->GetId(2)];
		Point p3 = points[tetra->m_PointIds->GetId(3)];

		// 计算外接球的球心和半径
		Point center;
		double radius;
		ComputeCircumcenter(p0, p1, p2, p3, center, radius);

		// 如果新点在外接球内部，说明该四面体是坏的
		if (IsPointInCircumball(new_point, center, radius)) {
			bad_tetrahedra.emplace_back(tetra);
		}
	}

	// 步骤2: 删除坏的四面体
	for (const auto& bad_tetra : bad_tetrahedra) {
		// 删除坏的四面体
		tetrahedra.erase(std::remove(tetrahedra.begin(), tetrahedra.end(), bad_tetra), tetrahedra.end());
	}

	// 步骤3: 创建新的四面体
	std::vector<Tetra::Pointer> new_tetrahedra;
	for (const auto& bad_tetra : bad_tetrahedra) {
		// 提取坏四面体的四个顶点
		int v0 = bad_tetra->m_PointIds->GetId(0);
		int v1 = bad_tetra->m_PointIds->GetId(1);
		int v2 = bad_tetra->m_PointIds->GetId(2);
		int v3 = bad_tetra->m_PointIds->GetId(3);

		// 为每个坏的四面体面生成一个新的四面体
		// 新四面体的顶点为新点与原四面体的每个面组合
		Tetra::Pointer newTetra = Tetra::New();
		//存放数据然后放到数组里面去

		//new_tetrahedra.push_back(Tetra(v0, v1, v2, points.size())); // 新点插入
		//new_tetrahedra.push_back(Tetra(v0, v1, v3, points.size())); // 新点插入
		//new_tetrahedra.push_back(Tetra(v0, v2, v3, points.size())); // 新点插入
		//new_tetrahedra.push_back(Tetra(v1, v2, v3, points.size())); // 新点插入
	}

	// 步骤4: 将新点插入到点列表
	points.push_back(new_point);

	// 步骤5: 将新生成的四面体添加到四面体列表中
	tetrahedra.insert(tetrahedra.end(), new_tetrahedra.begin(), new_tetrahedra.end());
}

// 基于给定的点集合生成四面体
std::vector<Tetra::Pointer> iGameDelaunayTetrahedralization::GenerateTetrahedraFromPoints(Points::Pointer originPoints) {
	std::vector<Tetra::Pointer> tetrahedra;

	// 步骤 1: 创建初始的四面体
	// 假设 originPoints 中已经包含了四个顶点，这里初始化时生成一个凸四面体
	std::vector<Tetra::Pointer> initial_tetrahedra = InitTetras();  // 这个函数可能根据你的数据结构返回一些初始四面体

	// 将这些初始的四面体加入到四面体列表中
	tetrahedra.insert(tetrahedra.end(), initial_tetrahedra.begin(), initial_tetrahedra.end());
	std::vector<Point> points;

	// 步骤 2: 对每一个新点进行插入
	for (int i = 0; i < originPoints->GetNumberOfPoints(); i++) {
		AddPointAndUpdate(points, tetrahedra, originPoints->GetPoint(i));
	}


	return tetrahedra;
}

// 执行测试函数，使用传入的 originPoints
void iGameDelaunayTetrahedralization::ExecuteTest(Points::Pointer originPoints) {

	// 生成四面体
	std::vector<Tetra::Pointer> tetrahedra = GenerateTetrahedraFromPoints(originPoints);

	// 打印四面体信息
	for (const auto& tetra : tetrahedra) {
		std::cout << "Tetra vertices: ";
		for (int i = 0; i < 4; i++) {
			std::cout << tetra->m_PointIds->GetId(i) << " ";
		}
		std::cout << std::endl;
	}
	return;
}


IGAME_NAMESPACE_END