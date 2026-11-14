#if defined(CGNS_ENABLE)
#ifndef iGameCGNSReader_h
#define iGameCGNSReader_h

#include <cgns_io.h>
#include <cgnslib.h>
#include <iGameFilter.h>
#include <iGameMacro.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <string>
#include <iGameFileReader.h>
IGAME_NAMESPACE_BEGIN
class iGameCGNSReader : public FileReader {

public:
    I_OBJECT(iGameCGNSReader);
    static iGameCGNSReader* New() { return new iGameCGNSReader; }

    bool Parsing() override;
    bool Execute() override;

    ~iGameCGNSReader();
    // Full-field Inf validation is diagnostic only and disabled by default.
    // Set IGAME_CGNS_CHECK_INF=1 or call this method to enable it.
    void SetCheckFieldInf(bool enabled) noexcept { m_CheckFieldInf = enabled; }
    bool GetCheckFieldInf() const noexcept { return m_CheckFieldInf; }
    void PrepareStructuredMesh(cgsize_t cellDim, cgsize_t* size);
    void ReadPointCoordinates(int pointNum, int positionDim, int index_file, int index_base, int index_zone,
                              cgsize_t* size);
    void ReadUnstructuredCellConnectivities(int index_file, int index_base, int index_zone, cgsize_t cellNum);
    void ReadFields(int index_file, int index_base, int index_zone, int index_sol, ZoneType_t zoneType, int celldim,
                    GridLocation_t location, cgsize_t* size);

    void ChangeMixElementToMyCell(std::vector<cgsize_t>, int);

    DataObject::Pointer GetCurrentDataObject() {
        switch (this->m_DataObjectType) {
            case IG_NONE:
                return nullptr;
            case IG_STRUCTURED_MESH:
                return m_StructuredMesh;
            case IG_UNSTRUCTURED_MESH:
                return m_UnstructuredMesh;
            case IG_VOLUME_MESH:
                return m_VolumeMesh;
            default:
                return nullptr;
        }
    }

private:
    iGameCGNSReader();
    Points::Pointer m_Points{nullptr};
    StructuredMesh::Pointer m_StructuredMesh{nullptr};
    UnstructuredMesh::Pointer m_UnstructuredMesh{nullptr};
    VolumeMesh::Pointer m_VolumeMesh{nullptr};
    AttributeSet::Pointer m_AttributeSet{nullptr};
    DataObject::Pointer m_ParentObject{nullptr};
    igIndex m_DataObjectType = IG_NONE;
    bool m_CheckFieldInf{false};
};

IGAME_NAMESPACE_END
#endif // iGameCGNSReader_h
#endif
