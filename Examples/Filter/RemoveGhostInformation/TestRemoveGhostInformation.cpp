// Source: dayuwan77/igamevis at eccac729b57aeacbe9312d7d5189f6990bb4eebd.
// Integration gap: e7ec6571 imported the filter but omitted its example.
// Preserve the numerical/attribute checks below and reject invalid inputs;
// visual examples support --no-render so CI requires a real exit status.
// Integration commit: test: add examples for first-batch standard filters
// Find it: git log --diff-filter=A --format="%h %s" -- Examples/Filter/RemoveGhostInformation/TestRemoveGhostInformation.cpp
#include <RemoveGhostInformation/iGameRemoveGhostInformationFilter.h>

#include <iGameAttributeSet.h>
#include <iGameFlatArray.h>
#include <iGamePoints.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <string>


namespace
{

// ============================================================
// 基础辅助函数
// ============================================================

bool IsGhostAttributeName(const std::string& inputName) {
    std::string name = inputName;

    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return name == "vtkghosttype";
}


bool AlmostEqual(double a, double b, double eps = 1e-6) { return std::fabs(a - b) < eps; }


// ============================================================
// 统计Ghost Attribute
// ============================================================

IGsize CountGhostAttributes(iGame::DataObject::Pointer dataObject) {
    if (dataObject.IsNull()) { return 0; }

    auto attributeSet = dataObject->GetAttributeSet();

    if (attributeSet == nullptr) { return 0; }

    IGsize count = 0;

    const IGsize numberOfAttributes = static_cast<IGsize>(attributeSet->GetNumberOfAttributes());

    for (IGsize i = 0; i < numberOfAttributes; ++i) {
        auto& attr = attributeSet->GetAttribute(i);

        if (attr.isDeleted || attr.pointer == nullptr) { continue; }

        if (IsGhostAttributeName(attr.pointer->GetName())) { ++count; }
    }

    return count;
}


// ============================================================
// 根据名字取得Attribute Array
// ============================================================

iGame::ArrayObject::Pointer GetAttributeArrayByName(iGame::DataObject::Pointer dataObject, const std::string& name) {
    if (dataObject.IsNull()) { return nullptr; }

    auto attributeSet = dataObject->GetAttributeSet();

    if (attributeSet == nullptr) { return nullptr; }

    const IGsize numberOfAttributes = static_cast<IGsize>(attributeSet->GetNumberOfAttributes());

    for (IGsize i = 0; i < numberOfAttributes; ++i) {
        auto& attr = attributeSet->GetAttribute(i);

        if (attr.isDeleted || attr.pointer == nullptr) { continue; }

        if (attr.pointer->GetName() == name) { return attr.pointer; }
    }

    return nullptr;
}


// ============================================================
// 创建人工测试网格
//
// 8 Points
//
// Cell 0:
// old Points = [0, 2, 4, 6]
//
// Cell 1:
// old Points = [1, 3, 5, 7]
//
// 两个Cell完全不共享Point。
//
// 因此删除一个Cell以后：
//
// Cells:
// 2 -> 1
//
// Points:
// 8 -> 4
//
// 这样最方便检查Point清理以及ID重映射。
// ============================================================

iGame::UnstructuredMesh::Pointer CreateTwoTetraMesh() {
    auto mesh = iGame::UnstructuredMesh::New();

    mesh->SetName("RemoveGhostTestMesh");


    // --------------------------------------------------------
    // 创建8个Points
    //
    // Point的x坐标直接等于oldPointId。
    //
    // 例如：
    //
    // P0.x = 0
    // P2.x = 2
    // P4.x = 4
    // P6.x = 6
    //
    // 删除Cell后可直接检查输出Point来源。
    // --------------------------------------------------------

    for (int i = 0; i < 8; ++i) {
        iGame::Point p;

        p[0] = static_cast<float>(i);
        p[1] = 0.0f;
        p[2] = 0.0f;

        mesh->AddPoint(p);
    }


    // --------------------------------------------------------
    // Cell 0
    // --------------------------------------------------------

    igIndex cell0[4] = {0, 2, 4, 6};

    mesh->AddCell(cell0, 4, iGame::IG_TETRA);


    // --------------------------------------------------------
    // Cell 1
    // --------------------------------------------------------

    igIndex cell1[4] = {1, 3, 5, 7};

    mesh->AddCell(cell1, 4, iGame::IG_TETRA);


    return mesh;
}


// ============================================================
// 创建Cell级vtkGhostType
//
// ghost0 -> Cell 0
// ghost1 -> Cell 1
//
// VTK Ghost Cell中目前已验证：
//
// 0  = Normal Cell
// 1  = DUPLICATECELL
// 32 = HIDDENCELL
// ============================================================

iGame::UnsignedCharArray::Pointer CreateCellGhostAttribute(unsigned char ghost0, unsigned char ghost1) {
    auto array = iGame::UnsignedCharArray::New();

    array->SetName("vtkGhostType");

    array->SetDimension(1);

    array->Resize(2);

    array->SetValue(0, ghost0);

    array->SetValue(1, ghost1);

    return array;
}


iGame::UnsignedCharArray::Pointer CreatePointGhostAttribute(IGsize numberOfPoints) {
    auto array = iGame::UnsignedCharArray::New();

    array->SetName("vtkGhostType");

    array->SetDimension(1);

    array->Resize(numberOfPoints);

    for (IGsize i = 0; i < numberOfPoints; ++i) { array->SetValue(i, 0); }

    return array;
}


// ============================================================
// 创建Point普通属性
//
// Temperature:
//
// old Point 0 -> 100
// old Point 1 -> 101
// old Point 2 -> 102
// ...
// old Point 7 -> 107
// ============================================================

void AddPointAttribute(iGame::UnstructuredMesh::Pointer mesh) {
    auto array = iGame::FloatArray::New();

    array->SetName("Temperature");

    array->SetDimension(1);

    array->Resize(mesh->GetNumberOfPoints());

    for (IGsize i = 0; i < mesh->GetNumberOfPoints(); ++i) { array->SetValue(i, static_cast<float>(100 + i)); }

    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, array);
}


// ============================================================
// 创建Cell普通属性
//
// MaterialId:
//
// Cell 0 -> 10
// Cell 1 -> 20
// ============================================================

void AddCellAttribute(iGame::UnstructuredMesh::Pointer mesh) {
    auto array = iGame::IntArray::New();

    array->SetName("MaterialId");

    array->SetDimension(1);

    array->Resize(2);

    array->SetValue(0, 10);

    array->SetValue(1, 20);

    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, array);
}


void AddInt64Attributes(iGame::UnstructuredMesh::Pointer mesh) {
    auto pointArray = iGame::LongLongArray::New();

    pointArray->SetName("PointInt64");

    pointArray->SetDimension(1);

    pointArray->Resize(mesh->GetNumberOfPoints());

    auto* pointValues = pointArray->RawPointer();

    const long long basePointValue = 9007199254740993LL;

    for (IGsize i = 0; i < mesh->GetNumberOfPoints(); ++i) {
        pointValues[i] = basePointValue + static_cast<long long>(i);
    }

    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, pointArray);


    auto cellArray = iGame::UnsignedLongLongArray::New();

    cellArray->SetName("CellUInt64");

    cellArray->SetDimension(1);

    cellArray->Resize(mesh->GetNumberOfCells());

    auto* cellValues = cellArray->RawPointer();

    cellValues[0] = 9007199254740993ULL;

    cellValues[1] = 9007199254740995ULL;

    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, cellArray);
}


// ============================================================
// Test 1
//
// DUPLICATECELL = 1
//
// Cell 0:
// vtkGhostType = 0
//
// Cell 1:
// vtkGhostType = 1
//
// 预期：
//
// Cells:
// 2 -> 1
//
// Points:
// 8 -> 4
//
// vtkGhostType:
// removed
// ============================================================

bool TestDuplicateCell() {
    auto mesh = CreateTwoTetraMesh();


    auto ghost = CreateCellGhostAttribute(0, 1);


    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, ghost);


    auto filter = iGame::RemoveGhostInformationFilter::New();


    filter->SetInput(mesh);


    if (!filter->Execute()) { return false; }


    auto output = DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());


    if (output.IsNull()) { return false; }


    if (output->GetNumberOfCells() != 1) { return false; }


    if (output->GetNumberOfPoints() != 4) { return false; }


    if (CountGhostAttributes(output) != 0) { return false; }


    return true;
}


// ============================================================
// Test 2
//
// HIDDENCELL = 32
//
// Cell 0:
// vtkGhostType = 0
//
// Cell 1:
// vtkGhostType = 32
//
// 预期：
//
// Cells:
// 2 -> 1
//
// Points:
// 8 -> 4
// ============================================================

bool TestHiddenCell() {
    auto mesh = CreateTwoTetraMesh();


    auto ghost = CreateCellGhostAttribute(0, 32);


    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, ghost);


    auto filter = iGame::RemoveGhostInformationFilter::New();


    filter->SetInput(mesh);


    if (!filter->Execute()) { return false; }


    auto output = DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());


    if (output.IsNull()) { return false; }


    if (output->GetNumberOfCells() != 1) { return false; }


    if (output->GetNumberOfPoints() != 4) { return false; }


    if (CountGhostAttributes(output) != 0) { return false; }


    return true;
}


// ============================================================
// Test 3
//
// Point ID Remapping
//
// 删除Cell 1以后保留Cell 0。
//
// Cell 0原始Points：
//
// old 0
// old 2
// old 4
// old 6
//
// 新Point ID应为：
//
// old 0 -> new 0
// old 2 -> new 1
// old 4 -> new 2
// old 6 -> new 3
//
// 输出Cell Connectivity应变成：
//
// [0, 1, 2, 3]
// ============================================================

bool TestPointRemapping() {
    auto mesh = CreateTwoTetraMesh();


    auto ghost = CreateCellGhostAttribute(0, 1);


    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, ghost);


    auto filter = iGame::RemoveGhostInformationFilter::New();


    filter->SetInput(mesh);


    if (!filter->Execute()) { return false; }


    auto output = DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());


    if (output.IsNull()) { return false; }


    if (output->GetNumberOfPoints() != 4) { return false; }


    // --------------------------------------------------------
    // 检查新Point实际来自哪些old Point
    // --------------------------------------------------------

    const double expectedX[4] = {0.0, 2.0, 4.0, 6.0};


    for (IGsize i = 0; i < 4; ++i) {
        if (!AlmostEqual(output->GetPoint(i)[0], expectedX[i])) { return false; }
    }


    // --------------------------------------------------------
    // 检查Cell Connectivity
    // --------------------------------------------------------

    igIndex ids[IGAME_CELL_MAX_SIZE]{};


    const int numberOfCellPoints = output->GetCellPointIds(0, ids);


    if (numberOfCellPoints != 4) { return false; }


    for (int i = 0; i < 4; ++i) {
        if (ids[i] != i) { return false; }
    }


    return true;
}


// ============================================================
// Test 4
//
// Point Attribute Remapping
//
// 原Temperature:
//
// Point 0 -> 100
// Point 1 -> 101
// Point 2 -> 102
// Point 3 -> 103
// Point 4 -> 104
// Point 5 -> 105
// Point 6 -> 106
// Point 7 -> 107
//
// 保留Points:
//
// old 0
// old 2
// old 4
// old 6
//
// 因此输出：
//
// 100
// 102
// 104
// 106
// ============================================================

bool TestPointAttribute() {
    auto mesh = CreateTwoTetraMesh();


    AddPointAttribute(mesh);


    auto ghost = CreateCellGhostAttribute(0, 1);


    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, ghost);


    auto filter = iGame::RemoveGhostInformationFilter::New();


    filter->SetInput(mesh);


    if (!filter->Execute()) { return false; }


    auto output = filter->GetOutput();


    auto array = GetAttributeArrayByName(output, "Temperature");


    if (array == nullptr) { return false; }


    if (array->GetNumberOfElements() != 4) { return false; }


    const double expected[4] = {100.0, 102.0, 104.0, 106.0};


    for (IGsize i = 0; i < 4; ++i) {
        if (!AlmostEqual(array->GetValue(i), expected[i])) { return false; }
    }


    return true;
}


// ============================================================
// Test 5
//
// Cell Attribute Remapping
//
// MaterialId:
//
// Cell 0 -> 10
// Cell 1 -> 20
//
// Cell 1是Ghost。
//
// 所以输出只应该剩：
//
// 10
// ============================================================

bool TestCellAttribute() {
    auto mesh = CreateTwoTetraMesh();


    AddCellAttribute(mesh);


    auto ghost = CreateCellGhostAttribute(0, 1);


    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, ghost);


    auto filter = iGame::RemoveGhostInformationFilter::New();


    filter->SetInput(mesh);


    if (!filter->Execute()) { return false; }


    auto output = filter->GetOutput();


    auto array = GetAttributeArrayByName(output, "MaterialId");


    if (array == nullptr) { return false; }


    if (array->GetNumberOfElements() != 1) { return false; }


    if (!AlmostEqual(array->GetValue(0), 10.0)) { return false; }


    return true;
}


// ============================================================
// Test 6
//
// Attribute Type Preservation
//
// Temperature:
// FloatArray -> FloatArray
//
// MaterialId:
// IntArray -> IntArray
// ============================================================

bool TestAttributeTypes() {
    auto mesh = CreateTwoTetraMesh();


    AddPointAttribute(mesh);

    AddCellAttribute(mesh);


    auto ghost = CreateCellGhostAttribute(0, 1);


    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, ghost);


    auto filter = iGame::RemoveGhostInformationFilter::New();


    filter->SetInput(mesh);


    if (!filter->Execute()) { return false; }


    auto output = filter->GetOutput();


    auto pointArray = GetAttributeArrayByName(output, "Temperature");


    auto cellArray = GetAttributeArrayByName(output, "MaterialId");


    if (pointArray == nullptr || cellArray == nullptr) { return false; }


    if (pointArray->GetArrayType() != IG_FloatArray) { return false; }


    if (cellArray->GetArrayType() != IG_IntArray) { return false; }


    return true;
}


bool TestPointGhostOnly() {
    auto mesh = CreateTwoTetraMesh();


    AddPointAttribute(mesh);

    AddCellAttribute(mesh);


    auto pointGhost = CreatePointGhostAttribute(mesh->GetNumberOfPoints());


    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, pointGhost);


    auto filter = iGame::RemoveGhostInformationFilter::New();


    filter->SetInput(mesh);


    if (!filter->Execute()) { return false; }


    if (!filter->WasModified()) { return false; }


    auto output = DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());


    if (output.IsNull()) { return false; }


    if (output == mesh) { return false; }


    if (output->GetNumberOfPoints() != mesh->GetNumberOfPoints()) { return false; }


    if (output->GetNumberOfCells() != mesh->GetNumberOfCells()) { return false; }


    if (CountGhostAttributes(output) != 0) { return false; }


    if (GetAttributeArrayByName(output, "Temperature") == nullptr) { return false; }


    if (GetAttributeArrayByName(output, "MaterialId") == nullptr) { return false; }


    return true;
}


bool TestInt64Precision() {
    auto mesh = CreateTwoTetraMesh();


    AddInt64Attributes(mesh);


    auto ghost = CreateCellGhostAttribute(0, 1);


    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, ghost);


    auto filter = iGame::RemoveGhostInformationFilter::New();


    filter->SetInput(mesh);


    if (!filter->Execute()) { return false; }


    auto output = filter->GetOutput();


    auto pointBase = GetAttributeArrayByName(output, "PointInt64");


    auto cellBase = GetAttributeArrayByName(output, "CellUInt64");


    auto pointArray = DynamicCast<iGame::LongLongArray>(pointBase);


    auto cellArray = DynamicCast<iGame::UnsignedLongLongArray>(cellBase);


    if (pointArray.IsNull() || cellArray.IsNull()) { return false; }


    if (pointArray->GetNumberOfElements() != 4) { return false; }


    if (cellArray->GetNumberOfElements() != 1) { return false; }


    const auto* pointValues = pointArray->RawPointer();


    const auto* cellValues = cellArray->RawPointer();


    if (pointValues == nullptr || cellValues == nullptr) { return false; }


    const long long expectedPointValues[4] = {9007199254740993LL, 9007199254740995LL, 9007199254740997LL,
                                              9007199254740999LL};


    for (IGsize i = 0; i < 4; ++i) {

        if (pointValues[i] != expectedPointValues[i]) { return false; }
    }


    if (cellValues[0] != 9007199254740993ULL) { return false; }


    return true;
}


// ============================================================
// Test 7
//
// No Ghost Attribute
//
// 如果输入中没有Point/Cell vtkGhostType：
//
// 不删除Cell
// 不删除Point
// 普通属性保持不变
// ============================================================

bool TestNoGhostAttribute() {
    auto mesh = CreateTwoTetraMesh();


    AddPointAttribute(mesh);

    AddCellAttribute(mesh);


    auto filter = iGame::RemoveGhostInformationFilter::New();


    filter->SetInput(mesh);


    if (!filter->Execute()) { return false; }


    if (filter->WasModified()) { return false; }


    if (filter->GetOutput() != nullptr) { return false; }


    if (mesh->GetNumberOfCells() != 2) { return false; }


    if (mesh->GetNumberOfPoints() != 8) { return false; }


    auto temperature = GetAttributeArrayByName(mesh, "Temperature");


    auto materialId = GetAttributeArrayByName(mesh, "MaterialId");


    if (temperature == nullptr || materialId == nullptr) { return false; }


    if (temperature->GetNumberOfElements() != 8) { return false; }


    if (materialId->GetNumberOfElements() != 2) { return false; }


    return true;
}


} // namespace


int main() {
    if (!TestDuplicateCell()) {
        std::cerr << "ERROR: DUPLICATECELL" << std::endl;

        return 1;
    }

    std::cout << "Success: DUPLICATECELL" << std::endl;


    if (!TestHiddenCell()) {
        std::cerr << "ERROR: HIDDENCELL" << std::endl;

        return 1;
    }

    std::cout << "Success: HIDDENCELL" << std::endl;


    if (!TestPointRemapping()) {
        std::cerr << "ERROR: Point Remapping" << std::endl;

        return 1;
    }

    std::cout << "Success: Point Remapping" << std::endl;


    if (!TestPointAttribute()) {
        std::cerr << "ERROR: Point Attribute" << std::endl;

        return 1;
    }

    std::cout << "Success: Point Attribute" << std::endl;


    if (!TestCellAttribute()) {
        std::cerr << "ERROR: Cell Attribute" << std::endl;

        return 1;
    }

    std::cout << "Success: Cell Attribute" << std::endl;


    if (!TestAttributeTypes()) {
        std::cerr << "ERROR: Attribute Types" << std::endl;

        return 1;
    }

    std::cout << "Success: Attribute Types" << std::endl;


    if (!TestPointGhostOnly()) {
        std::cerr << "ERROR: Point Ghost Only" << std::endl;

        return 1;
    }

    std::cout << "Success: Point Ghost Only" << std::endl;


    if (!TestInt64Precision()) {
        std::cerr << "ERROR: Int64 Precision" << std::endl;

        return 1;
    }

    std::cout << "Success: Int64 Precision" << std::endl;


    if (!TestNoGhostAttribute()) {
        std::cerr << "ERROR: No Ghost Attribute" << std::endl;

        return 1;
    }

    std::cout << "Success: No Ghost Attribute" << std::endl;


    std::cout << "All RemoveGhostInformation core tests passed." << std::endl;


    return 0;
}
