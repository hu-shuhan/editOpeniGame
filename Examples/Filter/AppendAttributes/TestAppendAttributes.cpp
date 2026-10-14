// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/AppendAttributes/TestAppendAttributes.cpp
// 集成回归（待提交）：迁移标准过滤器时，防止仅复制菜单而遗漏算法或测试资产。
// 保留源仓库测试的几何、属性与边界断言；交互示例仅用于人工检查。
#include <AppendAttributes/iGameAppendAttributesFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameSmartPointer.h>
#include <iGameVector.h>
#include <string>
#include <vector>

int main() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/append_grid_a.vtk";
    const std::string fileName2 = "./Models/append_grid_b.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (!obj) {
        std::cout << "Failed to read file: " << fileName << std::endl;
        std::cin.get();
        return 1;
    }
    std::cout << "Before modified:" << std::endl;
    auto attrSet = obj->GetAttributeSet();
    std::cout<<"append_grid_a.vtk:"<<std::endl;
    if (attrSet) {
        // 输出点属性名称
        auto pointAttrs = attrSet->GetAllPointAttributes();
        if (pointAttrs) {
            std::cout << "\nPoint AttributSet:\n";
            for (IGsize i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
                auto& attr = pointAttrs->GetElement(i);
                if (attr.IsNone()) continue;
                // 输出属性名、元组数、分量数
                std::cout << "  " << attr.pointer->GetName() << " (tuples=" << attr.pointer->GetNumberOfElements()
                          << ", comps=" << attr.pointer->GetDimension() << ")" << std::endl;
            }
        }

        // 输出单元属性名称
        auto cellAttrs = attrSet->GetAllCellAttributes();
        if (cellAttrs) {
            std::cout << "\nCell AttributeSet\n";
            for (IGsize i = 0; i < cellAttrs->GetNumberOfElements(); ++i) {
                auto& attr = cellAttrs->GetElement(i);
                if (attr.IsNone()) continue;
                std::cout << "  " << attr.pointer->GetName() << " (tuples=" << attr.pointer->GetNumberOfElements()
                          << ", comps=" << attr.pointer->GetDimension() << ")" << std::endl;
            }
        }
    } else {
        std::cout << "AttributeSet is null.\n";
    }
    attrSet = obj->GetAttributeSet();
    std::cout<<"append_grid_b.vtk:"<<std::endl;
    if (attrSet) {
        // 输出点属性名称
        auto pointAttrs = attrSet->GetAllPointAttributes();
        if (pointAttrs) {
            std::cout << "\nPoint AttributSet:\n";
            for (IGsize i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
                auto& attr = pointAttrs->GetElement(i);
                if (attr.IsNone()) continue;
                // 输出属性名、元组数、分量数
                std::cout << "  " << attr.pointer->GetName() << " (tuples=" << attr.pointer->GetNumberOfElements()
                          << ", comps=" << attr.pointer->GetDimension() << ")" << std::endl;
            }
        }

        // 输出单元属性名称
        auto cellAttrs = attrSet->GetAllCellAttributes();
        if (cellAttrs) {
            std::cout << "\nCell AttributeSet\n";
            for (IGsize i = 0; i < cellAttrs->GetNumberOfElements(); ++i) {
                auto& attr = cellAttrs->GetElement(i);
                if (attr.IsNone()) continue;
                std::cout << "  " << attr.pointer->GetName() << " (tuples=" << attr.pointer->GetNumberOfElements()
                          << ", comps=" << attr.pointer->GetDimension() << ")" << std::endl;
            }
        }
    } else {
        std::cout << "AttributeSet is null.\n";
    }


    auto filter = iGame::AppendAttributes::New();
    filter->AddInput(obj);
    obj=iGame::FileIO::ReadFile(fileName2);
    filter->AddInput(obj);
    filter->SetAppendPointData(true);
    filter->SetAppendCellData(true);
    if (!filter->Execute()) {
        std::cout << "Filter ERROR!\n";
        std::cin.get();
        return 0;
    }
    obj = filter->GetOutput();
    scene->AddModel(obj);
    std::cout << "After modified:" << std::endl;
    attrSet = obj->GetAttributeSet();
    if (attrSet) {
        // 输出点属性名称
        auto pointAttrs = attrSet->GetAllPointAttributes();
        if (pointAttrs) {
            std::cout << "\nPoint AttributSet:\n";
            for (IGsize i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
                auto& attr = pointAttrs->GetElement(i);
                if (attr.IsNone()) continue;
                // 输出属性名、元组数、分量数
                std::cout << "  " << attr.pointer->GetName() << " (tuples=" << attr.pointer->GetNumberOfElements()
                          << ", comps=" << attr.pointer->GetDimension() << ")" << std::endl;
            }
        }

        // 输出单元属性名称
        auto cellAttrs = attrSet->GetAllCellAttributes();
        if (cellAttrs) {
            std::cout << "\nCell AttributeSet\n";
            for (IGsize i = 0; i < cellAttrs->GetNumberOfElements(); ++i) {
                auto& attr = cellAttrs->GetElement(i);
                if (attr.IsNone()) continue;
                std::cout << "  " << attr.pointer->GetName() << " (tuples=" << attr.pointer->GetNumberOfElements()
                          << ", comps=" << attr.pointer->GetDimension() << ")" << std::endl;
            }
        }
    } else {
        std::cout << "AttributeSet is null.\n";
    }
    std::cin.get();
}