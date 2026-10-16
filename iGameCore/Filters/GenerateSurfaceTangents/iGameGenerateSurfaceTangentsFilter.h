#ifndef iGameGenerateSurfaceTangentsFilter_h
#define iGameGenerateSurfaceTangentsFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class GenerateSurfaceTangentsFilter : public Filter {
public:
    I_OBJECT(GenerateSurfaceTangentsFilter);
    static Pointer New() { return new GenerateSurfaceTangentsFilter; }

    bool Execute() override;

    /* 点切向量（默认开） */
    void SetComputePointTangents(bool value) { m_ComputePointTangents = value; }
    bool GetComputePointTangents() const { return m_ComputePointTangents; }

    /* 单元切向量（默认关） */
    void SetComputeCellTangents(bool value) { m_ComputeCellTangents = value; }
    bool GetComputeCellTangents() const { return m_ComputeCellTangents; }

    /* 副切向量（默认关，VTK 没有这个输出） */
    void SetComputeBitangents(bool value) { m_ComputeBitangents = value; }
    bool GetComputeBitangents() const { return m_ComputeBitangents; }

    /* 用哪个点属性当纹理坐标；留空 = 自动找第一个 IG_TCOORD 的点属性 */
    void SetTextureCoordinatesArrayName(const std::string& name) { m_TexCoordsArrayName = name; }
    const std::string& GetTextureCoordinatesArrayName() const { return m_TexCoordsArrayName; }

    void SetPointTangentsArrayName(const std::string& name) {
        if (!name.empty()) { m_PointTangentsArrayName = name; }
    }
    const std::string& GetPointTangentsArrayName() const { return m_PointTangentsArrayName; }

    void SetCellTangentsArrayName(const std::string& name) {
        if (!name.empty()) { m_CellTangentsArrayName = name; }
    }
    const std::string& GetCellTangentsArrayName() const { return m_CellTangentsArrayName; }

    void SetBitangentsArrayName(const std::string& name) {
        if (!name.empty()) { m_BitangentsArrayName = name; }
    }
    const std::string& GetBitangentsArrayName() const { return m_BitangentsArrayName; }

protected:
    GenerateSurfaceTangentsFilter();
    ~GenerateSurfaceTangentsFilter() override = default;

private:
    /* 三角化后的一条三角形：三个点下标 + 它属于哪个单元（用于单元切向量） */
    struct Triangle {
        igIndex ids[3]{};
        IGsize cellId{0};
    };

    /* 把输入的面/单元拆成三角形；返回 false 表示输入类型不支持 */
    bool CollectTriangles(DataObject::Pointer input, std::vector<Triangle>& triangles, IGsize& cellCount) const;

    /* 找纹理坐标属性的下标；-1 表示没找到 */
    int FindTextureCoordinates(DataObject::Pointer input) const;

    /* 深拷贝输入（点、单元、属性），得到独立输出；不支持的类型返回 nullptr */
    DataObject::Pointer CopyInput(DataObject::Pointer input) const;

    /* 把数组挂到输出上（同名 + 同附着类型则原地替换，避免留下空占位） */
    static bool SetAttribute(DataObject::Pointer output, const std::string& name, IGenum type,
                             IGenum attachmentType, ArrayObject::Pointer array);

    bool m_ComputePointTangents{true};
    bool m_ComputeCellTangents{false};
    bool m_ComputeBitangents{false};
    std::string m_TexCoordsArrayName{};
    std::string m_PointTangentsArrayName{"Tangents"};
    std::string m_CellTangentsArrayName{"Tangents"};
    std::string m_BitangentsArrayName{"Bitangents"};
};

IGAME_NAMESPACE_END
#endif
