#pragma once

#include "iGameVolume.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameFilter.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

IGAME_NAMESPACE_BEGIN
namespace MarchingCubes
{

	// 三维中的点
	struct Point_3D {
		float x;
		float y;
		float z;
		float value;
		Point_3D() : x(0), y(0), z(0), value(0) {}
		Point_3D(float x, float y, float z, float value = 0)
			: x(x), y(y), z(z), value(value) {}
	};
	// 三维的三角形
	struct Triangle_3D {
		Point_3D p[3];
		Triangle_3D() {
			for (int i = 0; i < 3; i++) { p[i] = Point_3D(); }
		}
	};
	// 两个三角形，num表明前几个三角形是有效的；
	struct Two_Triangle_3D {
		int num;
		Triangle_3D t[2];
		Two_Triangle_3D() : num(0) {
			for (int i = 0; i < 2; i++) { t[i] = Triangle_3D(); }
		}
	};
	// 十个三角形，num表明前几个三角形是有效的；
	struct Ten_Triangle_3D {
		int num;
		Triangle_3D t[10];
		Ten_Triangle_3D() : num(0) {
			for (int i = 0; i < 10; i++) { t[i] = Triangle_3D(); }
		}
	};
	// 四面体
	struct Tetrahedron {
		Point_3D p[4];
		Tetrahedron() {
			for (int i = 0; i < 4; i++) { p[i] = Point_3D(); }
		}
	};
	// 正方体
	struct Cube {
		Point_3D p[8];
		Cube() {
			p[0] = Point_3D();
			p[1] = Point_3D();
			p[2] = Point_3D();
			p[3] = Point_3D();
			p[4] = Point_3D();
			p[5] = Point_3D();
			p[6] = Point_3D();
			p[7] = Point_3D();
		}
	};
	// 16种情况 最多出现两个三角形，六个点，每个点对应正方体两个点组成的边。
	const int table[16][12] = {
			{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 1, 0, 3, 0, 2, -1, -1, -1, -1, -1, -1},
			{1, 2, 1, 3, 1, 0, -1, -1, -1, -1, -1, -1},
			{1, 3, 0, 3, 0, 2, 1, 2, 1, 3, 0, 2},
			{2, 0, 2, 3, 2, 1, -1, -1, -1, -1, -1, -1},
			{1, 0, 0, 3, 3, 2, 3, 2, 2, 1, 1, 0},
			{2, 3, 1, 0, 2, 0, 2, 3, 1, 3, 1, 0},
			{3, 1, 3, 2, 3, 0, -1, -1, -1, -1, -1, -1},
			{3, 2, 3, 1, 3, 0, -1, -1, -1, -1, -1, -1},
			{1, 0, 2, 3, 2, 0, 1, 3, 2, 3, 1, 0},
			{0, 1, 1, 2, 2, 3, 2, 3, 3, 0, 0, 1},
			{2, 3, 2, 0, 2, 1, -1, -1, -1, -1, -1, -1},
			{0, 3, 1, 3, 0, 2, 1, 3, 1, 2, 0, 2},
			{1, 3, 1, 2, 1, 0, -1, -1, -1, -1, -1, -1},
			{0, 3, 0, 1, 0, 2, -1, -1, -1, -1, -1, -1},
			{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	};
	Point_3D VertexInterp(Point_3D p1, Point_3D p2, float iso_val);

	Two_Triangle_3D marchingTetrahedrons(Tetrahedron t, float iso_value);

	Ten_Triangle_3D marchingCubes(Cube c, float iso_value);

	std::vector<Triangle_3D> get_result(std::vector<Cube> Cubes, float iso_value);

	void writeTrianglesToFile(
		const std::vector<MarchingCubes::Triangle_3D>& triangles,
		const std::string& filename);

	// 输出 Cube 的函数
	void printCube(const Cube& cube);

	void printTriangle(const Triangle_3D& triangle);

	float distance(const Point_3D& p1, const Point_3D& p2);

	float maxEdgeLength(const Triangle_3D& triangle);

	float minEdgeLength(const Triangle_3D& triangle);

	bool ratioGreaterThan20(const Triangle_3D& triangle);

} // namespace MarchingCubes


class iGameMarchingCubes : public Filter {

public:
	I_OBJECT(iGameMarchingCubes);

	static Pointer New() { return new iGameMarchingCubes; }
	~iGameMarchingCubes() {};
	bool Execute() override;
	void SetAttribute(AttributeSet::Attribute& attr,int dimension=-1) {
		this->m_Attribute=attr;
		this->m_Scalar=attr.pointer;
		this->m_Dimension=dimension;
	}
	void SetValue(float v){this->m_Value=v;}
protected:
	iGameMarchingCubes()
	{
		SetNumberOfInputs(1);
		SetNumberOfOutputs(1);
	};
	DataObject::Pointer m_Mesh{nullptr};
	VolumeMesh::Pointer m_VolumeMesh{nullptr};
	UnstructuredMesh::Pointer m_UnstructuredMesh{nullptr};
	AttributeSet::Attribute m_Attribute{nullptr};
	ArrayObject::Pointer m_Scalar{nullptr};
	int m_Dimension=-1;
	float m_Value=0.0;
	
};
IGAME_NAMESPACE_END
