// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Shrink/TestShrink.cpp
// Regression: Apply with point attributes crashed when a copied range was empty.
// Keep scalar/vector arrays and their values after duplicating cell vertices; fix feat: integrate third-batch standard filters.
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include <Shrink/iGameShrinkFilter.h>
#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameCellType.h>
#include <iGameFlatArray.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameType.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {

double Dist(double x0, double y0, double z0, double x1, double y1, double z1) {
	double dx = x0 - x1;
	double dy = y0 - y1;
	double dz = z0 - z1;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool Check(bool ok, const std::string& name) {
	if (ok) {
		std::cout << "[PASS] " << name << std::endl;
	} else {
		std::cout << "[FAIL] " << name << std::endl;
	}
	return ok;
}

void Step(const std::string& name) { std::cout << "  >> " << name << std::endl; }

template <typename Mesh>
typename Mesh::Pointer GetIndependentOutput(iGame::ShrinkFilter::Pointer filter,
                                          typename Mesh::Pointer input, IGsize inputPointCount) {
	auto output = iGame::DynamicCast<Mesh>(filter->GetOutput());
	if (!Check(!output.IsNull(), "output has the expected mesh type")) return nullptr;
	if (!Check(output != input && output->GetPoints() != input->GetPoints(),
	           "output mesh and points are independent of input")) return nullptr;
	if (!Check(input->GetPoints()->GetNumberOfPoints() == inputPointCount,
	           "input point count remains unchanged")) return nullptr;
	return output;
}

// 手工造一个正方形网格：4 个点、2 个三角形
iGame::SurfaceMesh::Pointer MakeSquareMesh() {
	auto mesh = iGame::SurfaceMesh::New();
	mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f)); // 0
	mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f)); // 1
	mesh->AddPoint(iGame::Point(1.0f, 1.0f, 0.0f)); // 2
	mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f)); // 3

	// 面 0 = (0,1,2)，面 1 = (0,2,3)
	auto faces = iGame::CellArray::New();
	mesh->SetFaces(faces);
	igIndex tri0[3] = {0, 1, 2};
	igIndex tri1[3] = {0, 2, 3};
	faces->AddCellIds(tri0, 3);
	faces->AddCellIds(tri1, 3);
	return mesh;
}

// 场景一：表面网格，收缩比例 0.5
bool TestShrinkHalf() {
	std::cout << "== Test 1: shrink factor 0.5 (surface mesh) ==" << std::endl;

	Step("create mesh");
	auto mesh = MakeSquareMesh();

	double orig[4][3] = {
	    {0.0, 0.0, 0.0},
	    {1.0, 0.0, 0.0},
	    {1.0, 1.0, 0.0},
	    {0.0, 1.0, 0.0},
	};

	Step("run filter");
	auto filter = iGame::ShrinkFilter::New();
	filter->SetShrinkFactor(0.5);
	filter->SetInput(0, mesh);
	if (!Check(filter->Execute(), "filter Execute()")) return false;

	Step("check result");
	for (int i = 0; i < 4; ++i) {
		const auto& point = mesh->GetPoints()->GetPoint(i);
		if (!Check(Dist(point[0], point[1], point[2], orig[i][0], orig[i][1], orig[i][2]) < 1e-5,
		           "input vertex remains unchanged")) return false;
	}
	mesh = GetIndependentOutput<iGame::SurfaceMesh>(filter, mesh, 4);
	if (mesh.IsNull()) return false;
	auto pts = mesh->GetPoints();
	if (!Check(pts->GetNumberOfPoints() == 6, "each cell got its own vertices (4 -> 6 points)")) {
		return false;
	}
	if (!Check(mesh->GetNumberOfFaces() == 2, "face count unchanged (2)")) return false;

	bool ok = true;
	igIndex faceIds[2][3] = {{0, 1, 2}, {0, 2, 3}};
	for (int f = 0; f < 2; f++) {
		double cx = (orig[faceIds[f][0]][0] + orig[faceIds[f][1]][0] + orig[faceIds[f][2]][0]) / 3.0;
		double cy = (orig[faceIds[f][0]][1] + orig[faceIds[f][1]][1] + orig[faceIds[f][2]][1]) / 3.0;
		double cz = 0.0;

		igIndex newIds[3]{};
		int n = mesh->GetFacePointIds(f, newIds);
		if (!Check(n == 3, "face still has 3 vertices")) return false;

		for (int k = 0; k < 3; k++) {
			const double* op = orig[faceIds[f][k]];
			double origDist = Dist(op[0], op[1], op[2], cx, cy, cz);
			const auto& np = pts->GetPoint(newIds[k]);
			double newDist = Dist(np[0], np[1], np[2], cx, cy, cz);
			ok &= Check(std::fabs(newDist - 0.5 * origDist) < 1e-4,
			            "face " + std::to_string(f) + " vertex " + std::to_string(k) +
			                " moved halfway to centroid");
		}
	}

	igIndex f0[3]{}, f1[3]{};
	mesh->GetFacePointIds(0, f0);
	mesh->GetFacePointIds(1, f1);
	bool noShared = true;
	for (int i = 0; i < 3 && noShared; i++) {
		for (int j = 0; j < 3; j++) {
			if (f0[i] == f1[j]) {
				noShared = false;
				break;
			}
		}
	}
	ok &= Check(noShared, "the two faces no longer share vertices");
	return ok;
}

// 场景二：表面网格，收缩比例 1.0（不收缩）
bool TestNoShrink() {
	std::cout << "\n== Test 2: shrink factor 1.0 (surface mesh) ==" << std::endl;

	Step("create mesh");
	auto mesh = MakeSquareMesh();

	double orig[4][3] = {
	    {0.0, 0.0, 0.0},
	    {1.0, 0.0, 0.0},
	    {1.0, 1.0, 0.0},
	    {0.0, 1.0, 0.0},
	};

	Step("run filter");
	auto filter = iGame::ShrinkFilter::New();
	filter->SetShrinkFactor(1.0);
	filter->SetInput(0, mesh);
	if (!Check(filter->Execute(), "filter Execute()")) return false;

	Step("check result");
	mesh = GetIndependentOutput<iGame::SurfaceMesh>(filter, mesh, 4);
	if (mesh.IsNull()) return false;
	auto pts = mesh->GetPoints();
	if (!Check(pts->GetNumberOfPoints() == 6, "vertices still duplicated (6 points)")) {
		return false;
	}

	igIndex faceIds[2][3] = {{0, 1, 2}, {0, 2, 3}};
	bool ok = true;
	for (int f = 0; f < 2; f++) {
		double cx = (orig[faceIds[f][0]][0] + orig[faceIds[f][1]][0] + orig[faceIds[f][2]][0]) / 3.0;
		double cy = (orig[faceIds[f][0]][1] + orig[faceIds[f][1]][1] + orig[faceIds[f][2]][1]) / 3.0;
		double cz = 0.0;

		igIndex newIds[3]{};
		mesh->GetFacePointIds(f, newIds);
		for (int k = 0; k < 3; k++) {
			const double* op = orig[faceIds[f][k]];
			double origDist = Dist(op[0], op[1], op[2], cx, cy, cz);
			const auto& np = pts->GetPoint(newIds[k]);
			double newDist = Dist(np[0], np[1], np[2], cx, cy, cz);
			ok &= Check(std::fabs(newDist - origDist) < 1e-4,
			            "face " + std::to_string(f) + " vertex " + std::to_string(k) +
			                " stays at original position");
		}
	}
	return ok;
}

// 场景三：体网格
bool TestVolumeMesh() {
	std::cout << "\n== Test 3: volume mesh ==" << std::endl;

	Step("create volume mesh");
	auto mesh = iGame::VolumeMesh::New();
	mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f)); // 0
	mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f)); // 1
	mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f)); // 2
	mesh->AddPoint(iGame::Point(0.0f, 0.0f, 1.0f)); // 3
	mesh->AddPoint(iGame::Point(1.0f, 1.0f, 1.0f)); // 4

	igIndex tet0[4] = {0, 1, 2, 3};
	igIndex tet1[4] = {1, 2, 3, 4};
	auto volumes = iGame::CellArray::New();
	mesh->SetVolumes(volumes);
	volumes->AddCellIds(tet0, 4);
	volumes->AddCellIds(tet1, 4);

	double orig[5][3] = {
	    {0.0, 0.0, 0.0},
	    {1.0, 0.0, 0.0},
	    {0.0, 1.0, 0.0},
	    {0.0, 0.0, 1.0},
	    {1.0, 1.0, 1.0},
	};

	Step("run filter");
	auto filter = iGame::ShrinkFilter::New();
	filter->SetShrinkFactor(0.5);
	filter->SetInput(0, mesh);
	if (!Check(filter->Execute(), "filter Execute()")) return false;

	Step("check result");
	mesh = GetIndependentOutput<iGame::VolumeMesh>(filter, mesh, 5);
	if (mesh.IsNull()) return false;
	auto pts = mesh->GetPoints();
	if (!Check(pts->GetNumberOfPoints() == 8, "each volume got its own vertices (5 -> 8 points)")) {
		return false;
	}
	if (!Check(mesh->GetNumberOfVolumes() == 2, "volume count unchanged (2)")) return false;

	bool ok = true;
	igIndex volIds[2][4] = {{0, 1, 2, 3}, {1, 2, 3, 4}};
	for (int v = 0; v < 2; v++) {
		double cx = 0.0, cy = 0.0, cz = 0.0;
		for (int k = 0; k < 4; k++) {
			cx += orig[volIds[v][k]][0];
			cy += orig[volIds[v][k]][1];
			cz += orig[volIds[v][k]][2];
		}
		cx /= 4.0;
		cy /= 4.0;
		cz /= 4.0;

		igIndex newIds[4]{};
		int n = mesh->GetVolumePointIds(v, newIds);
		if (!Check(n == 4, "volume still has 4 vertices")) return false;

		for (int k = 0; k < 4; k++) {
			const double* op = orig[volIds[v][k]];
			double origDist = Dist(op[0], op[1], op[2], cx, cy, cz);
			const auto& np = pts->GetPoint(newIds[k]);
			double newDist = Dist(np[0], np[1], np[2], cx, cy, cz);
			ok &= Check(std::fabs(newDist - 0.5 * origDist) < 1e-4,
			            "volume " + std::to_string(v) + " vertex " + std::to_string(k) +
			                " moved halfway to centroid");
		}
	}

	igIndex v0[4]{}, v1[4]{};
	mesh->GetVolumePointIds(0, v0);
	mesh->GetVolumePointIds(1, v1);
	bool noShared = true;
	for (int i = 0; i < 4 && noShared; i++) {
		for (int j = 0; j < 4; j++) {
			if (v0[i] == v1[j]) {
				noShared = false;
				break;
			}
		}
	}
	ok &= Check(noShared, "the two volumes no longer share vertices");
	return ok;
}

// 场景四：非结构化网格
bool TestUnstructuredMesh() {
	std::cout << "\n== Test 4: unstructured mesh ==" << std::endl;

	Step("create unstructured mesh");
	auto mesh = iGame::UnstructuredMesh::New();
	mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f)); // 0
	mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f)); // 1
	mesh->AddPoint(iGame::Point(1.0f, 1.0f, 0.0f)); // 2
	mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f)); // 3

	auto cells = iGame::CellArray::New();
	auto types = iGame::UnsignedIntArray::New();
	mesh->SetCells(cells, types);
	igIndex tri0[3] = {0, 1, 2};
	igIndex tri1[3] = {0, 2, 3};
	mesh->AddCell(tri0, 3, iGame::IG_TRIANGLE);
	mesh->AddCell(tri1, 3, iGame::IG_TRIANGLE);

	double orig[4][3] = {
	    {0.0, 0.0, 0.0},
	    {1.0, 0.0, 0.0},
	    {1.0, 1.0, 0.0},
	    {0.0, 1.0, 0.0},
	};

	Step("run filter");
	auto filter = iGame::ShrinkFilter::New();
	filter->SetShrinkFactor(0.5);
	filter->SetInput(0, mesh);
	if (!Check(filter->Execute(), "filter Execute()")) return false;

	Step("check result");
	mesh = GetIndependentOutput<iGame::UnstructuredMesh>(filter, mesh, 4);
	if (mesh.IsNull()) return false;
	auto pts = mesh->GetPoints();
	if (!Check(pts->GetNumberOfPoints() == 6, "each cell got its own vertices (4 -> 6 points)")) {
		return false;
	}
	if (!Check(mesh->GetNumberOfCells() == 2, "cell count unchanged (2)")) return false;

	bool ok = true;
	igIndex cellIds[2][3] = {{0, 1, 2}, {0, 2, 3}};
	for (int c = 0; c < 2; c++) {
		double cx = (orig[cellIds[c][0]][0] + orig[cellIds[c][1]][0] + orig[cellIds[c][2]][0]) / 3.0;
		double cy = (orig[cellIds[c][0]][1] + orig[cellIds[c][1]][1] + orig[cellIds[c][2]][1]) / 3.0;
		double cz = 0.0;

		igIndex newIds[3]{};
		int n = mesh->GetCellPointIds(c, newIds);
		if (!Check(n == 3, "cell still has 3 vertices")) return false;

		for (int k = 0; k < 3; k++) {
			const double* op = orig[cellIds[c][k]];
			double origDist = Dist(op[0], op[1], op[2], cx, cy, cz);
			const auto& np = pts->GetPoint(newIds[k]);
			double newDist = Dist(np[0], np[1], np[2], cx, cy, cz);
			ok &= Check(std::fabs(newDist - 0.5 * origDist) < 1e-4,
			            "cell " + std::to_string(c) + " vertex " + std::to_string(k) +
			                " moved halfway to centroid");
		}
	}
	return ok;
}

// 场景五：三分量点属性在收缩后长度与值正确
bool TestPointAttribute3Component() {
	std::cout << "\n== Test 5: 3-component point attribute ==" << std::endl;

	Step("create mesh with 3-component point attribute");
	auto mesh = MakeSquareMesh();

	auto vec = iGame::FloatArray::New();
	vec->SetName("Velocity");
	vec->SetDimension(3);
	vec->Resize(4);  // 4 个元素，每个 3 个分量 = 12 个原始值
	for (IGsize i = 0; i < 12; i++) {
		vec->SetValue(i, static_cast<double>(i + 1));
	}
	mesh->GetAttributeSet()->AddVector(IG_POINT, vec);

	Step("run filter");
	auto filter = iGame::ShrinkFilter::New();
	filter->SetShrinkFactor(0.5);
	filter->SetInput(0, mesh);
	if (!Check(filter->Execute(), "filter Execute()")) return false;

	Step("check result");
	if (!Check(vec->GetNumberOfElements() == 4 && vec->GetNumberOfValues() == 12,
	           "input Velocity array size remains unchanged")) return false;
	for (IGsize i = 0; i < 12; ++i) {
		if (!Check(std::fabs(vec->GetValue(i) - static_cast<double>(i + 1)) < 1e-5,
		           "input Velocity value remains unchanged")) return false;
	}
	mesh = GetIndependentOutput<iGame::SurfaceMesh>(filter, mesh, 4);
	if (mesh.IsNull()) return false;
	auto attrs = mesh->GetAttributeSet();
	int idx = attrs->GetAttributeIndex("Velocity");
	if (!Check(idx >= 0, "Velocity array exists")) return false;
	auto arr = iGame::DynamicCast<iGame::FloatArray>(attrs->GetAttribute(idx).pointer);
	if (!Check(!arr.IsNull(), "Velocity is a FloatArray")) return false;
	if (!Check(arr->GetNumberOfElements() == 6, "Velocity has 6 elements (4 -> 6 points)")) return false;
	if (!Check(arr->GetNumberOfValues() == 18, "Velocity has 18 raw values (6 x 3)")) return false;

	bool ok = true;
	// 新点 0、1、2 来自旧点 0、1、2；新点 3、4、5 来自旧点 0、2、3
	// 旧点 0 的三个分量为 1、2、3
	ok &= Check(std::fabs(arr->GetElementValue(0, 0) - 1.0) < 1e-5, "new point 0 dim0 = 1");
	ok &= Check(std::fabs(arr->GetElementValue(0, 1) - 2.0) < 1e-5, "new point 0 dim1 = 2");
	ok &= Check(std::fabs(arr->GetElementValue(0, 2) - 3.0) < 1e-5, "new point 0 dim2 = 3");
	// 新点 3 是旧点 0 的副本，分量同样为 1、2、3
	ok &= Check(std::fabs(arr->GetElementValue(3, 0) - 1.0) < 1e-5, "new point 3 dim0 = 1 (copy of point 0)");
	// 新点 5 来自旧点 3，旧点 3 的第 0 个分量 = 3*3+1 = 10
	ok &= Check(std::fabs(arr->GetElementValue(5, 0) - 10.0) < 1e-5, "new point 5 dim0 = 10 (from point 3)");
	return ok;
}

}

int main() {
	bool ok = true;
	ok &= TestShrinkHalf();
	ok &= TestNoShrink();
	ok &= TestVolumeMesh();
	ok &= TestUnstructuredMesh();
	ok &= TestPointAttribute3Component();

	if (ok) {
		std::cout << "\nALL TESTS PASSED" << std::endl;
		return 0;
	}
	std::cout << "\nSOME TESTS FAILED" << std::endl;
	return 1;
}
