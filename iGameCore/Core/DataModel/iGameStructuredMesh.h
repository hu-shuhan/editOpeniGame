#ifndef StructuredMesh_h
#define StructuredMesh_h
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameHexahedron.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameQuad.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
IGAME_NAMESPACE_BEGIN
class StructuredMesh : public VolumeMesh {
public:
    I_OBJECT(StructuredMesh);

    // Factory method to create a new instance of StructuredMesh.
    static StructuredMesh* New() { return new StructuredMesh; }

    // Return the type of data object as IG_STRUCTURED_MESH.
    IGenum GetDataObjectType() const { return IG_STRUCTURED_MESH; }

    // Get the number of cells in the structured mesh.
    IGsize GetNumberOfCells();

    // Set the dimensions of the mesh.
    void SetDimensionSize(igIndex s[3]);

    // Set the extent of the mesh (start and end indices in x, y, and z directions).
    void SetExtent(igIndex e[6]) { std::copy(e, e + 6, this->size); }

    // Get the dimension sizes of the mesh.
    igIndex* GetDimensionSize() { return this->size; }

    // Get the extent of the mesh.
    igIndex* GetExtent() { return this->extent; }

    // Get the dimension of the mesh (typically 3 for 3D).
    igIndex GetDimension() { return this->Dimension; }

    // Build the structured faces of the mesh.
    void BuildStructuredFaces();

    // Generate the connectivity information for structured cells.
    void GenStructuredCellConnectivities();

    // Get the index of a point given its i, j, k coordinates.
    igIndex GetPointIndex(igIndex i, igIndex j, igIndex k);

    // Get the index of a volume element (cell) given its i, j, k coordinates.
    igIndex GetVolumeIndex(igIndex i, igIndex j, igIndex k);

    // Override to view a cloud picture representation of the mesh.
    //void ViewCloudPicture(Scene* scene, int index, int demension = -1) override;

    //Get real size of DataObject
    IGsize GetRealMemorySize() override;

protected:
    // Default constructor for StructuredMesh.
    StructuredMesh() = default;
    // Destructor for StructuredMesh.
    ~StructuredMesh() override;
    // Dimension of the mesh (typically 3 for 3D).
    igIndex Dimension = 3;

    // Size of the mesh in each dimension (ni, nj, nk).
    igIndex size[3] = {0, 0, 0};

    // Extent of the mesh (start and end indices in x, y, and z directions).
    igIndex extent[6] = {0, 0, 0, 0, 0, 0};

    // Flag indicating whether the structured connectivity has been built.
    bool m_BuildStructuredConnectivty = false;
};


IGAME_NAMESPACE_END
#endif