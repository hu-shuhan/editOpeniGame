#ifndef iGameLagrangeUnstructuredMesh_h
#define iGameLagrangeUnstructuredMesh_h


#include "iGameCellArray.h"
#include "iGameEmptyCell.h"
#include "iGamePointSet.h"
#include "iGameScene.h"
#include "iGameLagrangeFace.h"
#include "iGameLagrangeHexahedron.h"
#include "iGameLagrangeLine.h"
#include "iGameLagrangePyramid.h"
#include "iGameLagrangePrism.h"
#include "iGameLagrangeQuadrilateral.h"
#include "iGameLagrangeTetrahedron.h"
#include "iGameLagrangeTriangle.h"
#include "iGameLagrangeVolume.h"


IGAME_NAMESPACE_BEGIN

class LagrangeUnstructuredMesh : public PointSet {
public:
    I_OBJECT(LagrangeUnstructuredMesh);
    static SmartPointer<LagrangeUnstructuredMesh> New() { return new LagrangeUnstructuredMesh; }

    // --- 数据设置与访问 ---
    void AddCell(igIndex* cell_nodes, int num_nodes, IGenum specific_cell_type, int order);

    IGsize GetNumberOfCells() const noexcept;
    int GetCellPointIds(const IGsize cellId, const igIndex*& ids);
    void GetCellPointIds(const IGsize cellId, IdArray::Pointer ids);

    // 返回精确的单元类型 (e.g., IG_LAGRANGE_HEXAHEDRON)
    IGenum GetSpecificCellType(const IGsize cellId) const;
    int GetCellOrder(const IGsize cellId) const;

    IGenum GetDataObjectType() const override { return IG_LAGRANGE_UNSTRUCTURED_MESH; }

    // --- 渲染与数据转换 ---
    Cell* GetCell(const IGsize cellId);
    void ConvertToDrawableData() override;

    void SetAttributeWithCellData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange,
                                  igIndex dimension = -1) override;

    FloatArray::Pointer GetTriangleQualities() const { return m_TriangleQualities; }

protected:
    LagrangeUnstructuredMesh();
    ~LagrangeUnstructuredMesh() override = default;

    // --- 核心数据存储 ---
    CellArray::Pointer m_Cells{};
    UnsignedIntArray::Pointer m_CellShapes{}; // 存储精确的单元类型
    IntArray::Pointer m_CellOrders{};

    ArrayObject::Pointer m_CellQualityData{}; // 用于存储原始的单元质量数据
    FloatArray::Pointer m_TriangleQualities{};  // 用于存储每个渲染三角形的质量
private:

};

IGAME_NAMESPACE_END
#endif

