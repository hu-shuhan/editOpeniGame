#if defined(CGNS_ENABLE)
#include "iGameCGNSReader.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
IGAME_NAMESPACE_BEGIN
iGameCGNSReader::iGameCGNSReader() {
    this->SetNumberOfOutputs(1);
}
iGameCGNSReader::~iGameCGNSReader() {}

void CollectNodeIDs(double parent_id, double node_id, int depth, std::vector<double>& node_ids) {
    char label[CGIO_MAX_NAME_LENGTH + 1];
    int numChildren;
    // Store the current node ID in the vector
    node_ids.push_back(node_id);
    // Get the label of the current node (optional, for debugging)
    cgio_get_label(parent_id, node_id, label);
    //std::cout << std::string(depth * 2, ' ') << node_id << " " << "Node Label: " << label << std::endl;
    // Get the number of children of the current node
    cgio_number_children(parent_id, node_id, &numChildren);
    if (numChildren > 0) {
        std::vector<double> child_ids(numChildren);
        cgio_children_ids(parent_id, node_id, 1, numChildren, &numChildren, child_ids.data());
        for (int i = 0; i < numChildren; ++i) { CollectNodeIDs(parent_id, child_ids[i], depth + 1, node_ids); }
    }
}
DataObject::Pointer iGameCGNSReader::ReadFile(std::string fileName) {

    int result;
    int file_type;
    result = cgio_check_file(fileName.c_str(), &file_type);
    if (file_type == CG_FILE_NONE) {
        std::cout << "Not a CGNS file." << std::endl;
        return nullptr;
    } else if (file_type == CG_FILE_ADF) {
        std::cout << "ADF (Advanced Data Format) file." << std::endl;
    } else if (file_type == CG_FILE_HDF5) {
        std::cout << "HDF5 (Hierarchical Data Format) file." << std::endl;
    } else {
        std::cout << "Unknown file type." << std::endl;
        return nullptr;
    }
    int index_file;
    result = cg_open(fileName.data(), CG_MODE_READ, &index_file);
    if (CG_OK != result) {
        std::cout << "Open Error: " << cg_get_error() << std::endl;
    } else {
        std::cout << "Success to open cgns file!" << std::endl;
    }

    int cgio_num;
    result = cg_get_cgio(index_file, &cgio_num);
    if (CG_OK != result) { std::cout << "Get cgio num Error: " << cg_get_error() << std::endl; }
    // Get the root node ID
    double root_id;
    cgio_get_root_id(cgio_num, &root_id);
    // Vector to store all node IDs
    std::vector<double> node_ids;
    // Collect all node IDs starting from the root node
    CollectNodeIDs(cgio_num, root_id, 0, node_ids);

    for (int i = 0; i < node_ids.size(); i++) {
        char label[256];
        cgio_get_label(cgio_num, node_ids[i], label);
        if (strcmp(label, "ZoneBC_t") != 0) { continue; }
        double zoneId = node_ids[i];
        std::vector<double> child_node_ids;
        CollectNodeIDs(cgio_num, zoneId, 0, child_node_ids);
        for (int j = 0; j < child_node_ids.size(); j++) {
            cgio_get_label(cgio_num, child_node_ids[j], label);
            if (strcmp(label, "BC_t") != 0) { continue; }
            auto bcnode = child_node_ids[j];
            char name[256];
            char dataType[256];
            cgio_get_name(cgio_num, bcnode, name);
            cgio_get_data_type(cgio_num, bcnode, dataType);
            BoundryNames.insert(name);
            continue;
        }
    }

    int nbases;
    result = cg_nbases(index_file, &nbases);
    if (CG_OK != result) {
        std::cout << "Get nbases Error: " << cg_get_error() << std::endl;
    } else {
        // std::cout << "Base Num = " << nbases << std::endl;
    }
    m_ParentObject = DrawObject::New();
    for (int index_base = 1; index_base <= nbases; index_base++) {
        char basename[100];
        int celldim, physdim;
        result = cg_base_read(index_file, index_base, basename, &celldim, &physdim);
        if (CG_OK != result) {
            std::cout << "Read Base: " << cg_get_error() << std::endl;
        } else {
            // std::cout << "BaseName = " << basename << std::endl;
            // std::cout << "Dimension of the cells = " << celldim << std::endl;
            // std::cout << "Number of coordinates required to define a vector in the field is " << physdim << std::endl;
        }
        int nzones;
        result = cg_nzones(index_file, index_base, &nzones);
        if (CG_OK != result) {
            std::cout << "Get nzones Error: " << cg_get_error() << std::endl;
        } else {
            std::cout << "Zone Num of Base(" << index_base << ") = " << nzones << std::endl;
            for (int index_zone = 1; index_zone <= nzones; index_zone++) {
                DataObject::Pointer DataSet = DataObject::New();
                ZoneType_t zoneType;
                result = cg_zone_type(index_file, index_base, index_zone, &zoneType);
                if (CG_OK != result) { std::cout << "Get Zone Type Error: " << cg_get_error() << std::endl; }
                cgsize_t size[9] = {0};
                char zonename[100];
                result = cg_zone_read(index_file, index_base, index_zone, zonename, size);
                if (CG_OK != result) {
                    std::cout << "Zone Read Error: " << cg_get_error() << std::endl;
                } else {
                    std::cout << "ZoneName = " << zonename << std::endl;
                    if (zoneType == Structured) {
                        std::cout << "ZoneType = Structured" << std::endl;
                        if (m_StructuredMesh == nullptr) { m_StructuredMesh = StructuredMesh::New(); }
                        this->m_DataObjectType = IG_STRUCTURED_MESH;
                        // Calculate total points and cells for structured zones
                        cgsize_t totalPoints = 1, totalCells = 1;
                        for (int i = 0; i < celldim; ++i) {
                            totalPoints *= size[i];
                            if (size[celldim + i] == 0) { break; }
                            totalCells *= size[celldim + i];
                        }
                        std::cout << "Total Points: " << totalPoints << std::endl;
                        std::cout << "Total Cells: " << totalCells << std::endl;
                        // Read vertices coordinates
                        this->ReadPointCoordinates(totalPoints, physdim, index_file, index_base, index_zone, size);
                        // Gen cells (connectivity)
                        this->GenStructuredCellConnectivities(celldim, size);
                    } else if (zoneType == Unstructured) {
                        std::cout << "ZoneType = Unstructured" << std::endl;
                        std::cout << "Total Points: " << size[0] << std::endl;
                        std::cout << "Total Cells: " << size[1] << std::endl;
                        // Read vertices coordinates
                        this->ReadPointCoordinates(size[0], physdim, index_file, index_base, index_zone, size);
                        // Read cells (connectivity)
                        this->ReadUnstructuredCellConnectivities(index_file, index_base, index_zone, size[1]);
                    } else {
                        std::cout << "Unknown ZoneType!" << std::endl;
                        this->m_DataObjectType = IG_NONE;
                    }
                    /* Flow Solution */
                    int nsols;
                    result = cg_nsols(index_file, index_base, index_zone, &nsols);
                    if (CG_OK != result) {
                        std::cout << "Get Num Of Solution Error: " << cg_get_error() << std::endl;
                    } else {
                        std::cout << "Solution Num = " << nsols << std::endl;
                    }
                    for (int index_sol = 1; index_sol <= nsols; index_sol++) {
                        char solname[100];
                        GridLocation_t location;
                        result = cg_sol_info(index_file, index_base, index_zone, index_sol, solname, &location);
                        if (CG_OK != result) {
                            std::cout << "Read Solution Info Error: " << cg_get_error() << std::endl;
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
                                std::cout << "Solution Location is bad data! Unknown!" << std::endl;
                        }
                        this->ReadFields(index_file, index_base, index_zone, index_sol, zoneType, celldim, location,
                                         size);
                    }
                    if (this->m_AttributeSet) {
                        this->GetCurrentDataObject()->SetAttributeSet(this->m_AttributeSet);
                    }
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
    return GetOutput();
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
            std::cout << "Get coordinate info failed for dim " << dim
                << ": " << cg_get_error() << std::endl;
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
            std::cout << "Read " << coordName << " Error: " << cg_get_error() << std::endl;
            continue;
        }

        int idx = dim;
        for (cgsize_t j = 0; j < pointNum; ++j) {
            points[idx] = coordData[j];
            idx += positionDim;
        }
    }
}


void iGameCGNSReader::GenStructuredCellConnectivities(cgsize_t cellDim, cgsize_t* size) {
    if (m_StructuredMesh == nullptr) { m_StructuredMesh = StructuredMesh::New(); }
    auto structType = QUAD_4;
    if (cellDim == 3 && size[2] > 1) { structType = HEXA_8; }
    m_StructuredMesh->SetPoints(m_Points);
    igIndex tmpSize[3] = {0};
    for (int i = 0; i < cellDim; i++) { tmpSize[i] = size[i]; }
    m_StructuredMesh->SetDimensionSize(tmpSize);
    m_StructuredMesh->GenStructuredCellConnectivities();
}
void iGameCGNSReader::ReadUnstructuredCellConnectivities(int index_file, int index_base, int index_zone,
                                                         cgsize_t cellNum) {
    //这边需要读取出邻接关系然后转换为我们的type
    int nsections;
    int result = cg_nsections(index_file, index_base, index_zone, &nsections);
    if (CG_OK != result) {
        std::cout << "Get Num Of Sections Error: " << cg_get_error() << std::endl;
        return;
    }
    //paraview只读取了部分的section，应该是非结构化网格只有一个section
    //nsections = 1;
    //igDebug("the number of sections is " << nsections);
    int cgio_num;
    if (cg_get_cgio(index_file, &cgio_num) != CG_OK) {
        std::cout << "Get cgio num Error: " << cg_get_error() << std::endl;
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
            CellArray::Pointer Faces = CellArray::New();
            CellArray::Pointer Volumes = CellArray::New();
            for (int index_section = 1; index_section <= nsections; index_section++) {
                char sectionname[100];
                ElementType_t type;
                cgsize_t start, end;
                int nbndry, parent_flag;
                cg_section_read(index_file, index_base, index_zone, index_section, sectionname, &type, &start, &end,
                                &nbndry, &parent_flag);
                cgsize_t num_elements = end - start + 1;
                //std::cout << "section " << index_section << " " << sectionname << ' ' << start << ' ' << end << " "
                //          << num_elements << '\n';
                bool is_boundry = BoundryNames.count(sectionname);
                bool is_zone =  std::strncmp(sectionname, "zone_", 5) == 0;
                if (type == NGON_n) {
                    cgsize_t elementDataSize;
                    cg_ElementDataSize(index_file, index_base, index_zone, index_section, &elementDataSize);
                    std::vector<cgsize_t> elements;
                    elements.resize(elementDataSize);
                    std::vector<cgsize_t> offset;
                    offset.resize(num_elements + 1);
                    auto elementsData = elements.data();
                    cg_poly_elements_read(index_file, index_base, index_zone, index_section, elementsData,
                                          offset.data(), NULL);
                    cgsize_t cellVcnt = 0;
                    cgsize_t idx = 0;
                    igIndex vhs[IGAME_CELL_MAX_SIZE];
                    if (offset[0] != offset[1]) {
                        for (cgsize_t i = 0; i < num_elements; i++) {
                            cellVcnt = offset[i + 1] - offset[i];
                            for (int j = 0; j < cellVcnt; j++) { vhs[j] = elements[idx++] - 1; }
                            Faces->AddCellIds(vhs, cellVcnt);
                        }
                    } else {
                        for (cgsize_t i = 0; i < num_elements; i++) {
                            cellVcnt = elements[idx++];
                            for (int j = 0; j < cellVcnt; j++) { vhs[j] = elements[idx++] - 1; }
                            Faces->AddCellIds(vhs, cellVcnt);
                        }
                    }
                    std::vector<cgsize_t> temp;
                    elements.swap(temp);
                    std::vector<cgsize_t> temp_1;
                    offset.swap(temp_1);
                } else if (type == NFACE_n) {
                    cgsize_t elementDataSize;
                    cg_ElementDataSize(index_file, index_base, index_zone, index_section, &elementDataSize);
                    std::vector<cgsize_t> elements;
                    elements.resize(elementDataSize);
                    std::vector<cgsize_t> offset;
                    offset.resize(num_elements + 1);
                    auto elementsData = elements.data();
                    cg_poly_elements_read(index_file, index_base, index_zone, index_section, elementsData,
                                          offset.data(), NULL);
                    cgsize_t cellFcnt = 0;
                    cgsize_t idx = 0;
                    igIndex vhs[IGAME_CELL_MAX_SIZE];
                    igIndex fhs[IGAME_CELL_MAX_SIZE];
                    if (offset[0] != offset[1]) {
                        for (cgsize_t i = 0; i < num_elements; i++) {
                            cellFcnt = offset[i + 1] - offset[i];
                            for (int j = 0; j < cellFcnt; j++) { fhs[j] = std::abs(elements[idx++]) - 1; }
                            Volumes->AddCellIds(fhs, cellFcnt);
                        }
                    } else {
                        for (int i = 0; i < num_elements; i++) {
                            cellFcnt = elements[idx++];
                            for (int j = 0; j < cellFcnt; j++) { fhs[j] = std::abs(elements[idx++]) - 1; }
                            Volumes->AddCellIds(fhs, cellFcnt);
                        }
                    }
                    std::vector<cgsize_t> temp;
                    elements.swap(temp);
                    std::vector<cgsize_t> temp_1;
                    offset.swap(temp_1);
                }
            }
            this->m_VolumeMesh->InitVolumesWithPolyhedron(Faces, Volumes);
        }
    } else {
        if (m_UnstructuredMesh == nullptr) { m_UnstructuredMesh = UnstructuredMesh::New(); }
        m_UnstructuredMesh->SetPoints(m_Points);
        this->m_DataObjectType = IG_UNSTRUCTURED_MESH;
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
    int nfileds;
    int result = cg_nfields(index_file, index_base, index_zone, index_sol, &nfileds);
    if (CG_OK != result) {
        std::cout << "Get Nfields Error: " << cg_get_error() << std::endl;
    } else {
        std::cout << "nfields = " << nfileds << std::endl;
        for (int index_field = 1; index_field <= nfileds; index_field++) {
            DataType_t dataType;
            char fieldname[100];
            result = cg_field_info(index_file, index_base, index_zone, index_sol, index_field, &dataType, fieldname);
            if (CG_OK != result) {
                std::cout << "Get Field Info Error: " << cg_get_error() << std::endl;
                continue;
            } else {
                //std::cout << "FieldName = " << fieldname << std::endl;
                //std::cout << "DataType_t(Integer, LongInteger, RealSingle, and RealDouble) = " << dataType << std::endl;
                cgsize_t range_min[3] = {1, 1, 1};
                cgsize_t range_max[3];
                int arrayNum = 1;
                if (zoneType == Structured) {
                    if (location == GridLocation_t::Vertex) {
                        for (int i = 0; i < celldim; i++) {
                            range_max[i] = size[i];
                            if (size[i]) arrayNum *= size[i];
                        }
                    } else {
                        for (int i = 0; i < celldim; i++) {
                            range_max[i] = size[i + celldim];
                            if (size[i + celldim]) arrayNum *= size[i + celldim];
                        }
                    }
                } else {
                    if (location == GridLocation_t::Vertex) {
                        range_max[0] = size[0];
                        arrayNum = size[0];
                    } else {
                        range_max[0] = size[1];
                        arrayNum = size[1];
                    }
                }

                ArrayObject::Pointer solutionArray;
                switch (dataType) {
                    case Integer: {
                        auto solutionArray_2 = IntArray::New();
                        solutionArray_2->Resize(arrayNum);
                        result = cg_field_read(index_file, index_base, index_zone, index_sol, fieldname, dataType,
                                               range_min, range_max, solutionArray_2->RawPointer());
                        solutionArray = solutionArray_2;
                        break;
                    }

                    case LongInteger: {
                        auto solutionArray_2 = LongLongArray::New();
                        solutionArray_2->Resize(arrayNum);
                        result = cg_field_read(index_file, index_base, index_zone, index_sol, fieldname, dataType,
                                               range_min, range_max, solutionArray_2->RawPointer());
                        solutionArray = solutionArray_2;
                        break;
                    }
                    case RealSingle: {
                        auto solutionArray_2 = FloatArray::New();
                        solutionArray_2->Resize(arrayNum);
                        result = cg_field_read(index_file, index_base, index_zone, index_sol, fieldname, dataType,
                                               range_min, range_max, solutionArray_2->RawPointer());
                        solutionArray = solutionArray_2;
                        break;
                    }
                    case RealDouble: {
                        auto solutionArray_2 = DoubleArray::New();
                        solutionArray_2->Resize(arrayNum);
                        result = cg_field_read(index_file, index_base, index_zone, index_sol, fieldname, dataType,
                                               range_min, range_max, solutionArray_2->RawPointer());
                        solutionArray = solutionArray_2;
                        break;
                    }
                    default:
                        break;
                }
                if (CG_OK != result) {
                    std::cout << "Get Field Error: " << cg_get_error() << std::endl;
                } else {
                    //存储
                    solutionArray->SetName(fieldname);
                    if (m_AttributeSet == nullptr) { m_AttributeSet = AttributeSet::New(); }
                    if (location == GridLocation_t::Vertex) {
                        m_AttributeSet->AddAttribute(IG_SCALAR, IG_POINT, solutionArray);
                    } else if (location == CellCenter) {
                        m_AttributeSet->AddAttribute(IG_SCALAR, IG_CELL, solutionArray);
                    }
                }
            }
        }
    }
}


IGAME_NAMESPACE_END
#endif