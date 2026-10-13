// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/Subdivide/TestSubdivideFilter.cpp
// 集成回归（待提交）：迁移标准过滤器时，防止仅复制菜单而遗漏算法或测试资产。
// 保留源仓库测试的几何、属性与边界断言；交互示例仅用于人工检查。
#include <Subdivide/iGameSubdivide.h>

#include <iGameArrayObject.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameType.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// SubdivideFilter 测试程序
//   - 默认以相对路径加载 Models/subdivide_test.vtk（工作目录为 Examples，构建时 Models 会拷贝到输出目录）；
//   - 执行 1 级线性细分；
//   - 输出输出网格的点/面数量，以及所有点的坐标位置。
// 用法：
//   TestSubdivideFilter.exe                         （使用默认相对路径模型）
//   TestSubdivideFilter.exe Models/subdivide_test.vtk（显式指定模型路径）
int main(int argc, char* argv[]) {
    std::string fileName = "././Models/subdivide_test.vtk";
    if (argc > 1 && argv[1] != nullptr) {
        fileName = argv[1];
    }

    std::cout << "读取模型: " << fileName << std::endl;
    auto input = iGame::FileIO::ReadFile(fileName);
    if (input == nullptr) {
        std::cout << "读取模型失败。" << std::endl;
        std::cout << "用法: TestSubdivideFilter.exe Models/subdivide_test.vtk" << std::endl;
        return 1;
    }

    auto inputMesh = iGame::DynamicCast<iGame::SurfaceMesh>(input);
    if (inputMesh == nullptr) {
        std::cout << "输入数据不是 SurfaceMesh；SubdivideFilter 仅支持多边形表面网格（Poly Data）。"
                  << std::endl;
        return 1;
    }

    std::cout << "输入: 点数 " << inputMesh->GetNumberOfPoints()
              << ", 面数 " << inputMesh->GetNumberOfFaces() << std::endl;

    // 执行 1 级细分
    auto filter = iGame::SubdivideFilter::New();
    filter->SetNumberOfSubdivisions(1);
    filter->SetInput(inputMesh);
    if (!filter->Execute()) {
        std::cout << "SubdivideFilter 执行失败。" << std::endl;
        return 1;
    }

    auto output = iGame::DynamicCast<iGame::SurfaceMesh>(filter->GetOutput(0));
    if (output == nullptr) {
        std::cout << "Filter 没有生成有效的 SurfaceMesh 输出。" << std::endl;
        return 1;
    }

    auto pts = output->GetPoints();
    const IGsize numPts = pts->GetNumberOfPoints();

    std::cout << "\n========== Subdivide（1 级细分） ==========" << std::endl;
    std::cout << "输出: 点数 " << numPts
              << ", 面数 " << output->GetNumberOfFaces() << std::endl;

    // 打印 PointData / CellData 清单，验证细分后属性不丢失
    auto dumpAttributes = [](const char* tag, auto attrs) {
        std::cout << "\n---------- " << tag << " ----------" << std::endl;
        if (attrs == nullptr) { std::cout << "(无)" << std::endl; return; }
        std::cout << "属性数: " << attrs->GetNumberOfElements() << std::endl;
        for (int i = 0; i < attrs->GetNumberOfElements(); ++i) {
            auto& a = attrs->GetElement(i);
            iGame::ArrayObject::Pointer data = a.GetPointer();
            if (data == nullptr) continue;
            const int dim = data->GetDimension();
            std::cout << "[" << data->GetName() << "] dim=" << dim
                      << "，元组数=" << data->GetNumberOfElements() << "，前 8 个元组:";
            const IGsize showN = std::min<IGsize>(data->GetNumberOfElements(), IGsize(8));
            std::vector<double> buf(dim);
            for (IGsize k = 0; k < showN; ++k) {
                data->GetElement(k, buf.data());
                std::cout << " (";
                for (int d = 0; d < dim; ++d) {
                    if (d) std::cout << ",";
                    std::cout << buf[d];
                }
                std::cout << ")";
            }
            std::cout << std::endl;
        }
    };
    auto outAttrSet = output->GetAttributeSet();
    dumpAttributes("PointData", outAttrSet->GetAllPointAttributes());
    dumpAttributes("CellData", outAttrSet->GetAllCellAttributes());

    std::cout << "\n---------- 所有点位置 ----------" << std::endl;
    std::cout << std::fixed << std::setprecision(6);
    for (IGsize i = 0; i < numPts; ++i) {
        const auto& p = pts->GetPoint(i);
        std::cout << "Point " << i << ": ("
                  << p[0] << ", " << p[1] << ", " << p[2] << ")\n";
    }

    std::cout << "\n共输出 " << numPts << " 个点。" << std::endl;
    return 0;
}
