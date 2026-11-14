#if defined(CGNS_ENABLE)
#include "iGameCGNSReader.h"
#include "Log/iGameLogger.h"
#include "ModelSurface/iGameModelGeometryFilter.h"
#include <cmath>
#include <cstdlib>
#include <limits>
#include <new>
#include <type_traits>
#include <vector>
IGAME_NAMESPACE_BEGIN

template<typename ArrayType>
static void ReportCgnsFieldInf(const char* fieldName, int index_base, int index_zone, int index_sol, int index_field,
                               const ArrayType& array) {
    if (!array) { return; }
    const auto* values = array->RawPointer();
    const IGsize count = array->GetNumberOfValues();
    for (IGsize i = 0; i < count; ++i) {
        if (std::isinf(values[i])) {
            IGAME_CORE_WARN("CGNS field '{}' contains Inf at value index {} (base {}, zone {}, solution {}, field {}); continuing read",
                            fieldName, i, index_base, index_zone, index_sol, index_field);
            return;
        }
    }
}

static bool TryBatchReadCgnsFixedCells(UnstructuredMesh::Pointer mesh,
                                       int index_file,
                                       int index_base,
                                       int index_zone,
                                       cgsize_t expected_cells);
static bool TryBatchReadCgnsPolyhedra(VolumeMesh::Pointer mesh,
                                      int index_file,
                                      int index_base,
                                      int index_zone,
                                      cgsize_t expected_cells);

iGameCGNSReader::iGameCGNSReader() {
    this->SetNumberOfOutputs(1);
    const char* checkInf = std::getenv("IGAME_CGNS_CHECK_INF");
    m_CheckFieldInf = checkInf != nullptr &&
            (std::strcmp(checkInf, "1") == 0 ||
             std::strcmp(checkInf, "true") == 0 ||
             std::strcmp(checkInf, "TRUE") == 0);
}
iGameCGNSReader::~iGameCGNSReader() {}

bool iGameCGNSReader::Parsing() 
{
    //CGNS不通过字节流Parsing
    return true;
}

bool iGameCGNSReader::Execute() {
    auto fileName = this->m_FilePath;
    int result;
    int file_type;
    result = cgio_check_file(fileName.c_str(), &file_type);
    if (file_type == CG_FILE_NONE) {
        IGAME_CORE_ERROR("Not a CGNS file.");
        return false;
    } else if (file_type == CG_FILE_ADF) {
        IGAME_CORE_DEBUG("ADF (Advanced Data Format) file.");
    } else if (file_type == CG_FILE_HDF5) {
        IGAME_CORE_DEBUG("HDF5 (Hierarchical Data Format) file.");
    } else {
        IGAME_CORE_WARN("Unknown file type.");
        return false;
    }
    this->UpdateProgress(0.05);
    int index_file;
    result = cg_open(fileName.data(), CG_MODE_READ, &index_file);
    if (CG_OK != result) {
        IGAME_CORE_ERROR("Open Error: {}", cg_get_error());
    } else {
        IGAME_CORE_DEBUG("Success to open cgns file!");
    }
    this->UpdateProgress(0.10);

    this->UpdateProgress(0.20);

    int nbases;
    result = cg_nbases(index_file, &nbases);
    if (CG_OK != result) {
        IGAME_CORE_ERROR("Get nbases Error: {}", cg_get_error());
    } else {
        // std::cout << "Base Num = " << nbases << std::endl;
    }
    this->UpdateProgress(0.15);
    m_ParentObject = DrawObject::New();
    double base_progress_step = 0.7 / nbases;  // 70% of total progress for all bases
    for (int index_base = 1; index_base <= nbases; index_base++) {
        char basename[100];
        int celldim, physdim;
        result = cg_base_read(index_file, index_base, basename, &celldim, &physdim);
        if (CG_OK != result) {
            IGAME_CORE_ERROR("Read Base: {}", cg_get_error());
        } else {
            // std::cout << "BaseName = " << basename << std::endl;
            // std::cout << "Dimension of the cells = " << celldim << std::endl;
            // std::cout << "Number of coordinates required to define a vector in the field is " << physdim << std::endl;
        }
        int nzones;
        result = cg_nzones(index_file, index_base, &nzones);
        if (CG_OK != result) {
            IGAME_CORE_ERROR("Get nzones Error: {}", cg_get_error());
        } else {
            IGAME_CORE_DEBUG("Zone Num of Base({}) = {}", index_base, nzones);
            // 计算每个 zone 的进度步长 (总进度 70% 分配给所有 base 和 zone)
            double zone_progress_step = base_progress_step / nzones;
            for (int index_zone = 1; index_zone <= nzones; index_zone++) {
                // 当前 zone 的基础进度
                double zone_base_progress = 0.20 + (index_base - 1) * base_progress_step + (index_zone - 1) * zone_progress_step;
                DataObject::Pointer DataSet = DataObject::New();
                ZoneType_t zoneType;
                result = cg_zone_type(index_file, index_base, index_zone, &zoneType);
                if (CG_OK != result) { IGAME_CORE_ERROR("Get Zone Type Error: {}", cg_get_error()); }
                cgsize_t size[9] = {0};
                char zonename[100];
                result = cg_zone_read(index_file, index_base, index_zone, zonename, size);
                if (CG_OK != result) {
                    IGAME_CORE_ERROR("Zone Read Error: {}", cg_get_error());
                } else {
                    IGAME_CORE_DEBUG("ZoneName = {}", zonename);
                    if (zoneType == Structured) {
                        IGAME_CORE_DEBUG("ZoneType = Structured");
                        if (m_StructuredMesh == nullptr) { m_StructuredMesh = StructuredMesh::New(); }
                        this->m_DataObjectType = IG_STRUCTURED_MESH;
                        // Calculate total points and cells for structured zones
                        cgsize_t totalPoints = 1, totalCells = 1;
                        for (int i = 0; i < celldim; ++i) {
                            totalPoints *= size[i];
                            if (size[celldim + i] == 0) { break; }
                            totalCells *= size[celldim + i];
                        }
                        IGAME_CORE_DEBUG("Total Points: {}", totalPoints);
                        IGAME_CORE_DEBUG("Total Cells: {}", totalCells);
                        // Read vertices coordinates
                        this->ReadPointCoordinates(totalPoints, physdim, index_file, index_base, index_zone, size);
                        // 读取完点后更新进度 (30% of zone progress)
                        this->UpdateProgress(zone_base_progress + zone_progress_step * 0.3);
                        // Connectivity is implicit for structured zones; keep it lazy
                        // until rendering or another consumer explicitly requests cells.
                        this->PrepareStructuredMesh(celldim, size);
                        // 网格维度和点已就绪，更新进度 (70% of zone progress)
                        this->UpdateProgress(zone_base_progress + zone_progress_step * 0.7);
                    } else if (zoneType == Unstructured) {
                        IGAME_CORE_DEBUG("ZoneType = Unstructured");
                        IGAME_CORE_DEBUG("Total Points: {}", size[0]);
                        IGAME_CORE_DEBUG("Total Cells: {}", size[1]);
                        // Read vertices coordinates
                        this->ReadPointCoordinates(size[0], physdim, index_file, index_base, index_zone, size);
                        // 读取完点后更新进度 (30% of zone progress)
                        this->UpdateProgress(zone_base_progress + zone_progress_step * 0.3);
                        // Read cells (connectivity)
                        this->ReadUnstructuredCellConnectivities(index_file, index_base, index_zone, size[1]);
                        // 读取完单元后更新进度 (70% of zone progress)
                        this->UpdateProgress(zone_base_progress + zone_progress_step * 0.7);
                    } else {
                        IGAME_CORE_WARN("Unknown ZoneType!");
                        this->m_DataObjectType = IG_NONE;
                    }
                    /* Flow Solution */
                    int nsols;
                    result = cg_nsols(index_file, index_base, index_zone, &nsols);
                    if (CG_OK != result) {
                        IGAME_CORE_ERROR("Get Num Of Solution Error: {}", cg_get_error());
                    } else {
                        IGAME_CORE_DEBUG("Solution Num = {}", nsols);
                    }
                    for (int index_sol = 1; index_sol <= nsols; index_sol++) {
                        char solname[100];
                        GridLocation_t location;
                        result = cg_sol_info(index_file, index_base, index_zone, index_sol, solname, &location);
                        if (CG_OK != result) {
                            IGAME_CORE_ERROR("Read Solution Info Error: {}", cg_get_error());
                        }
                        //std::cout << "Solution Name = " << solname << std::endl;
                        switch (location) {
                            case GridLocation_t::Vertex:
                                //std::cout << "Solution Location = Vertex" << std::endl;
                                break;
                            case CellCenter:
                                //std::cout << "Solution Location = CellCenter" << std::endl;
                                break;
                            case IFaceCenter:
                                //std::cout << "Solution Location = IFaceCenter" << std::endl;
                                break;
                            case JFaceCenter:
                                //std::cout << "Solution Location = JFaceCenter" << std::endl;
                                break;
                            case KFaceCenter:
                                //std::cout << "Solution Location = KFaceCenter" << std::endl;
                                break;
                            default:
                                IGAME_CORE_WARN("Solution Location is bad data! Unknown!");
                        }
                        this->ReadFields(index_file, index_base, index_zone, index_sol, zoneType, celldim, location,
                                         size);
                    }
                    if (this->m_AttributeSet) {
                        this->GetCurrentDataObject()->SetAttributeSet(this->m_AttributeSet);
                    }
                    // 读取完属性后更新进度 (100% of zone progress)
                    this->UpdateProgress(zone_base_progress + zone_progress_step);
                }
                m_ParentObject->AddSubDataObject(this->GetCurrentDataObject());
                this->m_Points = nullptr;
                this->m_StructuredMesh = nullptr;
                this->m_UnstructuredMesh = nullptr;
                this->m_VolumeMesh = nullptr;
                this->m_AttributeSet = nullptr;
            }
        }
    }
    if (this->m_ParentObject->GetNumberOfSubDataObjects() == 1) {
        auto dataobject = m_ParentObject->SubDataObjectIteratorBegin()->second;
        this->SetOutput(m_ParentObject->SubDataObjectIteratorBegin()->second);
    } else {
        this->SetOutput(m_ParentObject);
    }
    cg_close(index_file);
    this->UpdateProgress(1.0);
    return true;
}
void iGameCGNSReader::ReadPointCoordinates(int pointNum, int positionDim, int index_file, int index_base,
    int index_zone, cgsize_t* size) {
    if (m_Points == nullptr) { m_Points = Points::New(); }
    m_Points->SetNumberOfPoints(pointNum);
    auto points = m_Points->RawPointer();

    for (int dim = 0; dim < positionDim; ++dim) {
        // 获取 GridCoordinates 下第 dim+1 个坐标名称
        char coordName[33];
        CGNS_ENUMT(DataType_t) datatype;

        int result = cg_coord_info(index_file, index_base, index_zone, dim + 1, &datatype, coordName);
    if (CG_OK != result) {
            IGAME_CORE_ERROR("Get coordinate info failed for dim {}: {}", dim, cg_get_error());
            continue;
        }

        DoubleArray::Pointer CoordData = DoubleArray::New();
        CoordData->Resize(pointNum);
        auto coordData = CoordData->RawPointer();

        cgsize_t range_min[3] = { 1, 1, 1 };
        cgsize_t range_max[3] = { size[0], size[1], size[2] };

        result = cg_coord_read(index_file, index_base, index_zone, coordName, RealDouble, range_min,
            range_max, coordData);
        if (CG_OK != result) {
            IGAME_CORE_ERROR("Read {} Error: {}", coordName, cg_get_error());
            continue;
        }

        int idx = dim;
        for (cgsize_t j = 0; j < pointNum; ++j) {
            points[idx] = coordData[j];
            idx += positionDim;
        }
    }
}


void iGameCGNSReader::PrepareStructuredMesh(cgsize_t cellDim, cgsize_t* size) {
    if (m_StructuredMesh == nullptr) { m_StructuredMesh = StructuredMesh::New(); }
    m_StructuredMesh->SetPoints(m_Points);
    igIndex tmpSize[3] = {0};
    for (int i = 0; i < cellDim; i++) { tmpSize[i] = size[i]; }
    m_StructuredMesh->SetDimensionSize(tmpSize);
}
void iGameCGNSReader::ReadUnstructuredCellConnectivities(int index_file, int index_base, int index_zone,
                                                         cgsize_t cellNum) {
    //这边需要读取出邻接关系然后转换为我们的type
    int nsections;
    int result = cg_nsections(index_file, index_base, index_zone, &nsections);
    if (CG_OK != result) {
        IGAME_CORE_ERROR("Get Num Of Sections Error: {}", cg_get_error());
        return;
    }
    //paraview只读取了部分的section，应该是非结构化网格只有一个section
    //nsections = 1;
    //igDebug("the number of sections is " << nsections);
    int cgio_num;
    if (cg_get_cgio(index_file, &cgio_num) != CG_OK) {
        IGAME_CORE_ERROR("Get cgio num Error: {}", cg_get_error());
    }
    bool hasPolyGon = false;
    bool hasPolyHedron = false;
    bool hasUnstructGrid = false;
    for (int index_section = 1; index_section <= nsections; index_section++) {
        char sectionname[100];
        ElementType_t type;
        cgsize_t start, end;
        int nbndry, parent_flag;
        cg_section_read(index_file, index_base, index_zone, index_section, sectionname, &type, &start, &end, &nbndry,
                        &parent_flag);
        cgsize_t num_elements = end - start + 1;
        if (type == NGON_n) hasPolyGon = true;
        else if (type == NFACE_n)
            hasPolyHedron = true;
        else
            hasUnstructGrid = true;
    }
    if (hasPolyHedron) {
        if (!hasPolyGon || hasUnstructGrid) {
            igError("read Polyhedrons error");
        } else {
            if (m_VolumeMesh == nullptr) { m_VolumeMesh = VolumeMesh::New(); }
            m_VolumeMesh->SetPoints(m_Points);
            this->m_DataObjectType = IG_VOLUME_MESH;
            if (!TryBatchReadCgnsPolyhedra(m_VolumeMesh, index_file, index_base,
                                           index_zone, cellNum)) {
                IGAME_CORE_ERROR("Failed to batch-read CGNS polyhedral topology");
            }
        }
    } else {
        if (m_UnstructuredMesh == nullptr) { m_UnstructuredMesh = UnstructuredMesh::New(); }
        m_UnstructuredMesh->SetPoints(m_Points);
        this->m_DataObjectType = IG_UNSTRUCTURED_MESH;
        if (TryBatchReadCgnsFixedCells(m_UnstructuredMesh, index_file, index_base,
                                       index_zone, cellNum)) {
            return;
        }
        for (int index_section = 1; index_section <= nsections; index_section++) {
            char sectionname[100];
            ElementType_t type;
            cgsize_t start, end;
            int nbndry, parent_flag;
            cg_section_read(index_file, index_base, index_zone, index_section, sectionname, &type, &start, &end,
                            &nbndry, &parent_flag);
            cgsize_t num_elements = end - start + 1;
            if (type == MIXED) {
                cgsize_t elementDataSize;
                cg_ElementDataSize(index_file, index_base, index_zone, index_section, &elementDataSize);
                std::vector<cgsize_t> elements(elementDataSize);
                cg_poly_elements_read(index_file, index_base, index_zone, index_section, elements.data(), NULL, NULL);
                this->ChangeMixElementToMyCell(elements, num_elements);
                std::vector<cgsize_t> temp;
                elements.swap(temp);
            } else {
                igIndex vhs[64];
                int cellVcnt = 0;
                cg_npe(type, &cellVcnt);
                std::vector<cgsize_t> elements(num_elements * cellVcnt);
                cg_elements_read(index_file, index_base, index_zone, index_section, elements.data(), NULL);    
                if (type == HEXA_8) {
                    for (cgsize_t j = 0; j < num_elements; ++j) {
                        for (int k = 0; k < cellVcnt; k++) { vhs[k] = elements[j * cellVcnt + k] - 1; }
                        m_UnstructuredMesh->AddCell(vhs, cellVcnt, IG_HEXAHEDRON);
                    }
                } else if (type == TETRA_4) {
                    for (cgsize_t j = 0; j < num_elements; ++j) {
                        for (int k = 0; k < cellVcnt; k++) { vhs[k] = elements[j * cellVcnt + k] - 1; }
                        m_UnstructuredMesh->AddCell(vhs, cellVcnt, IG_TETRA);
                    }
                } else if (type == PYRA_5) {
                    for (cgsize_t j = 0; j < num_elements; ++j) {
                        for (int k = 0; k < cellVcnt; k++) { vhs[k] = elements[j * cellVcnt + k] - 1; }
                        m_UnstructuredMesh->AddCell(vhs, cellVcnt, IG_PYRAMID);
                    }
                } else if (type == PENTA_6) {
                    for (cgsize_t j = 0; j < num_elements; ++j) {
                        for (int k = 0; k < cellVcnt; k++) { vhs[k] = elements[j * cellVcnt + k] - 1; }
                        m_UnstructuredMesh->AddCell(vhs, cellVcnt, IG_PRISM);
                    }
                } else if (type == TRI_3) {
                    for (cgsize_t j = 0; j < num_elements; ++j) {
                        for (int k = 0; k < cellVcnt; k++) { vhs[k] = elements[j * cellVcnt + k] - 1; }
                        m_UnstructuredMesh->AddCell(vhs, cellVcnt, IG_TRIANGLE);
                    }
                } else if (type == QUAD_4) {
                    for (cgsize_t j = 0; j < num_elements; ++j) {
                        for (int k = 0; k < cellVcnt; k++) { vhs[k] = elements[j * cellVcnt + k] - 1; }
                        m_UnstructuredMesh->AddCell(vhs, cellVcnt, IG_QUAD);
                    }
                }
                std::vector<cgsize_t> temp;
                elements.swap(temp);
            }
        }
    }
}

static bool TryBatchReadCgnsPolyhedra(VolumeMesh::Pointer mesh,
                                      int index_file,
                                      int index_base,
                                      int index_zone,
                                      cgsize_t expected_cells) {
    if (!mesh || expected_cells <= 0) return false;

    int section_count = 0;
    if (cg_nsections(index_file, index_base, index_zone, &section_count) != CG_OK ||
        section_count <= 0) {
        return false;
    }

    struct PolySection {
        int index;
        ElementType_t type;
        cgsize_t elements;
        cgsize_t data_size;
    };
    std::vector<PolySection> sections;
    sections.reserve(static_cast<size_t>(section_count));

    IGsize total_faces = 0;
    IGsize total_face_data = 0;
    IGsize total_volumes = 0;
    IGsize total_volume_face_links = 0;
    for (int section_index = 1; section_index <= section_count; ++section_index) {
        char section_name[100];
        ElementType_t type;
        cgsize_t start = 0;
        cgsize_t end = 0;
        int boundary = 0;
        int parent = 0;
        if (cg_section_read(index_file, index_base, index_zone, section_index,
                            section_name, &type, &start, &end, &boundary,
                            &parent) != CG_OK ||
            end < start || (type != NGON_n && type != NFACE_n)) {
            return false;
        }

        cgsize_t data_size = 0;
        if (cg_ElementDataSize(index_file, index_base, index_zone,
                               section_index, &data_size) != CG_OK ||
            data_size < 0) {
            return false;
        }

        const IGsize element_count = static_cast<IGsize>(end - start + 1);
        const IGsize section_data_size = static_cast<IGsize>(data_size);
        if (type == NGON_n) {
            if (element_count > std::numeric_limits<IGsize>::max() - total_faces ||
                section_data_size >
                        std::numeric_limits<IGsize>::max() - total_face_data) {
                return false;
            }
            total_faces += element_count;
            total_face_data += section_data_size;
        } else {
            if (element_count > std::numeric_limits<IGsize>::max() - total_volumes ||
                section_data_size >
                        std::numeric_limits<IGsize>::max() - total_volume_face_links) {
                return false;
            }
            total_volumes += element_count;
            total_volume_face_links += section_data_size;
        }
        sections.push_back({section_index, type,
                            static_cast<cgsize_t>(element_count),
                            static_cast<cgsize_t>(section_data_size)});
    }

    if (total_faces == 0 || total_volumes == 0 ||
        total_volumes != static_cast<IGsize>(expected_cells)) {
        return false;
    }
    const IGsize max_offset =
            static_cast<IGsize>(std::numeric_limits<unsigned int>::max());
    const IGsize max_index =
            static_cast<IGsize>(std::numeric_limits<igIndex>::max());
    if (total_faces > max_index || total_face_data > max_offset ||
        total_volume_face_links > max_offset ||
        mesh->GetNumberOfPoints() > max_index + 1) {
        return false;
    }

    IdArray::Pointer face_ids;
    IdArray::Pointer volume_face_ids;
    UnsignedIntArray::Pointer face_offsets;
    UnsignedIntArray::Pointer volume_face_offsets;
    try {
        face_ids = IdArray::New();
        face_ids->Resize(total_face_data);
        volume_face_ids = IdArray::New();
        volume_face_ids->Resize(total_volume_face_links);
        face_offsets = UnsignedIntArray::New();
        face_offsets->Resize(total_faces + 1);
        volume_face_offsets = UnsignedIntArray::New();
        volume_face_offsets->Resize(total_volumes + 1);
    } catch (const std::bad_alloc&) {
        return false;
    }

    auto* face_id_values = face_ids->RawPointer();
    auto* volume_face_id_values = volume_face_ids->RawPointer();
    auto* face_offset_values = face_offsets->RawPointer();
    auto* volume_face_offset_values = volume_face_offsets->RawPointer();
    face_offset_values[0] = 0;
    volume_face_offset_values[0] = 0;

    IGsize face_cursor = 0;
    IGsize face_connection_cursor = 0;
    IGsize volume_cursor = 0;
    IGsize volume_face_cursor = 0;
    std::vector<cgsize_t> elements;
    std::vector<cgsize_t> offsets;
    for (const auto& section: sections) {
        try {
            elements.resize(static_cast<size_t>(section.data_size));
            offsets.resize(static_cast<size_t>(section.elements) + 1);
        } catch (const std::bad_alloc&) {
            return false;
        }
        if (cg_poly_elements_read(index_file, index_base, index_zone,
                                  section.index, elements.data(), offsets.data(),
                                  nullptr) != CG_OK) {
            return false;
        }

        const bool use_offsets =
                section.elements > 0 && offsets[0] != offsets[1];
        cgsize_t sequential = 0;
        for (cgsize_t element = 0; element < section.elements; ++element) {
            cgsize_t begin = 0;
            cgsize_t count = 0;
            if (use_offsets) {
                begin = offsets[static_cast<size_t>(element)];
                const cgsize_t next =
                        offsets[static_cast<size_t>(element) + 1];
                if (begin < 0 || next < begin) return false;
                count = next - begin;
            } else {
                if (sequential >= static_cast<cgsize_t>(elements.size())) {
                    return false;
                }
                count = elements[static_cast<size_t>(sequential++)];
                begin = sequential;
                if (count < 0) return false;
                sequential += count;
            }
            if (begin < 0 || count <= 0 ||
                static_cast<IGsize>(begin) + static_cast<IGsize>(count) >
                        static_cast<IGsize>(elements.size())) {
                return false;
            }

            if (section.type == NGON_n) {
                face_offset_values[face_cursor] =
                        static_cast<unsigned int>(face_connection_cursor);
                for (cgsize_t point = 0; point < count; ++point) {
                    const cgsize_t point_id =
                            elements[static_cast<size_t>(begin + point)];
                    if (point_id < 1 || point_id > max_index + 1) {
                        return false;
                    }
                    face_id_values[face_connection_cursor++] =
                            static_cast<igIndex>(point_id - 1);
                }
                ++face_cursor;
            } else {
                volume_face_offset_values[volume_cursor] =
                        static_cast<unsigned int>(volume_face_cursor);
                for (cgsize_t face = 0; face < count; ++face) {
                    const cgsize_t signed_face =
                            elements[static_cast<size_t>(begin + face)];
                    if (signed_face == 0) return false;
                    const cgsize_t face_id =
                            signed_face < 0 ? -signed_face : signed_face;
                    if (face_id > static_cast<cgsize_t>(total_faces) ||
                        face_id > static_cast<cgsize_t>(max_index) + 1) {
                        return false;
                    }
                    volume_face_id_values[volume_face_cursor++] =
                            static_cast<igIndex>(face_id - 1);
                }
                ++volume_cursor;
            }
        }
    }

    if (face_cursor != total_faces || volume_cursor != total_volumes ||
        face_connection_cursor > total_face_data ||
        volume_face_cursor > total_volume_face_links ||
        face_connection_cursor > max_offset ||
        volume_face_cursor > max_offset) {
        return false;
    }
    face_offset_values[total_faces] =
            static_cast<unsigned int>(face_connection_cursor);
    volume_face_offset_values[total_volumes] =
            static_cast<unsigned int>(volume_face_cursor);
    face_ids->Resize(face_connection_cursor);
    volume_face_ids->Resize(volume_face_cursor);

    auto faces = CellArray::New();
    faces->SetData(face_ids, face_offsets);
    auto volume_faces = CellArray::New();
    volume_faces->SetData(volume_face_ids, volume_face_offsets);
    return mesh->InitVolumesWithPolyhedron(faces, volume_faces);
}

struct CgnsFixedSection {
    int index;
    ElementType_t type;
    cgsize_t cells;
    int pointsPerCell;
    IGenum iGameType;
};

static bool MapCgnsFixedElementType(ElementType_t type, IGenum& result) {
    switch (type) {
        case HEXA_8: result = IG_HEXAHEDRON; return true;
        case TETRA_4: result = IG_TETRA; return true;
        case PYRA_5: result = IG_PYRAMID; return true;
        case PENTA_6: result = IG_PRISM; return true;
        case TRI_3: result = IG_TRIANGLE; return true;
        case QUAD_4: result = IG_QUAD; return true;
        default: return false;
    }
}

static bool TryBatchReadCgnsFixedCells(UnstructuredMesh::Pointer mesh,
                                       int index_file,
                                       int index_base,
                                       int index_zone,
                                       cgsize_t expected_cells) {
    if (!mesh || expected_cells <= 0) return false;

    int section_count = 0;
    if (cg_nsections(index_file, index_base, index_zone, &section_count) != CG_OK ||
        section_count <= 0) {
        return false;
    }

    std::vector<CgnsFixedSection> sections;
    sections.reserve(static_cast<size_t>(section_count));
    cgsize_t total_cells = 0;
    cgsize_t total_connections = 0;
    bool fixed = true;
    IGenum first_type = IG_NONE;
    int first_points_per_cell = -1;

    for (int section_index = 1; section_index <= section_count; ++section_index) {
        char section_name[100];
        ElementType_t type;
        cgsize_t start = 0;
        cgsize_t end = 0;
        int boundary = 0;
        int parent = 0;
        if (cg_section_read(index_file, index_base, index_zone, section_index,
                            section_name, &type, &start, &end, &boundary,
                            &parent) != CG_OK ||
            end < start) {
            return false;
        }

        IGenum iGameType = IG_NONE;
        int points_per_cell = 0;
        if (!MapCgnsFixedElementType(type, iGameType) ||
            cg_npe(type, &points_per_cell) != CG_OK || points_per_cell <= 0) {
            return false;
        }

        const cgsize_t cells = end - start + 1;
        if (cells > expected_cells - total_cells) return false;
        total_cells += cells;
        if (static_cast<unsigned long long>(points_per_cell) >
            (std::numeric_limits<cgsize_t>::max() - total_connections) /
                    static_cast<unsigned long long>(cells)) {
            return false;
        }
        total_connections += cells * points_per_cell;
        if (sections.empty()) {
            first_type = iGameType;
            first_points_per_cell = points_per_cell;
        } else if (iGameType != first_type ||
                   points_per_cell != first_points_per_cell) {
            fixed = false;
        }
        sections.push_back({section_index, type, cells, points_per_cell,
                            iGameType});
    }
    if (total_cells != expected_cells ||
        total_connections > static_cast<cgsize_t>(std::numeric_limits<unsigned int>::max())) {
        return false;
    }

    auto ids = IdArray::New();
    ids->Resize(static_cast<IGsize>(total_connections));
    auto id_values = ids->RawPointer();
    auto types = UnsignedIntArray::New();
    types->Resize(static_cast<IGsize>(total_cells));
    auto type_values = types->RawPointer();
    UnsignedIntArray::Pointer offsets;
    unsigned int* offset_values = nullptr;
    if (!fixed) {
        offsets = UnsignedIntArray::New();
        offsets->Resize(static_cast<IGsize>(total_cells) + 1);
        offset_values = offsets->RawPointer();
        offset_values[0] = 0;
    }

    cgsize_t cell_cursor = 0;
    cgsize_t connection_cursor = 0;
    for (const auto& section : sections) {
        std::vector<cgsize_t> elements(
                static_cast<size_t>(section.cells * section.pointsPerCell));
        if (cg_elements_read(index_file, index_base, index_zone, section.index,
                             elements.data(), nullptr) != CG_OK) {
            return false;
        }
        for (cgsize_t cell = 0; cell < section.cells; ++cell) {
            if (offset_values) {
                offset_values[cell_cursor] =
                        static_cast<unsigned int>(connection_cursor);
            }
            type_values[cell_cursor] = static_cast<unsigned int>(section.iGameType);
            for (int point = 0; point < section.pointsPerCell; ++point) {
                const cgsize_t value =
                        elements[static_cast<size_t>(cell) * section.pointsPerCell + point];
                if (value < 1 || value > static_cast<cgsize_t>(std::numeric_limits<igIndex>::max())) {
                    return false;
                }
                id_values[connection_cursor++] = static_cast<igIndex>(value - 1);
            }
            ++cell_cursor;
        }
    }
    if (offset_values) {
        offset_values[total_cells] = static_cast<unsigned int>(connection_cursor);
    }

    auto cells = CellArray::New();
    if (fixed) {
        cells->SetData(ids, first_points_per_cell);
    } else {
        cells->SetData(ids, offsets);
    }
    mesh->SetCells(cells, types);
    return true;
}
void iGameCGNSReader::ChangeMixElementToMyCell(std::vector<cgsize_t> elements, int num_read_elements) {
    if (m_UnstructuredMesh == nullptr) { m_UnstructuredMesh = UnstructuredMesh::New(); }
    // Process each element
    cgsize_t idx = 0;
    for (int i = 0; i < num_read_elements; i++) {
        ElementType_t elem_type = static_cast<ElementType_t>(elements[idx]);
        int num_vertices = 0;
        igIndex vhs[64];
        cg_npe(elem_type, &num_vertices);
        for (int i = 0; i < num_vertices; ++i) { vhs[i] = elements[idx + i + 1] - 1; }
        switch (elem_type) {
            case TRI_3:
                m_UnstructuredMesh->AddCell(vhs, num_vertices, IG_TRIANGLE);
                break;
            case QUAD_4:
                m_UnstructuredMesh->AddCell(vhs, num_vertices, IG_QUAD);
                break;
            case TETRA_4:
                m_UnstructuredMesh->AddCell(vhs, num_vertices, IG_TETRA);
                break;
            case HEXA_8:
                m_UnstructuredMesh->AddCell(vhs, num_vertices, IG_HEXAHEDRON);
                break;
            case PYRA_5:
                m_UnstructuredMesh->AddCell(vhs, num_vertices, IG_PYRAMID);
                break;
            case PENTA_6:
                m_UnstructuredMesh->AddCell(vhs, num_vertices, IG_PRISM);
                break;
            case NGON_n:
                m_UnstructuredMesh->AddCell(vhs, num_vertices, IG_POLYGON);
            default:
                break;
        }
        idx += num_vertices + 1;
    }
}

void iGameCGNSReader::ReadFields(int index_file, int index_base, int index_zone, int index_sol, ZoneType_t zoneType,
                                 int celldim, GridLocation_t location, cgsize_t* size) {
    int fieldCount = 0;
    int result = cg_nfields(index_file, index_base, index_zone, index_sol, &fieldCount);
    if (CG_OK != result) {
        IGAME_CORE_ERROR("Get Nfields Error: {}", cg_get_error());
        return;
    }

    struct CgnsFieldInfo {
        int index{0};
        bool valid{false};
        DataType_t dataType{DataTypeNull};
        std::string name;
    };
    std::vector<CgnsFieldInfo> fields;
    fields.reserve(static_cast<size_t>(fieldCount));
    for (int fieldIndex = 1; fieldIndex <= fieldCount; ++fieldIndex) {
        DataType_t dataType;
        char fieldName[100]{};
        result = cg_field_info(index_file, index_base, index_zone, index_sol,
                               fieldIndex, &dataType, fieldName);
        if (CG_OK != result) {
            IGAME_CORE_ERROR("Get Field Info Error: {}", cg_get_error());
            fields.push_back({fieldIndex, false, DataTypeNull, {}});
        } else {
            fields.push_back({fieldIndex, true, dataType, fieldName});
        }
    }

    cgsize_t rangeMin[3] = {1, 1, 1};
    cgsize_t rangeMax[3] = {0, 0, 0};
    IGsize elementCount = 1;
    if (zoneType == Structured) {
        if (location == GridLocation_t::Vertex) {
            for (int dim = 0; dim < celldim; ++dim) {
                rangeMax[dim] = size[dim];
                elementCount *= static_cast<IGsize>(size[dim]);
            }
        } else {
            for (int dim = 0; dim < celldim; ++dim) {
                rangeMax[dim] = size[dim + celldim];
                elementCount *= static_cast<IGsize>(size[dim + celldim]);
            }
        }
    } else if (location == GridLocation_t::Vertex) {
        rangeMax[0] = size[0];
        elementCount = static_cast<IGsize>(size[0]);
    } else {
        rangeMax[0] = size[1];
        elementCount = static_cast<IGsize>(size[1]);
    }

    const IGenum attachmentType =
            location == GridLocation_t::Vertex
                    ? IG_POINT
                    : location == CellCenter ? IG_CELL : IG_NONE;
    if (m_AttributeSet == nullptr) { m_AttributeSet = AttributeSet::New(); }

    const auto isSupportedFieldType = [](DataType_t type) {
        return type == Integer || type == LongInteger ||
               type == RealSingle || type == RealDouble;
    };
    const auto isVectorGroup = [&](int fieldIndex) {
        if (fieldIndex < 0 ||
            static_cast<size_t>(fieldIndex) + 2 >= fields.size()) {
            return false;
        }
        const auto& first = fields[static_cast<size_t>(fieldIndex)];
        if (!first.valid || first.name.empty() ||
            !isSupportedFieldType(first.dataType)) {
            return false;
        }
        const char suffix = first.name.back();
        if (suffix != 'X' && suffix != '0' && suffix != '1') return false;
        for (int axis = 1; axis < 3; ++axis) {
            const auto& component =
                    fields[static_cast<size_t>(fieldIndex + axis)];
            if (!component.valid || component.name.empty() ||
                component.name.back() != suffix + axis ||
                !isSupportedFieldType(component.dataType)) {
                return false;
            }
        }
        return true;
    };

    std::vector<double> componentScratch;
    for (int fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
        const auto& field = fields[static_cast<size_t>(fieldIndex)];
        if (!field.valid) continue;

        if (isVectorGroup(fieldIndex)) {
            if (elementCount <= std::numeric_limits<IGsize>::max() / 3) {
                try {
                    componentScratch.resize(
                            static_cast<size_t>(elementCount * 3));
                } catch (const std::bad_alloc&) {
                    componentScratch.clear();
                }
            } else {
                componentScratch.clear();
            }
        }
        if (isVectorGroup(fieldIndex) && componentScratch.size() ==
                                               static_cast<size_t>(elementCount) * 3) {
            bool readSucceeded = true;
            for (int axis = 0; axis < 3; ++axis) {
                const auto& component =
                        fields[static_cast<size_t>(fieldIndex + axis)];
                double* destination =
                        componentScratch.data() +
                        static_cast<size_t>(axis) * elementCount;
                result = cg_field_read(
                        index_file, index_base, index_zone, index_sol,
                        component.name.c_str(), RealDouble, rangeMin, rangeMax,
                        destination);
                if (result != CG_OK) {
                    IGAME_CORE_ERROR("Get Field Error: {}", cg_get_error());
                    readSucceeded = false;
                    break;
                }
                if (m_CheckFieldInf &&
                    (component.dataType == RealSingle ||
                     component.dataType == RealDouble)) {
                    for (IGsize value = 0; value < elementCount; ++value) {
                        if (std::isinf(destination[value])) {
                            IGAME_CORE_WARN(
                                    "CGNS field '{}' contains Inf at value index {} (base {}, zone {}, solution {}, field {}); continuing read",
                                    component.name, value, index_base, index_zone,
                                    index_sol, component.index);
                            break;
                        }
                    }
                }
            }

            if (readSucceeded) {
                const bool useDoubleVector =
                        field.dataType == LongInteger ||
                        field.dataType == RealDouble ||
                        fields[static_cast<size_t>(fieldIndex + 1)].dataType == LongInteger ||
                        fields[static_cast<size_t>(fieldIndex + 1)].dataType == RealDouble ||
                        fields[static_cast<size_t>(fieldIndex + 2)].dataType == LongInteger ||
                        fields[static_cast<size_t>(fieldIndex + 2)].dataType == RealDouble;
                const bool hasUnderscorePrefix =
                        field.name.size() >= 2 &&
                        field.name[field.name.size() - 2] == '_';
                const std::string vectorName =
                        hasUnderscorePrefix
                                ? field.name.substr(0, field.name.size() - 2)
                                : field.name.substr(0, field.name.size() - 1);
                auto range = DoubleArray::New();
                range->SetDimension(2);
                range->Resize(4);
                auto* rangeValues = range->RawPointer();

                const auto fillVector = [&](auto* output) {
                    const double* x = componentScratch.data();
                    const double* y = x + elementCount;
                    const double* z = y + elementCount;
                    double minX = std::numeric_limits<double>::max();
                    double maxX = -std::numeric_limits<double>::max();
                    double minY = std::numeric_limits<double>::max();
                    double maxY = -std::numeric_limits<double>::max();
                    double minZ = std::numeric_limits<double>::max();
                    double maxZ = -std::numeric_limits<double>::max();
                    double minMagnitude = std::numeric_limits<double>::max();
                    double maxMagnitude = 0.0;
                    for (IGsize value = 0; value < elementCount; ++value) {
                        const double xValue = x[value];
                        const double yValue = y[value];
                        const double zValue = z[value];
                        output[3 * value + 0] = static_cast<std::remove_pointer_t<decltype(output)>>(xValue);
                        output[3 * value + 1] = static_cast<std::remove_pointer_t<decltype(output)>>(yValue);
                        output[3 * value + 2] = static_cast<std::remove_pointer_t<decltype(output)>>(zValue);
                        minX = std::min(minX, xValue);
                        maxX = std::max(maxX, xValue);
                        minY = std::min(minY, yValue);
                        maxY = std::max(maxY, yValue);
                        minZ = std::min(minZ, zValue);
                        maxZ = std::max(maxZ, zValue);
                        const double magnitude =
                                std::sqrt(xValue * xValue +
                                          yValue * yValue +
                                          zValue * zValue);
                        minMagnitude = std::min(minMagnitude, magnitude);
                        maxMagnitude = std::max(maxMagnitude, magnitude);
                    }
                    rangeValues[0] = minMagnitude;
                    rangeValues[1] = maxMagnitude;
                    rangeValues[2] = minX;
                    rangeValues[3] = maxX;
                    rangeValues[4] = minY;
                    rangeValues[5] = maxY;
                    rangeValues[6] = minZ;
                    rangeValues[7] = maxZ;
                };

                ArrayObject::Pointer vector;
                if (useDoubleVector) {
                    auto doubleVector = DoubleArray::New();
                    doubleVector->SetName(vectorName);
                    doubleVector->SetDimension(3);
                    doubleVector->Resize(elementCount);
                    fillVector(doubleVector->RawPointer());
                    vector = doubleVector;
                } else {
                    auto floatVector = FloatArray::New();
                    floatVector->SetName(vectorName);
                    floatVector->SetDimension(3);
                    floatVector->Resize(elementCount);
                    fillVector(floatVector->RawPointer());
                    vector = floatVector;
                }
                if (attachmentType != IG_NONE) {
                    m_AttributeSet->AddAttribute(
                            IG_VECTOR, attachmentType, vector, range);
                }
                fieldIndex += 2;
                continue;
            }
        }

        ArrayObject::Pointer solutionArray;
        void* fieldBuffer = nullptr;
        switch (field.dataType) {
            case Integer: {
                auto array = IntArray::New();
                array->Resize(elementCount);
                fieldBuffer = array->RawPointer();
                solutionArray = array;
                break;
            }
            case LongInteger: {
                auto array = LongLongArray::New();
                array->Resize(elementCount);
                fieldBuffer = array->RawPointer();
                solutionArray = array;
                break;
            }
            case RealSingle: {
                auto array = FloatArray::New();
                array->Resize(elementCount);
                fieldBuffer = array->RawPointer();
                solutionArray = array;
                break;
            }
            case RealDouble: {
                auto array = DoubleArray::New();
                array->Resize(elementCount);
                fieldBuffer = array->RawPointer();
                solutionArray = array;
                break;
            }
            default:
                break;
        }
        if (!solutionArray || !fieldBuffer) continue;
        result = cg_field_read(
                index_file, index_base, index_zone, index_sol,
                field.name.c_str(), field.dataType, rangeMin, rangeMax,
                fieldBuffer);
        if (result != CG_OK) {
            IGAME_CORE_ERROR("Get Field Error: {}", cg_get_error());
            continue;
        }
        if (m_CheckFieldInf) {
            if (field.dataType == RealSingle) {
                ReportCgnsFieldInf(
                        field.name.c_str(), index_base, index_zone, index_sol,
                        field.index, DynamicCast<FloatArray>(solutionArray));
            } else if (field.dataType == RealDouble) {
                ReportCgnsFieldInf(
                        field.name.c_str(), index_base, index_zone, index_sol,
                        field.index, DynamicCast<DoubleArray>(solutionArray));
            }
        }
        solutionArray->SetName(field.name);
        if (attachmentType != IG_NONE) {
            m_AttributeSet->AddAttribute(
                    IG_SCALAR, attachmentType, solutionArray);
        }
    }
}


IGAME_NAMESPACE_END
#endif
