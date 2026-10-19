#include "iGameVTKAbstractReader.h"
#include "iGameByteSwap.h"
#include <cstring>
#include <vector>
IGAME_NAMESPACE_BEGIN

namespace {
bool IsVtkCellSizeValid(VTKAbstractReader::VTKTYPE type, int size) {
    switch (type) {
        case VTKAbstractReader::VERTEX:
            return size == 1;
        case VTKAbstractReader::LINE:
            return size == 2;
        case VTKAbstractReader::POLYLINE:
            return size >= 2;
        case VTKAbstractReader::TRIANGLE:
            return size == 3;
        case VTKAbstractReader::POLYGON:
            return size >= 3;
        case VTKAbstractReader::PIXEL:
        case VTKAbstractReader::QUAD:
        case VTKAbstractReader::TETRA:
            return size == 4;
        case VTKAbstractReader::VOXEL:
        case VTKAbstractReader::HEXAHEDRON:
            return size == 8;
        case VTKAbstractReader::WEDGE:
            return size == 6;
        case VTKAbstractReader::PYRAMID:
            return size == 5;
        case VTKAbstractReader::PENTAGONAL_PRISM:
            return size == 10;
        case VTKAbstractReader::HEXAGONAL_PRISM:
            return size == 12;
        case VTKAbstractReader::QUADRATIC_EDGE:
            return size == 3;
        case VTKAbstractReader::QUADRATIC_TRIANGLE:
            return size == 6;
        case VTKAbstractReader::QUADRATIC_QUAD:
            return size == 8;
        case VTKAbstractReader::QUADRATIC_TETRA:
            return size == 10;
        case VTKAbstractReader::QUADRATIC_HEXAHEDRON:
            return size == 20;
        case VTKAbstractReader::QUADRATIC_WEDGE:
            return size == 15;
        case VTKAbstractReader::QUADRATIC_PYRAMID:
            return size == 13;
        default:
            // Polyhedra and Lagrange cells have variable connectivity sizes.
            return true;
    }
}

VTKAbstractReader::VTKTYPE NormalizeVtkCellType(VTKAbstractReader::VTKTYPE type, int size) {
    // Some third-party writers keep all midside nodes but incorrectly label
    // the cell with its linear VTK type.  ParaView tolerates these files.  The
    // node counts below identify the corresponding quadratic type without
    // weakening validation for arbitrary malformed connectivity.
    switch (type) {
        case VTKAbstractReader::LINE:
            if (size == 3) return VTKAbstractReader::QUADRATIC_EDGE;
            break;
        case VTKAbstractReader::TRIANGLE:
            if (size == 6) return VTKAbstractReader::QUADRATIC_TRIANGLE;
            break;
        case VTKAbstractReader::QUAD:
            if (size == 8) return VTKAbstractReader::QUADRATIC_QUAD;
            break;
        case VTKAbstractReader::TETRA:
            if (size == 10) return VTKAbstractReader::QUADRATIC_TETRA;
            break;
        case VTKAbstractReader::HEXAHEDRON:
            if (size == 20) return VTKAbstractReader::QUADRATIC_HEXAHEDRON;
            break;
        case VTKAbstractReader::WEDGE:
            if (size == 15) return VTKAbstractReader::QUADRATIC_WEDGE;
            break;
        case VTKAbstractReader::PYRAMID:
            if (size == 13) return VTKAbstractReader::QUADRATIC_PYRAMID;
            break;
        default:
            break;
    }
    return type;
}
} // namespace

VTKAbstractReader::VTKAbstractReader() {
    this->Header = nullptr;
    this->FileMajorVersion = 0;
    this->FileMinorVersion = 0;
    this->FileVersion = 0;
}
VTKAbstractReader::~VTKAbstractReader() {
    if (this->Header) delete[] this->Header;
}

int VTKAbstractReader::ReadHeader() {
    char line[256];
    igDebug("Reading vtk file header");
    //
    // read header
    //
    if (!this->ReadLine(line)) {
        igError("Premature EOF reading first line! ");
        return 0;
    }
    const int VERSION_PREFIX_LENGTH = 22;
    if (strncmp("# vtk DataFile Version", line, VERSION_PREFIX_LENGTH) != 0) {
        igError("Unrecognized file type! ");
        return 0;
    }
    if (sscanf(line + VERSION_PREFIX_LENGTH, "%d.%d", &this->FileMajorVersion, &this->FileMinorVersion) != 2) {
        igDebug("Cannot read file version:!");
        this->FileMajorVersion = 0;
        this->FileMinorVersion = 0;
    }
    if (this->FileMajorVersion > vtkLegacyReaderMajorVersion ||
        (this->FileMajorVersion == vtkLegacyReaderMajorVersion &&
         this->FileMinorVersion > vtkLegacyReaderMinorVersion)) {
        // newer file than the reader version
        igDebug("Reading file version: {}.{} with older reader version {}.{}", this->FileMajorVersion,
                this->FileMinorVersion, vtkLegacyReaderMajorVersion, vtkLegacyReaderMinorVersion);
    }
    // Compose FileVersion
    this->FileVersion = 10 * this->FileMajorVersion + this->FileMinorVersion;

    //
    // read title
    //
    if (!this->ReadLine(line)) {
        igError("Premature EOF reading title! ");
        return 0;
    }
    //delete[] this->Header;
    this->Header = new char[strlen(line) + 1];
    strcpy(this->Header, line);

    igDebug("Reading vtk file entitled!{}", line);
    //
    // read type
    //
    if (!this->ReadString(line)) {
        //igError("Premature EOF reading file type!"
        //	<< " for file: " << (fname ? fname : "(Null FileName)"));
        return 0;
    }

    if (!strncmp(this->LowerCase(line), "ascii", 5)) {
        m_FileType = IGAME_ASCII;
    } else if (!strncmp(line, "binary", 6)) {
        m_FileType = IGAME_BINARY;
    } else {
        //igError("Unrecognized file type: " << line
        //	<< " for file: " << (fname ? fname : "(Null FileName)"));
        m_FileType = 0;
        return 0;
    }
    return 1;
}


//------------------------------------------------------------------------------
// Read point coordinates. Return 0 if error.
int VTKAbstractReader::ReadPointCoordinates(Points::Pointer points, int PointsNum) {
    char line[256];
    FloatArray::Pointer floatData;
    DoubleArray::Pointer doubleData;
    if (!this->ReadString(line)) {
        //igError("Cannot read points type!"
        //	<< " for file: " << (fname ? fname : "(Null FileName)"));
        return 0;
    }
    if (!strcmp(this->LowerCase(line), "double")) {
        doubleData = DynamicCast<DoubleArray>(this->ReadArray("double", PointsNum, 3));
    } else {
        floatData = DynamicCast<FloatArray>(this->ReadArray("float", PointsNum, 3));
    }
    if (floatData != nullptr) {
        //points->SetArray(data->RawPointer(), PointsNum);
        float p[3]{};
        for (int i = 0; i < PointsNum; i++) {
            floatData->GetElement(i, p);
            points->AddPoint(p);
        }
    } else if (doubleData != nullptr) {
        float p[3]{};
        for (int i = 0; i < PointsNum; i++) {
            doubleData->GetElement(i, p);
            points->AddPoint(p);
        }
    } else {
        return 0;
    }
    igDebug("Read {} points", points->GetNumberOfPoints());
    this->UpdateReadProgress();
    return 1;
}

//------------------------------------------------------------------------------
// Read the cell data of a vtk data file. The number of cells (from the
// dataset) must match the number of cells defined in cell attributes (unless
// no geometry was defined).
int VTKAbstractReader::ReadCellData(int CellsNum) {
    char line[256];
    this->DataType = CELL_TYPE;
    igDebug("Reading vtk cell data");
    int unrecognized = 0;
    //
    // Read keywords until end-of-file
    //
    while (this->ReadString(line)) {
        if (line[0] == '\0') { continue; }
        //
        // read scalar data
        //
        if (!strncmp(this->LowerCase(line), "scalars", 7)) {
            if (!this->ReadScalarData(CellsNum)) { return 0; }
        }
        //
        // read vector data
        //
        else if (!strncmp(line, "vectors", 7)) {
            if (!this->ReadVectorData(CellsNum)) { return 0; }
        }
        //
        // read 3x2 symmetric tensor data
        //
        else if (!strncmp(line, "tensors6", 8)) {
            if (!this->ReadTensorData(CellsNum, 6)) { return 0; }
        }
        //
        // read 3x3 tensor data
        //
        else if (!strncmp(line, "tensors", 7)) {
            if (!this->ReadTensorData(CellsNum)) { return 0; }
        }
        //
        // read normals data
        //
        else if (!strncmp(line, "normals", 7)) {
            if (!this->ReadNormalData(CellsNum)) { return 0; }
        }
        //
        // read texture coordinates data
        //
        else if (!strncmp(line, "texture_coordinates", 19)) {
            if (!this->ReadTCoordsData(CellsNum)) { return 0; }
        }
        //
        // read the global id data
        //
        else if (!strncmp(line, "global_ids", 10)) {
            if (!this->ReadGlobalIds(CellsNum)) { return 0; }
        }
        //
        // read the pedigree id data
        //
        else if (!strncmp(line, "pedigree_ids", 12)) {
            if (!this->ReadPedigreeIds(CellsNum)) { return 0; }
        }
        //
        // read color scalars data
        //
        else if (!strncmp(line, "color_scalars", 13)) {
            if (!this->ReadCoScalarData(CellsNum)) { return 0; }
        }
        //
        // read lookup table. Associate with scalar data.
        //
        else if (!strncmp(line, "lookup_table", 12)) {
            if (!this->ReadLutData()) { return 0; }
        }
        //
        // read field of data
        //
        else if (!strncmp(line, "field", 5)) {
            this->ReadFieldData();
        }
        //
        // maybe bumped into point data
        //
        else if (!strncmp(line, "point_data", 10)) {
            int npts;
            if (!this->Read(&npts)) {
                //igError("Cannot read point data!");
                return 0;
            }
            this->ReadPointData(npts);
            break;
        } else if (!strncmp(line, "metadata", 8)) {
            if (!ProcessMetaData()) { igDebug("Read metadata error!"); }
            continue;
        } else {
            //igError("Unsupported cell attribute type: " << line
            //	<< " for file: " << (fname ? fname : "(Null FileName)"));
            // 不认识的关键字：不再整体放弃。历史上这里直接 return 0，而上层
            // 忽略返回值，会让同一个文件里后面的 POINT_DATA 整段静默丢失。
            // 这里跳过当前行继续扫描；跳过长度和次数都有上限，避免在错位的
            // 二进制内容里空转。
            if (++unrecognized > 64) {
                igError("Too many unrecognized keywords in vtk cell data!");
                return 0;
            }
            igDebug("Unsupported vtk cell attribute keyword: {}", line);
            int scanned = 0;
            while (this->IS < this->FILEEND && *this->IS != '\n' &&
                   scanned < 4096) {
                ++this->IS;
                ++scanned;
            }
            if (this->IS < this->FILEEND && *this->IS == '\n') { ++this->IS; }
        }
    }
    this->UpdateReadProgress();
    return 1;
}


//------------------------------------------------------------------------------
// Read the point data of a vtk data file. The number of points (from the
// dataset) must match the number of points defined in point attributes (unless
// no geometry was defined).
int VTKAbstractReader::ReadPointData(int PointsNum) {
    char line[256];
    AttributeSet::Pointer PointData = m_Data.GetData();

    this->DataType = POINT_TYPE;
    igDebug("Reading vtk point data");
    int unrecognized = 0;
    //
    // Read keywords until end-of-file
    //
    while (this->ReadString(line)) {
        if (line[0] == '\0') { continue; }
        //
        // read scalar data
        //
        if (!strncmp(this->LowerCase(line), "scalars", 7)) {
            if (!this->ReadScalarData(PointsNum)) { return 0; }
        }
        //
        // read vector data
        //
        else if (!strncmp(line, "vectors", 7)) {
            if (!this->ReadVectorData(PointsNum)) { return 0; }
        }
        //
        // read 3x2 symmetric tensor data
        //
        else if (!strncmp(line, "tensors6", 8)) {
            if (!this->ReadTensorData(PointsNum, 6)) { return 0; }
        }
        //
        // read 3x3 tensor data
        //
        else if (!strncmp(line, "tensors", 7)) {
            if (!this->ReadTensorData(PointsNum)) { return 0; }
        }
        //
        // read normals data
        //
        else if (!strncmp(line, "normals", 7)) {
            if (!this->ReadNormalData(PointsNum)) { return 0; }
        }
        //
        // read texture coordinates data
        //
        else if (!strncmp(line, "texture_coordinates", 19)) {
            if (!this->ReadTCoordsData(PointsNum)) { return 0; }
        }
        //
        // read the global id data
        //
        else if (!strncmp(line, "global_ids", 10)) {
            if (!this->ReadGlobalIds(PointsNum)) { return 0; }
        }
        //
        // read the pedigree id data
        //
        else if (!strncmp(line, "pedigree_ids", 12)) {
            if (!this->ReadPedigreeIds(PointsNum)) { return 0; }
        }
        //
        // read the edge flags data
        //
        else if (!strncmp(line, "edge_flags", 10)) {
            if (!this->ReadEdgeFlags(PointsNum)) { return 0; }
        }
        //
        // read color scalars data
        //
        else if (!strncmp(line, "color_scalars", 13)) {
            if (!this->ReadCoScalarData(PointsNum)) { return 0; }
        }
        //
        // read lookup table. Associate with scalar data.
        //
        else if (!strncmp(line, "lookup_table", 12)) {
            if (!this->ReadLutData()) { return 0; }
        }
        //
        // read field of data
        //
        else if (!strncmp(line, "field", 5)) {
            this->ReadFieldData();
        }
        //
        // maybe bumped into cell data
        //
        else if (!strncmp(line, "cell_data", 9)) {
            int ncells;
            if (!this->Read(&ncells)) {
                igError("Cannot read cell data!");
                return 0;
            }
            this->ReadCellData(ncells);
            break;
        } else if (!strncmp(this->LowerCase(line), "metadata", 8)) {
            if (!ProcessMetaData()) { igDebug("Read metadata error!"); }
            continue;
        } else {
            //igError("Unsupported point attribute type: " << line
            //	<< " for file: " << (fname ? fname : "(Null FileName)"));
            // 同 ReadCellData：跳过当前行继续扫描，而不是整体放弃
            if (++unrecognized > 64) {
                igError("Too many unrecognized keywords in vtk point data!");
                return 0;
            }
            igDebug("Unsupported vtk point attribute keyword: {}", line);
            int scanned = 0;
            while (this->IS < this->FILEEND && *this->IS != '\n' &&
                   scanned < 4096) {
                ++this->IS;
                ++scanned;
            }
            if (this->IS < this->FILEEND && *this->IS == '\n') { ++this->IS; }
        }
    }
    this->UpdateReadProgress();
    return 1;
}

//------------------------------------------------------------------------------
// Read scalar point attributes. Return 0 if error.
int VTKAbstractReader::ReadScalarData(int PointsNum) {
    char line[256], name[256], key[256], tableName[256];
    int numComp = 1;
    char buffer[256];

    if (!(this->ReadString(buffer) && this->ReadString(line))) { return 0; }
    this->DecodeString(name, buffer);

    if (!this->ReadString(key)) { return 0; }

    // the next string could be an integer number of components or a lookup table
    if (strcmp(this->LowerCase(key), "lookup_table") != 0) {
        numComp = atoi(key);
        if (numComp < 1 || !this->ReadString(key)) { return 0; }
    }

    if (strcmp(this->LowerCase(key), "lookup_table") != 0) { return 0; }

    if (!this->ReadString(tableName)) { return 0; }

    // Read the data
    auto data = this->ReadArray(line, PointsNum, numComp);
    if (data != nullptr) {
        data->SetName(name);
        if (this->DataType == POINT_TYPE) {
            m_Data.GetData()->AddScalar(IG_POINT, data);
        } else if (this->DataType == CELL_TYPE) {
            m_Data.GetData()->AddScalar(IG_CELL, data);
        }
    } else {
        return 0;
    }
    this->UpdateReadProgress();
    return 1;
}

//------------------------------------------------------------------------------
// Read vector point attributes. Return 0 if error.
int VTKAbstractReader::ReadVectorData(int PointsNum) {
    char line[256], name[256];
    ArrayObject::Pointer data;
    char buffer[256];

    if (!(this->ReadString(buffer) && this->ReadString(line))) { return 0; }
    this->DecodeString(name, buffer);


    data = this->ReadArray(line, PointsNum, 3);
    if (data != nullptr) {
        data->SetName(name);
        if (this->DataType == POINT_TYPE) {
            m_Data.GetData()->AddAttribute(IG_VECTOR, IG_POINT, data);
        } else if (this->DataType == CELL_TYPE) {
            m_Data.GetData()->AddAttribute(IG_VECTOR, IG_CELL, data);
        }
    } else {
        return 0;
    }
    this->UpdateReadProgress();

    return 1;
}

//------------------------------------------------------------------------------
// Read normal point attributes. Return 0 if error.
int VTKAbstractReader::ReadNormalData(int PointsNum) {
    char line[256], name[256];
    ArrayObject::Pointer data;
    char buffer[256];

    if (!(this->ReadString(buffer) && this->ReadString(line))) { return 0; }
    this->DecodeString(name, buffer);

    data = this->ReadArray(line, PointsNum, 3);
    if (data != nullptr) {
        data->SetName(name);
        if (this->DataType == POINT_TYPE) {
            m_Data.GetData()->AddAttribute(IG_NORMAL, IG_POINT, data);
        } else if (this->DataType == CELL_TYPE) {
            m_Data.GetData()->AddAttribute(IG_NORMAL, IG_CELL, data);
        }
    } else {
        return 0;
    }

    this->UpdateReadProgress();
    return 1;
}

//------------------------------------------------------------------------------
// Read tensor point attributes. Return 0 if error.
int VTKAbstractReader::ReadTensorData(int PointsNum, int numComp) {
    char line[256], name[256];
    ArrayObject::Pointer data;
    char buffer[256];

    if (!(this->ReadString(buffer) && this->ReadString(line))) { return 0; }
    this->DecodeString(name, buffer);

    data = this->ReadArray(line, PointsNum, numComp);
    if (data != nullptr) {
        data->SetName(name);
        if (this->DataType == POINT_TYPE) {
            m_Data.GetData()->AddAttribute(IG_TENSOR, IG_POINT, data);
        } else if (this->DataType == CELL_TYPE) {
            m_Data.GetData()->AddAttribute(IG_TENSOR, IG_CELL, data);
        }
    } else {
        return 0;
    }
    this->UpdateReadProgress();
    return 1;
}

//------------------------------------------------------------------------------
// Read color scalar point attributes. Return 0 if error.
int VTKAbstractReader::ReadCoScalarData(int PointsNum) {
    int i, j, idx, numComp = 0;
    char name[256];
    char buffer[256];

    if (!(this->ReadString(buffer) && this->Read(&numComp))) {
        //igError("Cannot read color scalar data!"
        //	<< " for file: " << (fname ? fname : "(Null FileName)"));
        return 0;
    }
    this->DecodeString(name, buffer);

    // handle binary different from ASCII since they are stored
    // in a different format float versus uchar
    if (m_FileType == IGAME_BINARY) {
        ArrayObject::Pointer data;
        char type[14] = "unsigned_char";
        data = this->ReadArray(type, PointsNum, numComp);
        if (data != nullptr) {
            data->SetName(name);
            if (this->DataType == POINT_TYPE) {
                m_Data.GetData()->AddAttribute(IG_SCALAR, IG_POINT, data);
            } else if (this->DataType == CELL_TYPE) {
                m_Data.GetData()->AddAttribute(IG_SCALAR, IG_CELL, data);
            }
        } else {
            return 0;
        }
    } else {
        ArrayObject::Pointer data;
        char type[6] = "float";
        data = this->ReadArray(type, PointsNum, numComp);
        if (data != nullptr) {
            UnsignedCharArray::Pointer scalars = UnsignedCharArray::New();
            scalars->SetDimension(numComp);
            scalars->Resize(PointsNum);
            scalars->SetName(name);
            for (i = 0; i < PointsNum; i++) {
                for (j = 0; j < numComp; j++) {
                    idx = i * numComp + j;
                    scalars->SetValue(idx, (unsigned char) (255.0 * data->GetValue(idx) + 0.5));
                }
            }
            if (this->DataType == POINT_TYPE) {
                m_Data.GetData()->AddAttribute(IG_SCALAR, IG_POINT, scalars);
            } else if (this->DataType == CELL_TYPE) {
                m_Data.GetData()->AddAttribute(IG_SCALAR, IG_CELL, scalars);
            }
        } else {
            return 0;
        }
    }

    this->UpdateReadProgress();

    return 1;
}

//------------------------------------------------------------------------------
// Read texture coordinates point attributes. Return 0 if error.
int VTKAbstractReader::ReadTCoordsData(int PointsNum) {
    int dim = 0;
    char line[256], name[256];
    ArrayObject::Pointer data;
    char buffer[256];

    if (!(this->ReadString(buffer) && this->Read(&dim) && this->ReadString(line))) { return 0; }
    this->DecodeString(name, buffer);

    if (dim < 1 || dim > 3) { return 0; }


    data = this->ReadArray(line, PointsNum, dim);
    if (data != nullptr) {
        data->SetName(name);
        if (this->DataType == POINT_TYPE) {
            m_Data.GetData()->AddAttribute(IG_TCOORD, IG_POINT, data);
        } else if (this->DataType == CELL_TYPE) {
            m_Data.GetData()->AddAttribute(IG_TCOORD, IG_CELL, data);
        }
    } else {
        return 0;
    }
    this->UpdateReadProgress();

    return 1;
}

//------------------------------------------------------------------------------
// Read texture coordinates point attributes. Return 0 if error.
int VTKAbstractReader::ReadGlobalIds(int PointsNum) {
    char line[256], name[256];
    ArrayObject::Pointer data;
    char buffer[256];

    if (!(this->ReadString(buffer) && this->ReadString(line))) { return 0; }
    this->DecodeString(name, buffer);
    data = this->ReadArray(line, PointsNum, 1);
    if (data != nullptr) {
        data->SetName(name);
        if (this->DataType == POINT_TYPE) {
            m_Data.GetData()->AddAttribute(IG_SCALAR, IG_POINT, data);
        } else if (this->DataType == CELL_TYPE) {
            m_Data.GetData()->AddAttribute(IG_SCALAR, IG_CELL, data);
        }
    } else {
        return 0;
    }

    this->UpdateReadProgress();

    return 1;
}

//------------------------------------------------------------------------------
// Read pedigree ids. Return 0 if error.
int VTKAbstractReader::ReadPedigreeIds(int PointsNum) {
    char line[256], name[256];
    ArrayObject::Pointer data;
    char buffer[256];

    if (!(this->ReadString(buffer) && this->ReadString(line))) { return 0; }
    this->DecodeString(name, buffer);

    data = this->ReadArray(line, PointsNum, 1);
    if (data != nullptr) {
        data->SetName(name);
        if (this->DataType == POINT_TYPE) {
            m_Data.GetData()->AddAttribute(IG_SCALAR, IG_POINT, data);
        } else if (this->DataType == CELL_TYPE) {
            m_Data.GetData()->AddAttribute(IG_SCALAR, IG_CELL, data);
        }
    } else {
        return 0;
    }

    this->UpdateReadProgress();
    return 1;
}

//------------------------------------------------------------------------------
// Read edge flags. Return 0 if error.
int VTKAbstractReader::ReadEdgeFlags(int PointsNum) {
    int skipEdgeFlags = 0;
    char line[256], name[256];
    ArrayObject::Pointer data;
    char buffer[256];

    if (!(this->ReadString(buffer) && this->ReadString(line))) { return 0; }
    this->DecodeString(name, buffer);

    data = this->ReadArray(line, PointsNum, 1);
    if (data != nullptr) {
        data->SetName(name);
        if (this->DataType == POINT_TYPE) {
            m_Data.GetData()->AddAttribute(IG_SCALAR, IG_POINT, data);
        } else if (this->DataType == CELL_TYPE) {
            m_Data.GetData()->AddAttribute(IG_SCALAR, IG_CELL, data);
        }

    } else {
        return 0;
    }

    this->UpdateReadProgress();
    return 1;
}

//------------------------------------------------------------------------------
// Read lookup table. Return 0 if error.
int VTKAbstractReader::ReadLutData() {
    // temporarily not used ,wil be processed in fulturt
    igDebug("temporarily not process this type");
    this->UpdateReadProgress();
    return 1;
}

//------------------------------------------------------------------------------
int VTKAbstractReader::ReadFieldData() {
    int i, numArrays = 0, skipField = 0;

    char name[256], type[256];
    // metadata 块会用到上一个数组的分量数，初始化为安全值
    int numComp = 1, numTuples = 0;
    ArrayObject::Pointer data;

    if (!(this->ReadString(name) && this->Read(&numArrays))) { return 0; }
    // Read the number of arrays specified
    int addedArrays = 0;
    int skippedArrays = 0;
    for (i = 0; i < numArrays; i++) {
        char buffer[256];
        this->ReadString(buffer);
        if (strcmp(this->LowerCase(buffer), "null_array") == 0) { continue; }
        // 大小写无关：VTK 5.1 写的是 "METADATA"，这里原来只比小写，
        // 会把 METADATA 当数组名继续解析，导致整段错位
        if (strcmp(this->LowerCase(buffer), "metadata") == 0) {
            i--;
            // COMPONENT_NAMES 格式需要知道上一个数组的分量数
            ProcessMetaData(numComp);
            continue;
        }
        this->DecodeString(name, buffer);
        this->Read(&numComp);
        this->Read(&numTuples);
        this->ReadString(type);
        if (!strncmp(this->LowerCase(type), "string", 6)) {
            if (this->m_FileType == IGAME_BINARY) {
                this->ReadLine(buffer);
            } else {
                for (int i = 0; i < numComp * numTuples; i++) { this->ReadString(buffer); }
            }
            continue;
        }
        data = this->ReadArray(type, numTuples, numComp);
        if (data != nullptr) {
            data->SetName(name);
            if (this->DataType == POINT_TYPE) {
                m_Data.GetData()->AddAttribute(IG_SCALAR, IG_POINT, data);
            } else if (this->DataType == CELL_TYPE) {
                m_Data.GetData()->AddAttribute(IG_SCALAR, IG_CELL, data);
            }
            ++addedArrays;
        } else {
            // 类型不支持时 ReadArray 返回空指针；此时数据已经被跳过（或根本无法
            // 跳过），记录下来，避免像以前一样完全静默
            ++skippedArrays;
            igDebug("ReadFieldData: unsupported array '{}' type '{}'", name,
                    type);
        }
    }
    this->UpdateReadProgress();
    igDebug("ReadFieldData: {} arrays added, {} skipped ({})", addedArrays,
            skippedArrays,
            this->DataType == POINT_TYPE ? "point data" : "cell data");
    return 1;
}
bool VTKAbstractReader::ProcessMetaData(int componentCount) {
    char buffer[IGAME_CELL_MAX_SIZE];
    int infoNum = 0;
    if (!this->ReadString(buffer)) {
        igError("metadata error!");
        return false;
    }

    const std::string key = this->LowerCase(buffer, sizeof(buffer));
    if (strncmp(key.c_str(), "information", 6) == 0) {
        // 旧格式：metadata / INFORMATION n / (name, value) × n
        if (!this->Read(&infoNum)) {
            igError("metadata error!");
            return false;
        }
        // Read() 之后指针停在数字后的换行上，ReadLine 需要从行首开始读
        this->SkipNullData();
        for (int i = 0; i < infoNum; i++) {
            if (!this->ReadLine(buffer)) return false;
            if (!this->ReadLine(buffer)) return false;
        }
        return true;
    }

    if (strncmp(key.c_str(), "component_names", 15) == 0) {
        // VTK 5.1 给多分量数组写的元数据：metadata / COMPONENT_NAMES /
        // 每行一个分量名（行数 == 分量数），最后跟一个空行。
        // 有分量数就按数量读，没有就按空行结束（上限 64 行防止跑偏）。
        // 注意：ReadString 把指针停在换行上，必须先走到下一行行首，
        // 否则 ReadLine 会立刻返回失败（这正是之前多分量数组读崩的原因）。
        this->SkipNullData();
        const int lineCount = componentCount > 0 ? componentCount : 64;
        for (int i = 0; i < lineCount; i++) {
            if (this->IS >= this->FILEEND) { return true; }
            // 空行 = 分量名结束（无分量数时靠它判断，也顺便吃掉分隔空行）
            if (*this->IS == '\n' || *this->IS == '\r') {
                this->SkipNullData();
                return true;
            }
            if (!this->ReadLine(buffer)) { return true; }
        }
        return true;
    }

    igError("metadata error!");
    return false;
}
int VTKAbstractReader::ReadCellsWithOffsetType(int& CellsNum, ArrayObject::Pointer& CellsId,
                                               ArrayObject::Pointer& CellsConnect) {
    int offsetsSize{0};
    int connSize{0};
    char buffer[256];
    if (!(this->Read(&offsetsSize) && this->Read(&connSize))) {
        igError("Error while reading cell array header.");
        return 0;
    }
    if (offsetsSize < 1) { return 1; }
    CellsNum = offsetsSize - 1;
    if (!this->ReadString(buffer) ||                                                           // "offsets"
        (strcmp(this->LowerCase(buffer, 256), "offsets") != 0) || !this->ReadString(buffer)) { // datatype
        igError("Error reading cell array offset header.");
        return 0;
    }

    this->LowerCase(buffer, 256);
    CellsId = this->ReadArray(buffer, offsetsSize, 1);
    if (!CellsId) {
        igError("Error reading cell array offset data.");
        return 0;
    }
    if (!this->ReadString(buffer) ||                                                              // "connectivity"
        (strcmp(this->LowerCase(buffer, 256), "connectivity") != 0) || !this->ReadString(buffer)) // datatype
    {
        igError("Error reading cell array connectivity header.");
        return 0;
    }
    this->LowerCase(buffer, 256);
    CellsConnect = this->ReadArray(buffer, connSize, 1);
    if (!CellsConnect) {
        igError("Error reading cell array connectivity data.");
        return 0;
    }
    this->UpdateReadProgress();
    return 1;
}

int VTKAbstractReader::ReadCellsWithCellSizeType(int CellNum, int size, ArrayObject::Pointer& CellsId,
                                                 ArrayObject::Pointer& CellsConnect) {
    auto m_CellsId = IntArray::New();
    m_CellsId->Reserve(CellNum + 1);
    auto m_CellsConnect = IntArray::New();
    if (size > CellNum) { m_CellsConnect->Reserve(size - CellNum); }
    int component, i, j;
    int x;
    int index = 0;
    for (i = 0; i < CellNum; i++) {
        m_CellsId->AddValue(index);
        if (!this->Read(&component)) {
            igError("Cell id error!");
            return 0;
        }
        index += component;
        for (j = 0; j < component; j++) {
            if (!this->Read(&x)) {
                igError("Cell id error!");
                return 0;
            }
            m_CellsConnect->AddValue(x);
        }
    }
    m_CellsId->AddValue(index);
    this->UpdateReadProgress();
    CellsId = m_CellsId;
    CellsConnect = m_CellsConnect;
    return 1;
}
bool VTKAbstractReader::ReadUnstructuredGrid() {
    if (m_UnstructuredMesh == nullptr) { m_UnstructuredMesh = UnstructuredMesh::New(); }
    this->m_DataObjectType = IG_UNSTRUCTURED_MESH;
    char line[IGAME_CELL_MAX_SIZE];
    igIndex i = 0, PointsNum = 0, CellsNum = 0;
    igIndex npts, size = 0, ncells = 0;
    IntArray::Pointer Types;
    ArrayObject::Pointer CellsId;
    ArrayObject::Pointer CellsConnect;
    while (true) {
        if (!this->ReadString(line)) { break; }
        if (line[0] == '\0') {
            continue;
        } else if (!strncmp(this->LowerCase(line), "field", 5)) {
            this->ReadFieldData();
        } else if (!strncmp(line, "points", 6)) {
            if (!this->Read(&npts)) {
                igError("Cannot read number of points!");
                return false;
            }
            Points::Pointer points = Points::New();
            points->Reserve(npts);
            if (!this->ReadPointCoordinates(points, npts)) { return false; }
            m_UnstructuredMesh->SetPoints(points);
        } else if (!strncmp(line, "cells", 5)) {
            if (this->m_FileType == IGAME_BINARY) {
                this->ReadCellsWithOffsetType(ncells, CellsId, CellsConnect);
            } else { // ascii
                const char* tmpIS = this->IS;
                if (!(this->Read(&ncells) && this->Read(&size))) {
                    igError("Cannot read cells!");
                    return false;
                }
                this->SkipNullData();
                if (strncmp(this->IS, "OFFSETS", 7) == 0) {
                    this->IS = tmpIS;
                    this->ReadCellsWithOffsetType(ncells, CellsId, CellsConnect);
                } else {
                    this->ReadCellsWithCellSizeType(ncells, size, CellsId, CellsConnect);
                }
            }
            if (Types) { this->TransferVtkCellToiGameCell(CellsId, CellsConnect, Types); }
        } else if (!strncmp(line, "cell_types", 10)) {
            if (!this->Read(&ncells)) {
                igError("Cannot read cell types!");
                return false;
            }
            Types = DynamicCast<IntArray>(this->ReadArray("int", ncells, 1));
            // Update the dataset
            if (CellsId && CellsConnect) { this->TransferVtkCellToiGameCell(CellsId, CellsConnect, Types); }
        } else if (!strncmp(line, "cell_data", 9)) {
            if (!this->Read(&CellsNum)) {
                igError("Cannot read cell data!");
                return false;
            }
            if (ncells != CellsNum) {
                igError("Number of cells don't match!");
                return false;
            }
            // 不要 break：同一个文件里 CELL_DATA 之后可能还有 POINT_DATA
            if (!this->ReadCellData(CellsNum)) {
                igDebug("Read vtk cell data returned error, continue scanning!");
            }
        }

        else if (!strncmp(line, "point_data", 10)) {
            if (!this->Read(&PointsNum)) {
                igError("Cannot read point data!");
                return false;
            }
            if (npts != PointsNum) {
                igError("Number of points don't match!");
                return false;
            }
            if (!this->ReadPointData(PointsNum)) {
                igDebug("Read vtk point data returned error, continue scanning!");
            }
        } else if (!strncmp(line, "metadata", 8)) {
            this->ProcessMetaData();
        } else {
            //igDebug("Unrecognized keyword: " << line);
            //return false;
        }
    }
    return true;
}

bool VTKAbstractReader::ReadSurfaceMesh() {
    if (m_SurfaceMesh == nullptr) { m_SurfaceMesh = SurfaceMesh::New(); }
    m_DataObjectType = IG_SURFACE_MESH;
    m_SurfaceMesh->SetFaces(iGame::CellArray::New());

    char line[IGAME_CELL_MAX_SIZE];
    char buffer[IGAME_CELL_MAX_SIZE];
    igIndex i = 0, PointsNum = 0, CellsNum = 0, size = 0;
    igIndex npts = 0, ncells = 0;
    igIndex offsetsSize = 0, connSize = 0;
    ArrayObject::Pointer CellsId;
    ArrayObject::Pointer CellsConnect;
    bool sawVertices = false;
    bool sawLines = false;
    bool sawPolygons = false;

    // 在不分配数组的前提下跳过一段二进制数据（用于 VERTICES 这类本读取器
    // 不参与绘制的 payload）。返回 false 表示越界或类型无法识别而长度未知。
    const auto skipBinaryValues = [this](const char* typeToken, size_t count) -> bool {
        if (!typeToken) { return false; }
        char type[64] = {0};
        for (int i = 0; i < 63 && typeToken[i] != '\0'; ++i) { type[i] = typeToken[i]; }
        this->LowerCase(type, sizeof(type));

        size_t itemSize = 4;  // 默认按 int32 处理
        if (strstr(type, "int64") != nullptr ||
            strstr(type, "long_long") != nullptr ||
            strstr(type, "long long") != nullptr) {
            itemSize = 8;
        } else if (strncmp(type, "short", 5) == 0) {
            itemSize = 2;
        } else if (strncmp(type, "char", 4) == 0) {
            itemSize = 1;
        } else if (strncmp(type, "double", 6) == 0) {
            itemSize = 8;
        } else if (strncmp(type, "float", 5) == 0) {
            itemSize = 4;
        }
        const size_t bytes = count * itemSize;
        if (this->IS + bytes > this->FILEEND) { return false; }
        this->IS += bytes;
        return true;
    };

    while (true) {
        if (!this->ReadString(line)) { break; }
        if (line[0] == '\0') {
            continue;
        } else if (!strncmp(this->LowerCase(line), "field", 5)) {
            this->ReadFieldData();
        } else if (!strncmp(line, "points", 6)) {
            if (!this->Read(&npts)) {
                igError("Cannot read number of points!");
                return false;
            }
            Points::Pointer points = Points::New();
            points->Reserve(npts);
            if (!this->ReadPointCoordinates(points, npts)) { return false; }
            m_SurfaceMesh->SetPoints(points);
        } 
          
        else if (!strncmp(line, "lines", 5)) {
            if (this->m_FileType == IGAME_BINARY) {
                this->ReadCellsWithOffsetType(ncells, CellsId, CellsConnect);
            } else {
                const char* tmpIS = this->IS;
                if (!(this->Read(&ncells) && this->Read(&size))) {
                    igError("Cannot read cells!");
                    return false;
                }
                this->SkipNullData();
                if (strncmp(this->IS, "OFFSETS", 7) == 0) {
                    this->IS = tmpIS;
                    this->ReadCellsWithOffsetType(ncells, CellsId, CellsConnect);
                } else {
                    this->ReadCellsWithCellSizeType(ncells, size, CellsId, CellsConnect);
                }
            }
            m_SurfaceMesh->SetEdges(this->CreateCellArray(CellsId, CellsConnect));
            sawLines = true;
        }

        else if (!strncmp(line, "vertices", 8)) {
            // 与 lines/polygons 一致：先记住两个计数之前的位置，
            // 便于 ASCII 的 OFFSETS 分支回退重读。
            const char* tmpIS = this->IS;
            igIndex verticesNum = 0, verticesSize = 0;
            if (!(this->Read(&verticesNum) && this->Read(&verticesSize))) {
                igError("Cannot read vertices!");
                return false;
            }
            if (verticesNum < 0 || verticesSize < 0) {
                igError("Invalid vertices counts!");
                return false;
            }
            if (this->m_FileType == IGAME_BINARY) {
                // POLYDATA 的 VERTICES 是“每 cell 一个点”的点云单元，本读取器
                // 以“点 + 点属性”形式承载它，不需要真正建立 8192 万个单点单元。
                // 关键是按正确字节数跳过 payload，避免把二进制当文本继续扫描。
                bool skipped = false;
                if (this->FileVersion >= 51) {
                    // VTK 5.1+ offset 布局：
                    // <offsetsSize> <connSize> "OFFSETS" <type> <data> "CONNECTIVITY" <type> <data>
                    igIndex offsetsCount = 0, connectCount = 0;
                    char token[256], typeName[256];
                    if (!(this->Read(&offsetsCount) && this->Read(&connectCount)) ||
                        !this->ReadString(token) ||
                        strcmp(this->LowerCase(token), "offsets") != 0 ||
                        !this->ReadString(typeName)) {
                        igError("Error reading vertices offset header.");
                        return false;
                    }
                    if (offsetsCount < 0 || connectCount < 0) {
                        igError("Invalid vertices offset counts!");
                        return false;
                    }
                    skipped = skipBinaryValues(typeName, offsetsCount);
                    if (skipped && (!this->ReadString(token) ||
                                    strcmp(this->LowerCase(token), "connectivity") != 0 ||
                                    !this->ReadString(typeName))) {
                        igError("Error reading vertices connectivity header.");
                        return false;
                    }
                    if (skipped) { skipped = skipBinaryValues(typeName, connectCount); }
                } else {
                    // 经典布局（VTK <= 5.0）：size 个 int32，按 (count, ids...) 排列
                    skipped = skipBinaryValues("int", verticesSize);
                }
                if (!skipped) {
                    igError("Cannot skip vertices payload!");
                    return false;
                }
            } else {
                this->SkipNullData();
                if (strncmp(this->IS, "OFFSETS", 7) == 0) {
                    this->IS = tmpIS;
                    if (!this->ReadCellsWithOffsetType(verticesNum, CellsId, CellsConnect)) {
                        igError("Error reading vertices offset cells!");
                        return false;
                    }
                } else if (!this->ReadCellsWithCellSizeType(verticesNum, verticesSize, CellsId,
                                                           CellsConnect)) {
                    igError("Error reading vertices cells!");
                    return false;
                }
            }
            sawVertices = true;
        }
        
        
        else if (!strncmp(line, "polygons", 8)) {

            if (this->m_FileType == IGAME_BINARY) {
                this->ReadCellsWithOffsetType(ncells, CellsId, CellsConnect);
            } else {
                const char* tmpIS = this->IS;
                if (!(this->Read(&ncells) && this->Read(&size))) {
                    igError("Cannot read cells!");
                    return false;
                }
                this->SkipNullData();
                if (strncmp(this->IS, "OFFSETS", 7) == 0) {
                    this->IS = tmpIS;
                    this->ReadCellsWithOffsetType(ncells, CellsId, CellsConnect);
                } else {
                    this->ReadCellsWithCellSizeType(ncells, size, CellsId, CellsConnect);
                }
            }
            m_SurfaceMesh->SetFaces(this->CreateCellArray(CellsId, CellsConnect));
            sawPolygons = true;
        } else if (!strncmp(line, "cell_data", 9)) {
            if (!this->Read(&CellsNum)) {
                igError("Cannot read cell data!");
                return false;
            }
            if (ncells != CellsNum) {
                igError("Number of cells don't match!");
                return false;
            }
            // 不要 break：同一个文件里 CELL_DATA 之后可能还有 POINT_DATA
            if (!this->ReadCellData(ncells)) {
                igDebug("Read vtk cell data returned error, continue scanning!");
            }
        } else if (!strncmp(line, "point_data", 10)) {
            if (!this->Read(&PointsNum)) {
                igError("Cannot read point data!");
                return false;
            }
            if (npts != PointsNum) {
                igError("Number of points don't match!");
                return false;
            }
            if (!this->ReadPointData(PointsNum)) {
                igDebug("Read vtk point data returned error, continue scanning!");
            }
        } else if (!strncmp(line, "metadata", 8)) {
            this->ProcessMetaData();
        } else {
            //igDebug("Unrecognized keyword: " << line);
            //return false;
        }
    }
    // 只有 VERTICES 的 POLYDATA 是点云：保留点与点属性，用点样式显示
    // （SurfaceMesh 默认样式是 IG_SURFACE，没有面片时不可见）。
    if (sawVertices && !sawLines && !sawPolygons) {
        m_SurfaceMesh->SetViewStyle(IG_POINT);
    }
    return true;
}
bool VTKAbstractReader::ReadStructuredGrid() {
    if (m_StructuredMesh == nullptr) { m_StructuredMesh = StructuredMesh::New(); }
    m_DataObjectType = IG_STRUCTURED_MESH;
    char line[IGAME_CELL_MAX_SIZE];
    char buffer[IGAME_CELL_MAX_SIZE];
    igIndex size[3] = {0, 0, 0};
    // STRUCTURED_POINTS 的点坐标由 原点 + 间距 推出；文件里没写时按 VTK 默认值 0 / 1
    double origin[3] = {0.0, 0.0, 0.0};
    double aspectRatio[3] = {1.0, 1.0, 1.0};
    igIndex npts = 0;
    igIndex ncells = 0;

    while (true) {
        if (!this->ReadString(line)) { break; }
        if (line[0] == '\0') { continue; }
        if (!strncmp(this->LowerCase(line), "dimensions", 10)) {
            if (!this->Read(&size[0]) || !this->Read(&size[1]) || !this->Read(&size[2])) {
                igError("Read dimension error!");
                return false;
            }
            if (size[2] == 0) { size[2] = 1; }
            m_StructuredMesh->SetDimensionSize(size);
        } else if (!strncmp(line, "spacing", 7) || !strncmp(line, "aspect_ratio", 12)) {
            // SPACING 是 VTK legacy 格式的标准写法，ASPECT_RATIO 是它的旧名，两者含义相同
            if (!this->Read(&aspectRatio[0]) || !this->Read(&aspectRatio[1]) || !this->Read(&aspectRatio[2])) {
                igError("Read extend error!");
                return false;
            }
        } else if (!strncmp(line, "origin", 6)) {
            if (!this->Read(&origin[0]) || !this->Read(&origin[1]) || !this->Read(&origin[2])) {
                igError("Read extend error!");
                return false;
            }
        } else if (!strncmp(line, "points", 6)) {
            if (!this->Read(&npts)) {
                igError("Cannot read number of points!");
                return false;
            }
            if (size[0] * size[1] * size[2] != npts) {
                igError("Number of points don't match!");
                return false;
            }
            Points::Pointer points = Points::New();
            if (!this->ReadPointCoordinates(points, npts)) { return false; }
            m_StructuredMesh->SetPoints(points);
        } else if (!strncmp(line, "cell_data", 9)) {
            if (!this->Read(&ncells)) {
                igError("Cannot read cell data!");
                return false;
            }
            // 用 64 位比较：网格稍大时 size[0]*size[1]*size[2] 会溢出 int
            const long long expectedCells =
                    size[2] == 1
                            ? static_cast<long long>(size[0] - 1) * (size[1] - 1)
                            : static_cast<long long>(size[0] - 1) *
                                      (size[1] - 1) * (size[2] - 1);
            if (ncells != expectedCells) {
                igError("Number of cells don't match!");
                return false;
            }
            // 注意：不要 break。同一个文件里 CELL_DATA 之后往往还有 POINT_DATA
            // （ParaView 导出的顺序就是 CELL_DATA 在前），提前 break 会把点数据
            // 整段丢掉，界面上就只剩一个 ghost 属性。
            if (!this->ReadCellData(ncells)) {
                igDebug("Read vtk cell data returned error, continue scanning!");
            }
        } else if (!strncmp(line, "point_data", 10)) {
            if (!this->Read(&npts)) {
                igError("Cannot read point data!");
                return false;
            }
            const long long expectedPoints =
                    static_cast<long long>(size[0]) * size[1] * size[2];
            if (npts != expectedPoints) {
                igError("Number of points don't match!");
                return false;
            }
            if (!this->ReadPointData(npts)) {
                igDebug("Read vtk point data returned error, continue scanning!");
            }
        } else if (!strncmp(line, "metadata", 8)) {
            this->ProcessMetaData();
        } else {
            //igDebug("Unrecognized keyword: " << line);
            //return false;
        }
    }
    if (m_StructuredMesh->GetPoints() == nullptr || m_StructuredMesh->GetPoints()->GetNumberOfPoints() == 0) {
        igIndex i, j, k;
        Point p;
        Points::Pointer points = Points::New();
        points->Reserve(npts);
        p[2] = origin[2];
        for (k = 0; k < size[2]; ++k) {
            p[1] = origin[1];
            for (j = 0; j < size[1]; ++j) {
                p[0] = origin[0];
                for (i = 0; i < size[0]; ++i) {
                    points->AddPoint(p);
                    p[0] += aspectRatio[0];
                }
                p[1] += aspectRatio[1];
            }
            p[2] += aspectRatio[2];
        }
        m_StructuredMesh->SetPoints(points);
    }
    m_StructuredMesh->GenStructuredCellConnectivities();
    return true;
}
CellArray::Pointer VTKAbstractReader::CreateCellArray(ArrayObject::Pointer CellsID, ArrayObject::Pointer CellsConnect) {
    // Each POLYGONS/LINES section owns its connectivity. Reset removes the
    // initial zero offset (breaking mixed polygons), and also overwrites any
    // preceding section that already holds this same CellArray.
    m_CellArray = CellArray::New();

    if (CellsID == nullptr || CellsConnect == nullptr || CellsID->GetNumberOfElements() < 2) {
        igError("Invalid cell arrays while creating CellArray.");
        return m_CellArray;
    }

    int CellNum = CellsID->GetNumberOfElements() - 1;
    const int connectCount = static_cast<int>(CellsConnect->GetNumberOfElements());
    int skippedCells = 0;

    for (int i = 0; i < CellNum; i++) {
        const int st = static_cast<int>(CellsID->GetValue(i));
        const int ed = static_cast<int>(CellsID->GetValue(i + 1));
        const int size = ed - st;
        if (st < 0 || ed < st || ed > connectCount || size <= 0) {
            skippedCells++;
            continue;
        }

        std::vector<igIndex> vhs(size);
        for (int j = 0; j < size; j++) { vhs[j] = static_cast<igIndex>(CellsConnect->GetValue(st + j)); }
        m_CellArray->AddCellIds(vhs.data(), size);
    }

    if (skippedCells > 0) { igDebug("CreateCellArray skipped invalid cells: {}", skippedCells); }
    return m_CellArray;
}

const void VTKAbstractReader::TransferVtkCellToiGameCell(ArrayObject::Pointer CellsID,
                                                         ArrayObject::Pointer CellsConnect,
                                                         IntArray::Pointer VtkCellsType) {
    DataObject::Pointer Mesh = m_UnstructuredMesh;
    this->m_DataObjectType = IG_UNSTRUCTURED_MESH;
    TransferVtkCellToiGameCell(Mesh, CellsID, CellsConnect, VtkCellsType);
    m_UnstructuredMesh = DynamicCast<UnstructuredMesh>(Mesh);
    this->UpdateReadProgress();
}

void VTKAbstractReader::TransferVtkCellToiGameCell(DataObject::Pointer& _mesh, ArrayObject::Pointer CellsID,
                                                   ArrayObject::Pointer CellsConnect,
                                                   ArrayObject::Pointer VtkCellsType) {
    TransferVtkCellToiGameCell(_mesh, CellsID, CellsConnect, VtkCellsType, nullptr, nullptr, nullptr, nullptr);
}

void VTKAbstractReader::TransferVtkCellToiGameCell(DataObject::Pointer& _mesh, ArrayObject::Pointer CellsID,
                                                   ArrayObject::Pointer CellsConnect,
                                                   ArrayObject::Pointer VtkCellsType,
                                                   ArrayObject::Pointer FacesConnect,
                                                   ArrayObject::Pointer FacesOffset,
                                                   ArrayObject::Pointer PolyToFaces,
                                                   ArrayObject::Pointer PolysOffset) {
    if (CellsID == nullptr || CellsConnect == nullptr || VtkCellsType == nullptr) {
        igError("Invalid input arrays in TransferVtkCellToiGameCell.");
        return;
    }
    UnstructuredMesh::Pointer mesh = _mesh == nullptr ? UnstructuredMesh::New() : DynamicCast<UnstructuredMesh>(_mesh);
    const int CellNum = static_cast<int>(VtkCellsType->GetNumberOfElements());
    const int offsetsCount = static_cast<int>(CellsID->GetNumberOfElements());
    const int connectCount = static_cast<int>(CellsConnect->GetNumberOfElements());
    int skippedCells = 0;
    int normalizedCells = 0;
    bool onlyVertexCells = CellNum > 0 && mesh->GetNumberOfCells() == 0;

    for (int i = 0; i < CellNum; i++) {
        if (i + 1 >= offsetsCount) {
            skippedCells++;
            continue;
        }

        const int st = static_cast<int>(CellsID->GetValue(i));
        const int ed = static_cast<int>(CellsID->GetValue(i + 1));
        int size = ed - st;
        if (st < 0 || ed < st || ed > connectCount || size <= 0) {
            skippedCells++;
            continue;
        }

        const VTKTYPE declaredType = (VTKTYPE) VtkCellsType->GetValue(i);
        onlyVertexCells &= declaredType == VERTEX;
        VTKTYPE type = NormalizeVtkCellType(declaredType, size);
        if (type != declaredType) normalizedCells++;
        if (!IsVtkCellSizeValid(type, size)) {
            skippedCells++;
            continue;
        }

        std::vector<igIndex> vhs(size);

        if (type != POLYHEDRON) {
            for (int j = 0; j < size; j++) { vhs[j] = static_cast<igIndex>(CellsConnect->GetValue(st + j)); }
        } else {
            // for celltype == 42
            bool isValid = (FacesConnect && FacesConnect->GetNumberOfElements() > 0 && PolysOffset &&
                            PolysOffset->GetNumberOfElements() > 0);
            if (!isValid) {
                igError("no faces/face_connectivity/faceoffsets/polyhedron_offsets data array found in POLYHEDRON "
                        "cell");
                return;
            }

            if (PolyToFaces && PolyToFaces->GetNumberOfElements() > 0) {
                // >= 2.3 version
                isValid = (FacesOffset && FacesOffset->GetNumberOfElements() > 0);
                if (!isValid) {
                    igError("no face_offsets data array found in POLYHEDRON cell (>=2.3 version)");
                    return;
                }

                int fSt = static_cast<int>(PolysOffset->GetValue(i));
                int fEd = static_cast<int>(PolysOffset->GetValue(i + 1));
                int numFaces = fEd - fSt;

                // compute the vhs size in advance
                int totalSize = 1; // numFaces
                for (int f = fSt; f < fEd; f++) {
                    int faceId = PolyToFaces->GetValue(f);
                    int pSt = static_cast<int>(FacesOffset->GetValue(faceId));
                    int pEd = static_cast<int>(FacesOffset->GetValue(faceId + 1));
                    totalSize += 1 + (pEd - pSt); // 1(fSize) + facePointCount
                }
                vhs.resize(totalSize);

                // make sure the vhs structure is: [numFaces, fSize0, v0..vN, fSize1, v0..vM, ...]
                int idx = 0;
                vhs[idx++] = numFaces;
                for (int f = fSt; f < fEd; f++) {
                    int faceId = PolyToFaces->GetValue(f);
                    if (faceId + 1 >= FacesOffset->GetNumberOfElements()) {
                        skippedCells++;
                        continue;
                    }
                    int pSt = static_cast<int>(FacesOffset->GetValue(faceId));
                    int pEd = static_cast<int>(FacesOffset->GetValue(faceId + 1));
                    int numPts = pEd - pSt;
                    vhs[idx++] = numPts;
                    for (int k = pSt; k < pSt + numPts; k++) {
                        vhs[idx++] = static_cast<igIndex>(FacesConnect->GetValue(k));
                    }
                }
                size = totalSize;
            } else {
                // < 2.3 version
                int fSt = static_cast<int>(PolysOffset->GetValue(i));
                int fEd = static_cast<int>(PolysOffset->GetValue(i + 1));
                int totalSize = fEd - fSt;
                vhs.resize(totalSize);
                for (int k = 0; k < totalSize; k++) { vhs[k] = static_cast<igIndex>(FacesConnect->GetValue(fSt + k)); }
                size = totalSize;
            }
        }

        switch (type) {
            case iGame::VTKAbstractReader::T0:
                break;
            case iGame::VTKAbstractReader::VERTEX:
                mesh->AddCell(vhs.data(), size, IG_VERTEX);
                break;
            case iGame::VTKAbstractReader::POLYVERTEX:
                break;
            case iGame::VTKAbstractReader::LINE:
                mesh->AddCell(vhs.data(), size, IG_LINE);
                break;
            case iGame::VTKAbstractReader::POLYLINE:
                mesh->AddCell(vhs.data(), size, IG_POLY_LINE);
                break;
            case iGame::VTKAbstractReader::TRIANGLE:
                mesh->AddCell(vhs.data(), size, IG_TRIANGLE);
                break;
            case iGame::VTKAbstractReader::TRIANGLESTRIP:
                break;
            case iGame::VTKAbstractReader::POLYGON:
                mesh->AddCell(vhs.data(), size, IG_POLYGON);
                break;
            case iGame::VTKAbstractReader::PIXEL:
                mesh->AddCell(vhs.data(), size, IG_QUAD);
                break;
            case iGame::VTKAbstractReader::QUAD:
                mesh->AddCell(vhs.data(), size, IG_QUAD);
                break;
            case iGame::VTKAbstractReader::TETRA:
                mesh->AddCell(vhs.data(), size, IG_TETRA);
                break;
            case iGame::VTKAbstractReader::VOXEL: {
                // VTK_VOXEL (cell type 11) is topologically a hexahedron whose node
                // ordering differs from VTK_HEXAHEDRON. Reorder the 8 point ids into
                // iGame hexahedron ordering and store it as IG_HEXAHEDRON so that
                // rendering, surface extraction and the IGC codec all handle it.
                static constexpr igIndex VoxelToHexahedron[8] = {0, 1, 3, 2, 4, 5, 7, 6};
                igIndex hexVhs[8];
                for (int j = 0; j < 8; ++j) { hexVhs[j] = vhs[VoxelToHexahedron[j]]; }
                mesh->AddCell(hexVhs, size, IG_HEXAHEDRON);
            } break;
            case iGame::VTKAbstractReader::HEXAHEDRON:
                mesh->AddCell(vhs.data(), size, IG_HEXAHEDRON);
                break;
            case iGame::VTKAbstractReader::WEDGE:
                mesh->AddCell(vhs.data(), size, IG_PRISM);
                break;
            case iGame::VTKAbstractReader::PYRAMID:
                mesh->AddCell(vhs.data(), size, IG_PYRAMID);
                break;
            case iGame::VTKAbstractReader::PENTAGONAL_PRISM:
                break;
            case iGame::VTKAbstractReader::HEXAGONAL_PRISM:
                break;
            case iGame::VTKAbstractReader::T17:
                break;
            case iGame::VTKAbstractReader::T18:
                break;
            case iGame::VTKAbstractReader::T19:
                break;
            case iGame::VTKAbstractReader::T20:
                break;
            case iGame::VTKAbstractReader::QUADRATIC_EDGE:
                mesh->AddCell(vhs.data(), size, IG_QUADRATIC_EDGE);
                break;
            case iGame::VTKAbstractReader::QUADRATIC_TRIANGLE:
                mesh->AddCell(vhs.data(), size, IG_QUADRATIC_TRIANGLE);
                break;
            case iGame::VTKAbstractReader::QUADRATIC_QUAD:
                mesh->AddCell(vhs.data(), size, IG_QUADRATIC_QUAD);
                break;
            case iGame::VTKAbstractReader::QUADRATIC_TETRA:
                mesh->AddCell(vhs.data(), size, IG_QUADRATIC_TETRA);
                break;
            case iGame::VTKAbstractReader::QUADRATIC_HEXAHEDRON:
                mesh->AddCell(vhs.data(), size, IG_QUADRATIC_HEXAHEDRON);
                break;
            case iGame::VTKAbstractReader::QUADRATIC_WEDGE:
                mesh->AddCell(vhs.data(), size, IG_QUADRATIC_PRISM);
                break;
            case iGame::VTKAbstractReader::QUADRATIC_PYRAMID:
                mesh->AddCell(vhs.data(), size, IG_QUADRATIC_PYRAMID);
                break;
            case iGame::VTKAbstractReader::BIQUADRATIC_QUAD:
                break;
            case iGame::VTKAbstractReader::TRIQUADRATIC_HEXAHEDRON:
                break;
            case iGame::VTKAbstractReader::QUADRATIC_LINEAR_QUAD:
                break;
            case iGame::VTKAbstractReader::QUADRATIC_LINEAR_WEDGE:
                break;
            case iGame::VTKAbstractReader::BIQUADRATIC_QUADRATIC_WEDGE:
                break;
            case iGame::VTKAbstractReader::BIQUADRATIC_QUADRATIC_HEXAHEDRON:
                break;
            case iGame::VTKAbstractReader::BIQUADRATIC_TRIANGLE:
                break;
            case iGame::VTKAbstractReader::CUBIC_LINE:
                break;
            case iGame::VTKAbstractReader::QUADRATIC_POLYGON:
                break;
            case iGame::VTKAbstractReader::TRIQUADRATIC_PYRAMID:
                break;
            case iGame::VTKAbstractReader::POLYHEDRON: //42
            {
                mesh->AddCell(vhs.data(), size, IG_POLYHEDRON);
            } break;
            case iGame::VTKAbstractReader::LAGRANGE_TETRAHEDRON:
                mesh->AddCell(vhs.data(), size, IG_LAGRANGE_TETRAHEDRON);
                break;
            case iGame::VTKAbstractReader::LAGRANGE_HEXAHEDRON:
                mesh->AddCell(vhs.data(), size, IG_LAGRANGE_HEXAHEDRON);
                break;
            case iGame::VTKAbstractReader::LAGRANGE_PRISM:
                mesh->AddCell(vhs.data(), size, IG_LAGRANGE_PRISM);
                break;
            case iGame::VTKAbstractReader::LAGRANGE_PYRAMID:
                mesh->AddCell(vhs.data(), size, IG_LAGRANGE_PYRAMID);
                break;
            case iGame::VTKAbstractReader::LAGRANGE_QUADRILATERAL:
                mesh->AddCell(vhs.data(), size, IG_LAGRANGE_QUADRILATERAL);
                break;
            case iGame::VTKAbstractReader::LAGRANGE_TRIANGLE:
                mesh->AddCell(vhs.data(), size, IG_LAGRANGE_TRIANGLE);
                break;
            default:
                break;
        }
    }
    if (normalizedCells > 0) {
        igDebug("TransferVtkCellToiGameCell normalized mislabeled quadratic cells: {}", normalizedCells);
    }
    if (skippedCells > 0) { igDebug("TransferVtkCellToiGameCell skipped invalid cells: {}", skippedCells); }
    // A vertex-only grid has no surface to extract. Preserve its cells and
    // attributes, and draw the original points instead of an empty shell.
    if (onlyVertexCells && mesh->GetNumberOfCells() == CellNum) {
        mesh->SetShellRenderingOption(false);
        mesh->SetViewStyle(IG_POINTS);
    }
    _mesh = mesh;
}


IGAME_NAMESPACE_END
