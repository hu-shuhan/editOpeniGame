// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Shrink/TestShrinkModel.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include <Shrink/iGameShrinkFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGameSurfaceMesh.h>
#include <iGameType.h>

#include <fstream>
#include <iostream>
#include <string>

namespace {

bool FileExists(const std::string& path) {
	std::ifstream f(path);
	return f.good();
}

// 依次尝试常见运行目录，找到模型后返回完整相对路径
std::string FindModel(const std::string& name) {
	const std::string candidates[] = {
	    "./Examples/Models/" + name,
	    "./Models/" + name,
	    "../Examples/Models/" + name,
	};
	for (const auto& c : candidates) {
		if (FileExists(c)) return c;
	}
	return "";
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

// 场景一：读入三角面片立方体，收缩 0.5 后 8 个顶点应变为 12x3=36 个
bool TestCube() {
	std::cout << "== Test 1: cube model (Shrink_Cube.vtk) ==" << std::endl;

	Step("find model");
	std::string path = FindModel("Shrink_Cube.vtk");
	if (path.empty()) {
		std::cout << "MODEL NOT FOUND: Shrink_Cube.vtk" << std::endl;
		return false;
	}

	Step("read " + path);
	auto obj = iGame::FileIO::ReadFile(path);
	if (obj.IsNull()) {
		std::cout << "READ FAILED" << std::endl;
		return false;
	}
	auto ps = iGame::DynamicCast<iGame::PointSet>(obj);
	if (ps.IsNull()) {
		std::cout << "NOT A POINT SET" << std::endl;
		return false;
	}

	Step("run shrink factor 0.5");
	auto filter = iGame::ShrinkFilter::New();
	filter->SetShrinkFactor(0.5);
	filter->SetInput(0, obj);
	if (!Check(filter->Execute(), "filter Execute()")) return false;

	Step("check result");
	auto output = filter->GetOutput();
	auto outputPoints = iGame::DynamicCast<iGame::PointSet>(output);
	if (!Check(!outputPoints.IsNull(), "output is a point set")) return false;
	if (!Check(output != obj && outputPoints->GetPoints() != ps->GetPoints(),
	           "output is independent of input")) return false;
	if (!Check(ps->GetPoints()->GetNumberOfPoints() == 8, "input cube retains 8 points")) return false;
	IGsize points = outputPoints->GetPoints()->GetNumberOfPoints();
	if (!Check(points == 36, "each triangle got its own vertices (8 -> 36 points)")) return false;

	auto mesh = iGame::DynamicCast<iGame::SurfaceMesh>(output);
	if (!Check(!mesh.IsNull() && mesh->GetNumberOfFaces() == 12, "face count unchanged (12)")) return false;
	return true;
}

// 场景二：读入两个四面体（带 Pressure 点标量），收缩 0.5 后顶点 5 -> 8，且标量被正确复制
bool TestTwoTets() {
	std::cout << "\n== Test 2: two-tetrahedra model (Shrink_TwoTets.vtk) ==" << std::endl;

	Step("find model");
	std::string path = FindModel("Shrink_TwoTets.vtk");
	if (path.empty()) {
		std::cout << "MODEL NOT FOUND: Shrink_TwoTets.vtk" << std::endl;
		return false;
	}

	Step("read " + path);
	auto obj = iGame::FileIO::ReadFile(path);
	if (obj.IsNull()) {
		std::cout << "READ FAILED" << std::endl;
		return false;
	}
	auto ps = iGame::DynamicCast<iGame::PointSet>(obj);
	if (ps.IsNull()) {
		std::cout << "NOT A POINT SET" << std::endl;
		return false;
	}

	Step("run shrink factor 0.5");
	auto filter = iGame::ShrinkFilter::New();
	filter->SetShrinkFactor(0.5);
	filter->SetInput(0, obj);
	if (!Check(filter->Execute(), "filter Execute()")) return false;

	Step("check result");
	auto output = filter->GetOutput();
	auto outputPoints = iGame::DynamicCast<iGame::PointSet>(output);
	if (!Check(!outputPoints.IsNull(), "output is a point set")) return false;
	if (!Check(output != obj && outputPoints->GetPoints() != ps->GetPoints(),
	           "output is independent of input")) return false;
	if (!Check(ps->GetPoints()->GetNumberOfPoints() == 5, "input tetrahedra retain 5 points")) return false;
	auto inputAttrs = obj->GetAttributeSet();
	int inputIdx = inputAttrs->GetAttributeIndex("Pressure");
	if (!Check(inputIdx >= 0 && inputAttrs->GetAttribute(inputIdx).pointer->GetNumberOfElements() == 5,
	           "input Pressure retains 5 elements")) return false;
	IGsize points = outputPoints->GetPoints()->GetNumberOfPoints();
	if (!Check(points == 8, "each tetra got its own vertices (5 -> 8 points)")) return false;

	// 点标量 Pressure 应随顶点复制，元素数从 5 变为 8
	auto attrs = output->GetAttributeSet();
	int idx = attrs->GetAttributeIndex("Pressure");
	if (!Check(idx >= 0, "Pressure array exists")) return false;
	auto arr = iGame::DynamicCast<iGame::FloatArray>(attrs->GetAttribute(idx).pointer);
	if (!Check(!arr.IsNull(), "Pressure is a FloatArray")) return false;
	if (!Check(arr->GetNumberOfElements() == 8, "Pressure copied to all new points (5 -> 8)")) return false;
	return true;
}

}  // namespace

int main() {
	bool ok = true;
	ok &= TestCube();
	ok &= TestTwoTets();

	if (ok) {
		std::cout << "\nALL TESTS PASSED" << std::endl;
		return 0;
	}
	std::cout << "\nSOME TESTS FAILED" << std::endl;
	return 1;
}
