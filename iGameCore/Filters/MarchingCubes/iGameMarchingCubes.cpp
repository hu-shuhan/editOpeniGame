#include "iGameMarchingCubes.h"

IGAME_NAMESPACE_BEGIN
namespace MarchingCubes
{
	// 线性插值函数
	Point_3D VertexInterp(Point_3D p1, Point_3D p2, float iso_val) {
		Point_3D result;

		if (std::abs(iso_val - p1.value) < 1e-6) { return p1; }

		if (std::abs(iso_val - p2.value) < 1e-6) { return p2; }

		if (std::abs(p1.value - p2.value) < 1e-6) { return p1; }

		float alpha = (iso_val - p1.value) / (p2.value - p1.value);

		result.x = p1.x + alpha * (p2.x - p1.x);
		result.y = p1.y + alpha * (p2.y - p1.y);
		result.z = p1.z + alpha * (p2.z - p1.z);
		result.value = iso_val;
		/*if (abs(result.x) > 1) {
				std::cout << p1.x << " " << alpha << " " << p2.x - p1.x << " " << p2.x << " " << p1.x << " " << p1.value<<" "<<p2.value<<std::endl;
			}
			if (abs(result.y) > 1) {
				std::cout << p1.y << " " << alpha << " " << p2.y - p1.y << " " << p2.y << " " << p1.y << " " << p1.value << " " << p2.value << std::endl;
			}
			if (abs(result.z) > 1) {
				std::cout << p1.z << " " << alpha << " " << p2.z - p1.z << " " << p2.z << " " << p1.z << " " << p1.value << " " << p2.value << std::endl;
			}*/

		return result;
	}

	// 返回
	Point_3D point_return_by_ratio(Point_3D p1, Point_3D p2, float ratio) {
		if (ratio > 1 || ratio < 0) {
			std::cout << "error_ratio" << std::endl;
			return Point_3D();
		}
		Point_3D ans;
		ans.x = ratio * p1.x + (1 - ratio) * p2.x;
		ans.y = ratio * p1.y + (1 - ratio) * p2.y;
		ans.z = ratio * p1.z + (1 - ratio) * p2.z;
		return ans;
	}


	// Two_Triangle_3D marchingTetrahedrons(Tetrahedron tet, float iso_value) {
	//	int index_for_table = 0;
	//	if (tet.p[0].value > iso_value) index_for_table |= 1;
	//	if (tet.p[1].value > iso_value) index_for_table |= 2;
	//	if (tet.p[2].value > iso_value) index_for_table |= 4;
	//	if (tet.p[3].value > iso_value) index_for_table |= 8;

	//	Two_Triangle_3D two_tri;
	//	two_tri.num = 0;

	//	if (index_for_table == 5 || index_for_table == 10) {
	//		// 使用统一的方式处理这两种情况
	//		Point_3D temp = point_return_by_ratio(tet.p[0], tet.p[2], 0.5);
	//		temp.value = iso_value - ((tet.p[0].value + tet.p[2].value) - 2 * iso_value);
	//		std::cout << "temp.value = " << temp.value << std::endl;

	//		two_tri.t[0].p[0] = VertexInterp(tet.p[1], tet.p[2], iso_value);
	//		two_tri.t[0].p[1] = VertexInterp(temp, tet.p[2], iso_value);
	//		two_tri.t[0].p[2] = VertexInterp(tet.p[3], tet.p[2], iso_value);

	//		two_tri.t[1].p[0] = VertexInterp(tet.p[0], tet.p[1], iso_value);
	//		two_tri.t[1].p[1] = VertexInterp(tet.p[0], tet.p[3], iso_value);
	//		two_tri.t[1].p[2] = VertexInterp(tet.p[0], temp, iso_value);
	//		return two_tri;
	//	}

	//	for (int i = 0; i <= 6; i += 6) {
	//		if (table[index_for_table][i] != -1) {
	//			two_tri.t[two_tri.num].p[0] = VertexInterp(tet.p[table[index_for_table][i]], tet.p[table[index_for_table][i + 1]], iso_value);
	//			two_tri.t[two_tri.num].p[1] = VertexInterp(tet.p[table[index_for_table][i + 2]], tet.p[table[index_for_table][i + 3]], iso_value);
	//			two_tri.t[two_tri.num].p[2] = VertexInterp(tet.p[table[index_for_table][i + 4]], tet.p[table[index_for_table][i + 5]], iso_value);
	//			two_tri.num++;
	//		}
	//	}
	//	return two_tri;
	//}

	Two_Triangle_3D marchingTetrahedrons(Tetrahedron tet, float iso_value) {
		int index_for_table = 0;
		if (tet.p[0].value > iso_value) { index_for_table |= 1; }
		if (tet.p[1].value > iso_value) { index_for_table |= 2; }
		if (tet.p[2].value > iso_value) { index_for_table |= 4; }
		if (tet.p[3].value > iso_value) { index_for_table |= 8; }
		// 好像因为差值问题对5和10要做特殊处理
		// 好像没问题，是书错了
		/*if (index_for_table == 5) {
				Two_Triangle_3D two_tri;
				Point_3D temp = point_return_by_ratio(tet.p[0], tet.p[2], 0.5);
				temp.value = iso_value - ((tet.p[0].value + tet.p[2].value) - 2 * iso_value);
				std::cout <<"temp.value = " << temp.value << std::endl;
				two_tri.t[0].p[0] = VertexInterp(tet.p[1], tet.p[2], iso_value);
				two_tri.t[0].p[1] = VertexInterp(temp, tet.p[2], iso_value);
				two_tri.t[0].p[2] = VertexInterp(tet.p[3], tet.p[2], iso_value);

				two_tri.t[1].p[0] = VertexInterp(tet.p[0], tet.p[1], iso_value);
				two_tri.t[1].p[1] = VertexInterp(tet.p[0], tet.p[3], iso_value);
				two_tri.t[1].p[2] = VertexInterp(tet.p[0], temp, iso_value);
				return two_tri;
			}
			if (index_for_table == 10) {
				Two_Triangle_3D two_tri;
				Point_3D temp = point_return_by_ratio(tet.p[0], tet.p[2], 0.5);
				temp.value = iso_value - ((tet.p[0].value + tet.p[2].value) - 2 * iso_value);
				std::cout << "temp.value = " << temp.value << std::endl;
				two_tri.t[0].p[0] = VertexInterp(temp, tet.p[2], iso_value);
				two_tri.t[0].p[1] = VertexInterp(tet.p[1], tet.p[2], iso_value);
				two_tri.t[0].p[2] = VertexInterp(tet.p[3], tet.p[2], iso_value);

				two_tri.t[1].p[0] = VertexInterp(tet.p[0], tet.p[3], iso_value);
				two_tri.t[1].p[1] = VertexInterp(tet.p[0], tet.p[1], iso_value);
				two_tri.t[1].p[2] = VertexInterp(tet.p[0], temp, iso_value);
				return two_tri;
			}*/
		Two_Triangle_3D two_tri;
		two_tri.num = 0;
		for (int i = 0; i <= 6; i = i + 6) {
			if (table[index_for_table][i] != -1) {
				two_tri.t[two_tri.num].p[0] = VertexInterp(
					tet.p[table[index_for_table][i]],
					tet.p[table[index_for_table][i + 1]], iso_value);
				two_tri.t[two_tri.num].p[1] = VertexInterp(
					tet.p[table[index_for_table][i + 2]],
					tet.p[table[index_for_table][i + 3]], iso_value);
				two_tri.t[two_tri.num].p[2] = VertexInterp(
					tet.p[table[index_for_table][i + 4]],
					tet.p[table[index_for_table][i + 5]], iso_value);
				two_tri.num++;
				// debug用
				if (abs(two_tri.t[two_tri.num - 1].p[1].z) > 1) {
					std::cout << index_for_table << std::endl;
				}
			}
		}
		return two_tri;
	}

	Ten_Triangle_3D marchingCubes(Cube c, float iso_value) {
		Tetrahedron tet[5];
		tet[0].p[0] = c.p[0];
		tet[0].p[1] = c.p[5];
		tet[0].p[2] = c.p[7];
		tet[0].p[3] = c.p[4];

		tet[1].p[0] = c.p[0];
		tet[1].p[1] = c.p[5];
		tet[1].p[2] = c.p[2];
		tet[1].p[3] = c.p[1];

		tet[2].p[0] = c.p[5];
		tet[2].p[1] = c.p[2];
		tet[2].p[2] = c.p[7];
		tet[2].p[3] = c.p[6];

		tet[3].p[0] = c.p[7];
		tet[3].p[1] = c.p[0];
		tet[3].p[2] = c.p[2];
		tet[3].p[3] = c.p[3];

		tet[4].p[0] = c.p[7];
		tet[4].p[1] = c.p[0];
		tet[4].p[2] = c.p[2];
		tet[4].p[3] = c.p[5];
		Ten_Triangle_3D ten_tri;
		for (int i = 0; i < 5; i++) {
			Two_Triangle_3D temp;
			temp = marchingTetrahedrons(tet[i], iso_value);
			for (int j = 0; j < temp.num; j++) {
				/*if (ratioGreaterThan20(temp.t[j])) {
						std::cout << "i = " << i << std::endl;
						printCube(c);
						printTriangle(temp.t[j]);
					}*/
				ten_tri.t[ten_tri.num] = temp.t[j];
				ten_tri.num++;
			}
		}
		return ten_tri;
	}

	// 计算两个点之间的欧几里得距离
	float distance(const Point_3D& a, const Point_3D& b) {
		return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) +
			(a.z - b.z) * (a.z - b.z));
	}

	//  细分代码实现的好像有问题？暂时先不用了
	//// 细分六面体函数
	// std::pair<Cube, Cube> subdivideCube(const Cube& c) {
	//	// 找出最长边的两个顶点对
	//	int maxEdgeIndex1 = 0, maxEdgeIndex2 = 0;
	//	float maxDistance = 0;
	//	for (int i = 0; i < 8; ++i) {
	//		for (int j = i + 1; j < 8; ++j) {
	//			float d = distance(c.p[i], c.p[j]);
	//			if (d > maxDistance) {
	//				maxDistance = d;
	//				maxEdgeIndex1 = i;
	//				maxEdgeIndex2 = j;
	//			}
	//		}
	//	}

	//	// 中间点
	//	Point_3D midpoint((c.p[maxEdgeIndex1].x + c.p[maxEdgeIndex2].x) / 2,
	//		(c.p[maxEdgeIndex1].y + c.p[maxEdgeIndex2].y) / 2,
	//		(c.p[maxEdgeIndex1].z + c.p[maxEdgeIndex2].z) / 2,
	//		(c.p[maxEdgeIndex1].value + c.p[maxEdgeIndex2].value) / 2);

	//	// 创建两个新立方体
	//	Cube c1, c2;

	//	// 按照中间点进行细分
	//	for (int i = 0; i < 8; ++i) {
	//		if (i == maxEdgeIndex1) {
	//			c1.p[i] = c.p[i];
	//			c2.p[i] = midpoint;
	//		}
	//		else if (i == maxEdgeIndex2) {
	//			c1.p[i] = midpoint;
	//			c2.p[i] = c.p[i];
	//		}
	//		else {
	//			float d1 = distance(c.p[i], c.p[maxEdgeIndex1]);
	//			float d2 = distance(c.p[i], c.p[maxEdgeIndex2]);
	//			if (d1 < d2) {
	//				c1.p[i] = c.p[i];
	//				c2.p[i] = midpoint;
	//			}
	//			else {
	//				c1.p[i] = midpoint;
	//				c2.p[i] = c.p[i];
	//			}
	//		}
	//	}

	//	return std::make_pair(c1, c2);
	//}

	// std::vector<Ten_Triangle_3D> marchingCubes_with_subdivision(Cube c, int time, float iso_value) {
	//	std::pair<Cube, Cube> subCubes = subdivideCube(c);
	//	std::vector<Ten_Triangle_3D> ten_tris;

	//	ten_tris.push_back(marchingCubes(subCubes.first, iso_value));
	//	ten_tris.push_back(marchingCubes(subCubes.second, iso_value));

	//	return ten_tris;
	//}

	std::vector<Triangle_3D> get_result(std::vector<Cube> Cubes, float iso_value) {
		std::vector<Ten_Triangle_3D> ten_tris;
		// 不做细分
		for (auto c : Cubes) { ten_tris.push_back(marchingCubes(c, iso_value)); }
		// 做细分
		// for (auto c : Cubes) {
		//	std::vector<Ten_Triangle_3D> temp = marchingCubes_with_subdivision(c, 1, iso_value);
		//	// 将temp的内容插入到ten_tris的末尾
		//	ten_tris.insert(ten_tris.end(),temp.begin(),temp.end());
		//}

		std::vector<Triangle_3D> tris;
		for (auto temp : ten_tris) {
			for (int i = 0; i < temp.num; i++) { tris.push_back(temp.t[i]); }
		}
		return tris;
	}

	// 以下代码都为debug用
	void writeTrianglesToFile(
		const std::vector<MarchingCubes::Triangle_3D>& triangles,
		const std::string& filename) {
		std::ofstream outputFile(filename);
		if (!outputFile.is_open()) {
			std::cerr << "Error opening file: " << filename << std::endl;
			return;
		}

		for (const auto& triangle : triangles) {
			for (int i = 0; i < 3; i++) {
				outputFile << "Point " << i << ": (" << triangle.p[i].x << ", "
					<< triangle.p[i].y << ", " << triangle.p[i].z << ")"
					<< std::endl;
			}
			outputFile << std::endl; // 空行分隔每个三角形
		}

		outputFile.close();
	}

	// 输出 Cube 的函数
	void printCube(const Cube& cube) {
		std::cout << "Cube:\n";
		for (int i = 0; i < 8; ++i) {
			std::cout << "Point " << i << ": (" << cube.p[i].x << ", "
				<< cube.p[i].y << ", " << cube.p[i].z
				<< ") Value: " << cube.p[i].value << "\n";
		}
	}

	void printTriangle(const Triangle_3D& triangle) {
		std::cout << "Triangle vertices:" << std::endl;
		for (int i = 0; i < 3; ++i) {
			std::cout << "Point " << i + 1 << ": (" << triangle.p[i].x << ", "
				<< triangle.p[i].y << ", " << triangle.p[i].z
				<< ") Value: " << triangle.p[i].value << std::endl;
		}
	}

	// 返回三角形的最长边长度
	float maxEdgeLength(const Triangle_3D& triangle) {
		float maxLen = 0;
		for (int i = 0; i < 3; ++i) {
			for (int j = i + 1; j < 3; ++j) {
				float len = distance(triangle.p[i], triangle.p[j]);
				if (len > maxLen) { maxLen = len; }
			}
		}
		return maxLen;
	}

	// 返回三角形的最短边长度
	float minEdgeLength(const Triangle_3D& triangle) {
		float minLen = std::numeric_limits<float>::max();
		for (int i = 0; i < 3; ++i) {
			for (int j = i + 1; j < 3; ++j) {
				float len = distance(triangle.p[i], triangle.p[j]);
				if (len < minLen) { minLen = len; }
			}
		}
		return minLen;
	}

	// 判断最长边与最短边的比例是否大于20
	bool ratioGreaterThan20(const Triangle_3D& triangle) {
		float maxLen = maxEdgeLength(triangle);
		float minLen = minEdgeLength(triangle);
		return maxLen / minLen > 20;
	}
} // namespace MarchingCubes

bool iGameMarchingCubes::Execute()
{
	// 输入数据从 inInfo 中获取，输出数据放到 outInfo
	if (!this->GetNumberOfInputs()) { return false; }
	// 获取网格数据，inInfo->GetObject(MESH_KEY()) 得到 iGameObject 对象，
	// 通过向下转换得到想要的网格对象，注意该算法针对的网格种类，获取对应
	// 的类，如果不是该类或者该类的子类，则会返回 nullptr。
	//VolumeMesh::Pointer m_Mesh = DynamicCast<VolumeMesh>(this->GetInput(0));
	VolumeMesh::Pointer m_Mesh = DynamicCast<UnstructuredMesh>(this->GetInput(0))->TransferToVolumeMesh();
	if (!m_Mesh) { return false; }
	auto cellNum = m_Mesh->GetNumberOfVolumes();
	auto Points = m_Mesh->GetPoints();
	auto PointData = m_Mesh->GetAttributeSet()->GetAllPointAttributes();
	if (!PointData->GetNumberOfElements()) { return false; }
	auto scalars = PointData->GetElement(0).pointer;

	//auto cells = iGame::iGameVolumeMesh::SafeDownCast(
	//        inInfo->GetObject(iGame::MESH_KEY()));
	//int cells_nums = iGame::iGameVolumeMesh::SafeDownCast(
	//                         inInfo->GetObject(iGame::MESH_KEY()))
	//                         ->GetNumberOfVolumes();
	//auto points_data = cells->GetPoints();
	//if (cells->GetPointData()->GetAllScalars().empty()) { return false; }
	//auto scalars = cells->GetPointData()->GetScalars(0);

	std::vector<MarchingCubes::Cube> cubes;
	igIndex i = 0, j = 0;
	igIndex vhs[IGAME_CELL_MAX_SIZE] = { 0 };
	igIndex vcnt = 0;
	for (i = 0; i < cellNum; i++) {
		MarchingCubes::Cube cube_temp;
		vcnt = m_Mesh->GetVolumePointIds(i, vhs);
		for (j = 0; j < vcnt; j++) {
			auto id = vhs[j];
			auto p_t = Points->GetPoint(id);
			cube_temp.p[j].x = p_t[0];
			cube_temp.p[j].y = p_t[1];
			cube_temp.p[j].z = p_t[2];
			cube_temp.p[j].value = scalars->GetValue(id);
			// std::cout << cube_temp.p[j].value << std::endl;
		}
		cubes.push_back(cube_temp);
	}
	// 打印一下cubes
	/*for (auto c : cubes) {
				printCube(c);
			}*/
	float iso_value;
	std::cout<<"please input iso_value\n";
	std::cin >> iso_value;
	std::vector<MarchingCubes::Triangle_3D> tris;
	tris = get_result(cubes, iso_value);
	/*for (auto t : tris) {
				printTriangle(t);
			}*/
	auto m_ResultMesh = SurfaceMesh::New();
	auto m_ResultPoints = Points::New();
	auto m_ResultTriangles = CellArray::New();
	m_ResultMesh->SetPoints(m_ResultPoints);
	m_ResultMesh->SetFaces(m_ResultTriangles);
	igIndex index = 0;
	for (auto& t : tris) {
		m_ResultPoints->AddPoint(t.p[0].x, t.p[0].y, t.p[0].z);
		m_ResultPoints->AddPoint(t.p[1].x, t.p[1].y, t.p[1].z);
		m_ResultPoints->AddPoint(t.p[2].x, t.p[2].y, t.p[2].z);
		m_ResultTriangles->AddCellId3(index++, index++, index++);
		//    igIndex p0_index = return_mesh->AddVertex(
		//            iGame::Point(t.p[0].x, t.p[0].y, t.p[0].z));
		//    igIndex p1_index = return_mesh->AddVertex(
		//            iGame::Point(t.p[1].x, t.p[1].y, t.p[1].z));
		//    igIndex p2_index = return_mesh->AddVertex(
		//            iGame::Point(t.p[2].x, t.p[2].y, t.p[2].z));
		//    /*return_mesh->AddEdge(p0_index, p1_index);
					//return_mesh->AddEdge(p1_index, p2_index);
					//return_mesh->AddEdge(p2_index, p0_index);*/
		//    igIndex points_index[3];
		//    points_index[0] = p0_index;
		//    points_index[1] = p1_index;
		//    points_index[2] = p2_index;
		//    return_mesh->AddFace(points_index, 3);
	}
	m_Outputs->SetElement(0,m_ResultMesh);
	return true;
}
IGAME_NAMESPACE_END