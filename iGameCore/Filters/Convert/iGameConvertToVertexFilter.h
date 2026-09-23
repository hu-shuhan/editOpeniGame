#pragma once

#include <iGameFilter.h>
#include <iGamePointSet.h>
#include <iGameUnstructuredMesh.h>

IGAME_NAMESPACE_BEGIN
class ConvertToVertexFilter : public Filter {
public:
    I_OBJECT(ConvertToVertexFilter);
    static Pointer New() { return new ConvertToVertexFilter; }

    bool Execute() override;

    // 转换模式（多模式框架，后续模式在此扩展）
    enum ConvertMethod {
        INVALID = -1,
        IG_CONVERT_POINT_TO_VERTEX = 0, // 默认：每个输入点 -> 一个 IG_VERTEX 顶点单元，忽略原单元
    };
    void SetConvertMethod(ConvertMethod method) { m_ConvertMethod = method; }
    ConvertMethod GetConvertMethod() const { return m_ConvertMethod; }

protected:
    ConvertToVertexFilter();
    ~ConvertToVertexFilter() override = default;

private:
    // 每个输入点复制为输出点，并生成一个引用该点的 IG_VERTEX 单元；
    // 点属性原样拷贝为输出点属性，不复制到顶点单元；原单元属性因数量无法对应而丢弃。
    bool ExecutePointToVertex(PointSet::Pointer in, UnstructuredMesh::Pointer out);

    ConvertMethod m_ConvertMethod{IG_CONVERT_POINT_TO_VERTEX};
};
IGAME_NAMESPACE_END
