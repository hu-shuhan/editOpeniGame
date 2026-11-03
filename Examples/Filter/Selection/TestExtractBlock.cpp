// 迁移回归（待提交）：防止标准过滤器仅有菜单占位，验证本文件记录的算法结果、输入保持不变及边界情况。
#include "Selection/iGameExtractBlockFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameCompositeDataObject.h"
#include "iGameStructuredMesh.h"
#include "iGameVisibleBlockTraversal.h"
#include <iostream>
#include <stdexcept>

using namespace iGame;
class TestComposite : public CompositeDataObject {
public:
    I_OBJECT(TestComposite);
    static Pointer New() { return new TestComposite; }
    TestComposite() = default;
    std::vector<DataObject::Pointer> children;
    unsigned int GetNumberOfChildren() override { return static_cast<unsigned int>(children.size()); }
    DataObject* GetChild(int index) override { return children.at(index).get(); }
};
static void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
static UnstructuredMesh::Pointer Mesh(const char* name, double offset) {
    auto mesh = UnstructuredMesh::New();
    mesh->SetName(name);
    mesh->GetPoints()->AddPoint(Point(offset, 0, 0));
    mesh->GetPoints()->AddPoint(Point(offset + 1, 0, 0));
    mesh->GetPoints()->AddPoint(Point(offset, 1, 0));
    igIndex ids[]{0, 1, 2};
    mesh->AddCell(ids, 3, IG_TRIANGLE);
    auto values = LongLongArray::New();
    values->SetName("ids");
    for (int i = 0; i < 3; ++i) values->AddValue(9007199254740993LL + i);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, values);
    auto vectors = FloatArray::New();
    vectors->SetName("velocity");
    vectors->SetDimension(3);
    for (int i = 0; i < 3; ++i) vectors->AddElement3(i, i + 1, i + 2);
    mesh->GetAttributeSet()->AddAttribute(IG_VECTOR, IG_POINT, vectors);
    auto material = IntArray::New();
    material->SetName("material");
    material->AddValue(7);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, material);
    return mesh;
}
int main() {
    try {
        auto filter = ExtractBlockFilter::New();
        Check(!filter->Execute() && !filter->GetOutput(), "null input");
        auto root = DrawObject::New();
        auto first = Mesh("first", 0);
        auto second = Mesh("second", 3);
        root->AddSubDataObject(first);
        root->AddSubDataObject(second);
        filter->SetInput(root);
        filter->SetBlockIndex(1);
        Check(filter->Execute(), "index extraction");
        auto out = DynamicCast<UnstructuredMesh>(filter->GetOutput());
        Check(out && out->GetNumberOfCells() == 1 && out->GetNumberOfPoints() == 3, "counts");
        Check(out->GetCellType(0) == IG_TRIANGLE && out->GetPoint(0)[0] == 3, "geometry");
        Check(out->GetPoints() != second->GetPoints() && out->GetCells() != second->GetCells(), "deep copy");
        auto ids = DynamicCast<LongLongArray>(out->GetAttributeSet()->GetAttribute(0).pointer);
        Check(ids && ids->RawPointer()[0] == 9007199254740993LL, "integer precision");
        Check(out->GetAttributeSet()->GetNumberOfAttributes() == 3, "all numeric attributes copied");
        auto vectors = DynamicCast<FloatArray>(out->GetAttributeSet()->GetAttribute(1).pointer);
        Check(vectors && vectors->GetDimension() == 3 && vectors->GetValue(8) == 4, "vector values and type");
        auto material = DynamicCast<IntArray>(out->GetAttributeSet()->GetAttribute(2).pointer);
        Check(material && material->RawPointer()[0] == 7, "cell values and type");
        material->SetValue(0, 99);
        Check(second->GetAttributeSet()->GetAttribute(2).pointer->GetValue(0) == 7,
              "independent cell attributes");
        ids->SetValue(0, 42);
        Check(DynamicCast<LongLongArray>(second->GetAttributeSet()->GetAttribute(0).pointer)->RawPointer()[0]
                  == 9007199254740993LL, "independent attributes");
        Check(ExtractBlockFilter::GetBlocks(root).size() == 2, "input hierarchy unchanged");
        filter->SetBlockName("first");
        Check(filter->Execute(), "name extraction");
        filter->SetBlockName("missing");
        Check(!filter->Execute() && !filter->GetOutput(), "missing name clears output");
        filter->SetBlockName("");
        Check(!filter->Execute(), "empty name");
        filter->SetBlockIndex(-1);
        Check(!filter->Execute(), "negative index");
        filter->SetBlockIndex(2);
        Check(!filter->Execute(), "out of range");
        filter->SetBlockPath({});
        Check(!filter->Execute(), "empty path");
        auto nested = DrawObject::New();
        nested->SetName("nested");
        nested->AddSubDataObject(Mesh("third", 6));
        root->AddSubDataObject(nested);
        filter->SetBlockPath({2, 0});
        Check(filter->Execute() && filter->GetOutput()->GetName() == "third_extracted_block", "nested path");
        filter->SetBlockName("third");
        Check(filter->Execute(), "recursive name lookup");
        filter->SetBlockIndex(2);
        Check(filter->Execute() && ExtractBlockFilter::GetBlocks(filter->GetOutput()).size() == 1, "subtree extraction");
        second->SetName("first");
        filter->SetBlockName("first");
        Check(!filter->Execute(), "ambiguous name");
        filter->SetInput(first);
        filter->SetBlockIndex(0);
        Check(!filter->Execute(), "single mesh is not multiblock");
        auto slots = TestComposite::New();
        slots->children = {nullptr, first};
        filter->SetInput(slots);
        Check(!filter->Execute(), "empty block slot");
        filter->SetBlockIndex(1);
        Check(filter->Execute(), "composite child interface");
        slots->children = {slots};
        filter->SetBlockIndex(0);
        Check(!filter->Execute() && !filter->GetOutput(), "cyclic subtree rejected");
        filter->SetBlockName("missing");
        Check(!filter->Execute(), "cyclic name traversal rejected");
        slots->children.clear();
        auto broken = Mesh("broken", 0);
        broken->GetAttributeSet()->GetAttribute(0).pointer->Resize(1);
        auto brokenRoot = DrawObject::New();
        brokenRoot->AddSubDataObject(broken);
        filter->SetInput(brokenRoot);
        filter->SetBlockIndex(0);
        Check(!filter->Execute() && !filter->GetOutput(), "short attribute rejected");
        for (igIndex depth : {igIndex{1}, igIndex{2}}) {
            auto structured = StructuredMesh::New();
            igIndex dimensions[]{2, 2, depth};
            structured->SetDimensionSize(dimensions);
            for (igIndex z = 0; z < depth; ++z)
                for (igIndex y = 0; y < 2; ++y)
                    for (igIndex x = 0; x < 2; ++x)
                        structured->GetPoints()->AddPoint(Point(x, y, z));
            structured->GenStructuredCellConnectivities();
            auto material = IntArray::New();
            material->SetName("material");
            material->AddValue(9);
            structured->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, material);
            auto container = TestComposite::New();
            container->children = {structured};
            filter->SetInput(container);
            filter->SetBlockIndex(0);
            Check(filter->Execute(), "structured extraction");
            auto result = DynamicCast<StructuredMesh>(filter->GetOutput());
            Check(result && result->GetDimensionSize()[2] == depth && result->GetNumberOfCells() == 1,
                  "structured dimensions and topology preserved");
            Check(result->GetPoints() != structured->GetPoints() &&
                  result->GetAttributeSet()->GetAttribute(0).pointer->GetValue(0) == 9,
                  "structured geometry and cell attributes copied");
        }
        auto visibleRoot = DrawObject::New();
        auto visibleGroup = DrawObject::New();
        visibleRoot->AddSubDataObject(Mesh("surface", 0));
        visibleGroup->AddSubDataObject(Mesh("nested", 2));
        visibleRoot->AddSubDataObject(visibleGroup);
        int count = 0;
        RenderingDetail::ForEachVisibleLeaf(visibleRoot, [&](DrawObject::Pointer) { ++count; });
        Check(count == 2, "nested rendering visits all visible leaves");
        visibleGroup->SetVisibility(false);
        count = 0;
        RenderingDetail::ForEachVisibleLeaf(visibleRoot, [&](DrawObject::Pointer) { ++count; });
        Check(count == 1, "hidden group suppresses descendants");
        std::cout << "ExtractBlock: all regression checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ExtractBlock FAILED: " << e.what() << '\n';
        return 1;
    }
}
