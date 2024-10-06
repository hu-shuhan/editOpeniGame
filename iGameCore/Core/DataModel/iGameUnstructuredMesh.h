#ifndef iGameUnstructuredMesh_h
#define iGameUnstructuredMesh_h

#include "Quadratic/Base/iGameQuadraticLine.h"
#include "Quadratic/iGameQuadraticHexahedron.h"
#include "Quadratic/iGameQuadraticPrism.h"
#include "Quadratic/iGameQuadraticPyramid.h"
#include "Quadratic/iGameQuadraticQuad.h"
#include "Quadratic/iGameQuadraticTetra.h"
#include "Quadratic/iGameQuadraticTriangle.h"
#include "iGameCellArray.h"
#include "iGameEmptyCell.h"
#include "iGameHexahedron.h"
#include "iGameLine.h"
#include "iGamePointSet.h"
#include "iGamePolyLine.h"
#include "iGamePolygon.h"
#include "iGamePolyhedron.h"
#include "iGamePrism.h"
#include "iGamePyramid.h"
#include "iGameQuad.h"
#include "iGameScene.h"
#include "iGameSurfaceMesh.h"
#include "iGameTetra.h"
#include "iGameTriangle.h"
#include "iGameVertex.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN

class UnstructuredMesh : public PointSet {
public:
    I_OBJECT(UnstructuredMesh);
    static Pointer New() { return new UnstructuredMesh; }

    void SetCells(CellArray::Pointer cell, UnsignedIntArray::Pointer type);
    CellArray::Pointer GetCells() { return this->m_Cells; };
    void AddCell(igIndex* cell, int size, IGenum type);

    IGsize GetNumberOfCells() const noexcept;

    // Get cell's point index by index cellId. Return ids's size
    void GetCellPointIds(const IGsize cellId, IdArray::Pointer ids);
    int GetCellPointIds(const IGsize cellId, igIndex* ids);
    int GetCellPointIds(const IGsize cellId, const igIndex*& ids);

    // Get all cell's type.
    UnsignedIntArray* GetCellTypes() const;

    // Get cell's type by index cellId.
    IGenum GetCellType(const IGsize cellId) const;

    // Get cell by index cellId, which is Thread-Unsafe
    Cell* GetCell(const IGsize cellId);
    //Get DataObject Type
    IGenum GetDataObjectType() const { return IG_UNSTRUCTURED_MESH; }

    //Transfer to other mesh, if could not transfer , it will return nullptr;
    SurfaceMesh::Pointer TransferToSurfaceMesh();
    VolumeMesh::Pointer TransferToVolumeMesh();
    //extract other mesh, if could not transfer , you can use this to extract data to generate
    //a new mesh if it has other mesh's cell.
    SurfaceMesh::Pointer ExtractSurfaceMesh();
    VolumeMesh::Pointer ExtractVolumeMesh();
    //Generate from VolumeMesh
    bool GenerateFromVolumeMesh(VolumeMesh::Pointer volumeMesh);

    static bool
    TransferVolumeMeshToUnstructuredMesh(VolumeMesh::Pointer volumeMesh,
                                         UnstructuredMesh::Pointer&);

    SurfaceMesh::Pointer GetDrawMesh() { return m_DrawMesh; }

    //Get real size of DataObject
    IGsize GetRealMemorySize() override;
    bool GetClipped() override { return true; }

protected:
    UnstructuredMesh();
    ~UnstructuredMesh() override = default;

    // Get cell object according to cell type.
    Cell* GetTypedCell(const IGsize cellId);

    CellArray::Pointer m_Cells;        // Stores the PointIds of all cells
    UnsignedIntArray::Pointer m_Types; // Stores the Type of all cells

private:
    // Used for the returned cell object, which is Thread-Unsafe
    EmptyCell::Pointer m_EmptyCell{};
    Vertex::Pointer m_Vertex{};
    Line::Pointer m_Line{};
    PolyLine::Pointer m_PolyLine{};
    Triangle::Pointer m_Triangle{};
    Quad::Pointer m_Quad{};
    Polygon::Pointer m_Polygon{};
    Tetra::Pointer m_Tetra{};
    Hexahedron::Pointer m_Hexahedron{};
    Prism::Pointer m_Prism{};
    Pyramid::Pointer m_Pyramid{};
    Polyhedron::Pointer m_Polyhedron{};
    QuadraticLine::Pointer m_QuadraticLine{};
    QuadraticTriangle::Pointer m_QuadraticTriangle{};
    QuadraticQuad::Pointer m_QuadraticQuad{};
    QuadraticTetra::Pointer m_QuadraticTetra{};
    QuadraticHexahedron::Pointer m_QuadraticHexahedron{};
    QuadraticPrism::Pointer m_QuadraticPrism{};
    QuadraticPyramid::Pointer m_QuadraticPyramid{};

public:
    //void Draw(Scene* scene) override;
    void ConvertToDrawableData() override;
    bool IsDrawable() override { return true; }
    void ViewCloudPicture(Scene* scene, int index, int demension = -1) override;
    void SetAttributeWithPointData(ArrayObject::Pointer attr,
                                   std::pair<float, float>& range,
                                   igIndex i = -1) override;
    void SetAttributeWithCellData(ArrayObject::Pointer attr, igIndex i = -1);

protected:
    SurfaceMesh::Pointer m_DrawMesh{nullptr};
};
IGAME_NAMESPACE_END
#endif