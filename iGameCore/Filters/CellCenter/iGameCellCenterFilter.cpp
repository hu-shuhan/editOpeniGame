#include "iGameCellCenterFilter.h"
#include "iGameCellArray.h"
#include "iGameStructuredMesh.h"

IGAME_NAMESPACE_BEGIN

CellCenterFilter::CellCenterFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

//CellCenterFilter: 遍历网格的所有单元，计算每个单元的几何中心
//输出一个 PointSet，每个点对应一个单元中心，可直接作为点云渲染
bool CellCenterFilter::Execute() {
    auto input = GetInput(0);
    if (!input) return false;

    // 按输入数组的实际类型创建同类型输出数组，
    // 避免 Double/Int 数组被降成 Float 导致精度和类型丢失
    auto createLikeArray = [](ArrayObject::Pointer src) -> ArrayObject::Pointer {
        switch (src->GetArrayType()) {
            case IG_DoubleArray:      return DoubleArray::New();
            case IG_IntArray:         return IntArray::New();
            case IG_UnsignedIntArray: return UnsignedIntArray::New();
            default:                  return FloatArray::New();
        }
    };

    auto inPoints = input->GetPoints();
    CellArray::Pointer inCells;
    if (input->GetDataObjectType() == IG_STRUCTURED_MESH) {
        // 结构化网格：GetCellArray() 返回空（隐式拓扑），
        // 先调用 GenStructuredCellConnectivities() 生成连接表，
        // 3D 取体单元，2D 取面单元。
        auto structured = DynamicCast<StructuredMesh>(input);
        if (!structured) return false;
        structured->GenStructuredCellConnectivities();
        if (structured->GetDimension() >= 3) {
            inCells = structured->GetVolumes();
        } else {
            inCells = structured->GetFaces();
        }
    } else {
        inCells = input->GetCellArray();
    }
    if (!inPoints || !inCells) return false;

    const IGsize inCellNum = inCells->GetNumberOfCells();

    //新建输出点集：每个单元一个中心点
    PointSet::Pointer centerSet = PointSet::New();
    centerSet->SetName(input->GetName() + "_cell_center");

    auto outPoints = centerSet->GetPoints();
    outPoints->Reset();
    outPoints->Reserve(inCellNum);

    //遍历单元，求几何中心
    igIndex ids[IGAME_CELL_MAX_SIZE] = {0};
    for (IGsize cellId = 0; cellId < inCellNum; cellId++) {
        const IGsize n = inCells->GetCellIds(cellId, ids);
        if (n <= 0) continue;

        Vector3f center(0.0f, 0.0f, 0.0f);
        for (IGsize j = 0; j < n; j++) {
            center += inPoints->GetPoint(ids[j]);
        }
        center /= static_cast<double>(n);
        outPoints->AddPoint(center);
    }

//点属性：输出点是单元中心，取该单元所有顶点的属性均值，作为该中心点的属性值
//单元属性：按单元索引拷贝（第i个中心点↔第i个单元），
//          统一标记为IG_POINT，PointSet渲染层才能识别并用于着色
    auto inAttr = input->GetAttributeSet();
    if (inAttr) {
        auto outAttr = AttributeSet::New();
        auto allAttrs = inAttr->GetAllAttributes();

        for (IGsize i = 0; i < allAttrs->GetNumberOfElements(); i++) {
            auto& attr = allAttrs->GetElement(i);
            if (attr.attachmentType == IG_POINT) {
                //点属性插值：中心点属性 = 该单元所有顶点属性值的均值
        auto outArray = createLikeArray(attr.pointer);
        outArray->SetName(attr.pointer->GetName());
                outArray->SetDimension(attr.pointer->GetDimension());
                outArray->Resize(inCellNum);

                const int dim = attr.pointer->GetDimension();
                double values[IGAME_CELL_MAX_SIZE] = {0};
                for (IGsize c = 0; c < inCellNum; c++) {
                    const IGsize n = inCells->GetCellIds(c, ids);
                    if (n <= 0) continue;

                    for (int d = 0; d < dim; d++) { values[d] = 0.0; }
                    for (IGsize k = 0; k < n; k++) {
                        double ptValues[IGAME_CELL_MAX_SIZE] = {0};
                        attr.pointer->GetElement(ids[k], ptValues);
                        for (int d = 0; d < dim; d++) { values[d] += ptValues[d]; }
                    }
                    for (int d = 0; d < dim; d++) { values[d] /= static_cast<double>(n); }
                    outArray->SetElement(c, values);
                }
                outAttr->AddAttribute(attr.type, IG_POINT, outArray, attr.GetDataRange());
            }
            else if (attr.attachmentType == IG_CELL) {
        auto outArray = createLikeArray(attr.pointer);
        outArray->SetName(attr.pointer->GetName());
                outArray->SetDimension(attr.pointer->GetDimension());
                outArray->Resize(inCellNum);

                double values[IGAME_CELL_MAX_SIZE] = {0};
                for (IGsize j = 0; j < inCellNum; j++) {
                    attr.pointer->GetElement(j, values);//将对应属性的第j个值赋给values
                    outArray->SetElement(j, values);//将values的值赋给我们要输出的outarray的数组指针的第j号位
                }
                outAttr->AddAttribute(attr.type, IG_POINT, outArray, attr.GetDataRange());
            }
            else{}//常规网格数据只用IG_POINT和IG_CELL，输出 PointSet 也只能容纳这两种
        }
        centerSet->SetAttributeSet(outAttr);
    }

    //挂到输出端口
    SetOutput(centerSet);
    return true;
}

IGAME_NAMESPACE_END
