#include "iGameVTKAbstractReader.h"
#include "iGameByteSwap.h"
IGAME_NAMESPACE_BEGIN

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
        igDebug("Reading file version: {}.{} with older reader version {}.{}" ,
                this->FileMajorVersion,
                this->FileMinorVersion,
                vtkLegacyReaderMajorVersion,
                vtkLegacyReaderMinorVersion);
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

    igDebug("Reading vtk file entitled!{}" , line);
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
    igDebug("Read {} points" , points->GetNumberOfPoints());
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
            return 0;
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
            return 0;
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
    int numComp, numTuples;
    ArrayObject::Pointer data;

    if (!(this->ReadString(name) && this->Read(&numArrays))) { return 0; }
    // Read the number of arrays specified
    for (i = 0; i < numArrays; i++) {
        char buffer[256];
        this->ReadString(buffer);
        if (strcmp(this->LowerCase(buffer), "null_array") == 0) { continue; }
        if (strcmp(buffer, "metadata") == 0) {
            i--;
            ProcessMetaData();
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
        }
    }
    this->UpdateReadProgress();
    return 1;
}
bool VTKAbstractReader::ProcessMetaData() {
    char buffer[IGAME_CELL_MAX_SIZE];
    int infoNum = 0;
    if (!this->ReadString(buffer) || strncmp(this->LowerCase(buffer), "information", 6)) {
        igError("metadata error!");
        return false;
    }
    if (!this->Read(&infoNum)) {
        igError("metadata error!");
        return false;
    }
    for (int i = 0; i < infoNum; i++) {
        if (!this->ReadLine(buffer)) return false;
        if (!this->ReadLine(buffer)) return false;
    }
    return true;
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
            this->ReadCellData(CellsNum);
            break; // out of this loop
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
            this->ReadPointData(PointsNum);
            break; // out of this loop
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
    char line[IGAME_CELL_MAX_SIZE];
    char buffer[IGAME_CELL_MAX_SIZE];
    igIndex i = 0, PointsNum = 0, CellsNum = 0, size = 0;
    igIndex npts = 0, ncells = 0;
    igIndex offsetsSize = 0, connSize = 0;
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
            m_SurfaceMesh->SetPoints(points);
        } else if (!strncmp(line, "polygons", 8)) {

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
        } else if (!strncmp(line, "cell_data", 9)) {
            if (!this->Read(&CellsNum)) {
                igError("Cannot read cell data!");
                return false;
            }
            if (ncells != CellsNum) {
                igError("Number of cells don't match!");
                return false;
            }
            this->ReadCellData(ncells);
            break; // out of this loop
        } else if (!strncmp(line, "point_data", 10)) {
            if (!this->Read(&PointsNum)) {
                igError("Cannot read point data!");
                return false;
            }
            if (npts != PointsNum) {
                igError("Number of points don't match!");
                return false;
            }
            this->ReadPointData(PointsNum);
            break; // out of this loop
        } else if (!strncmp(line, "metadata", 8)) {
            this->ProcessMetaData();
        } else {
            //igDebug("Unrecognized keyword: " << line);
            //return false;
        }
    }
    return true;
}
bool VTKAbstractReader::ReadStructuredGrid() {
    if (m_StructuredMesh == nullptr) { m_StructuredMesh = StructuredMesh::New(); }
    m_DataObjectType = IG_STRUCTURED_MESH;
    char line[IGAME_CELL_MAX_SIZE];
    char buffer[IGAME_CELL_MAX_SIZE];
    igIndex size[3];
    double origin[3];
    double aspectRatio[3];
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
        } else if (!strncmp(line, "aspect_ratio", 12)) {
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
            if ((size[2] == 1 && (ncells != (size[0] - 1) * (size[1] - 1))) ||
                (size[2] != 1 && (ncells != (size[0] - 1) * (size[1] - 1) * (size[2] - 1)))) {
                igError("Number of cells don't match!");
                return false;
            }
            this->ReadCellData(ncells);
            break; // out of this loop
        } else if (!strncmp(line, "point_data", 10)) {
            if (!this->Read(&npts)) {
                igError("Cannot read point data!");
                return false;
            }
            if (size[0] * size[1] * size[2] != npts) {
                igError("Number of points don't match!");
                return false;
            }
            this->ReadPointData(npts);
            break; // out of this loop
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
    if (m_CellArray == nullptr) { m_CellArray = CellArray::New(); }
    int CellNum = CellsID->GetNumberOfElements() - 1;
    int index = 0;
    int size = 0;
    int vhs[IGAME_CELL_MAX_SIZE];
    int st, ed;

    for (int i = 0; i < CellNum; i++) {
        st = static_cast<int>(CellsID->GetValue(i));
        ed = static_cast<int>(CellsID->GetValue(i + 1));
        size = ed - st;
        for (int j = 0; j < size; j++) { vhs[j] = static_cast<int>(CellsConnect->GetValue(st + j)); }
        m_CellArray->AddCellIds(vhs, size);
    }
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
    UnstructuredMesh::Pointer mesh = _mesh == nullptr ? UnstructuredMesh::New() : DynamicCast<UnstructuredMesh>(_mesh);
    int CellNum = VtkCellsType->GetNumberOfElements();
    int index = 0;
    int size = 0;
    int vhs[IGAME_CELL_MAX_SIZE];
    int st, ed;
    for (int i = 0; i < CellNum; i++) {
        st = static_cast<int>(CellsID->GetValue(i));
        ed = static_cast<int>(CellsID->GetValue(i + 1));
        size = ed - st;
        for (int j = 0; j < size; j++) { vhs[j] = static_cast<int>(CellsConnect->GetValue(st + j)); }
        VTKTYPE type = (VTKTYPE) VtkCellsType->GetValue(i);
        switch (type) {
            case iGame::VTKAbstractReader::T0:
                break;
            case iGame::VTKAbstractReader::VERTEX:
                break;
            case iGame::VTKAbstractReader::POLYVERTEX:
                break;
            case iGame::VTKAbstractReader::LINE:
                mesh->AddCell(vhs, size, IG_LINE);
                break;
            case iGame::VTKAbstractReader::POLYLINE:
                mesh->AddCell(vhs, size, IG_POLY_LINE);
                break;
            case iGame::VTKAbstractReader::TRIANGLE:
                mesh->AddCell(vhs, size, IG_TRIANGLE);
                break;
            case iGame::VTKAbstractReader::TRIANGLESTRIP:
                break;
            case iGame::VTKAbstractReader::POLYGON:
                mesh->AddCell(vhs, size, IG_POLYGON);
                break;
            case iGame::VTKAbstractReader::PIXEL:
                mesh->AddCell(vhs, size, IG_QUAD);
                break;
            case iGame::VTKAbstractReader::QUAD:
                mesh->AddCell(vhs, size, IG_QUAD);
                break;
            case iGame::VTKAbstractReader::TETRA:
                mesh->AddCell(vhs, size, IG_TETRA);
                break;
            case iGame::VTKAbstractReader::VOXEL:
                break;
            case iGame::VTKAbstractReader::HEXAHEDRON:
                mesh->AddCell(vhs, size, IG_HEXAHEDRON);
                break;
            case iGame::VTKAbstractReader::WEDGE:
                mesh->AddCell(vhs, size, IG_PRISM);
                break;
            case iGame::VTKAbstractReader::PYRAMID:
                mesh->AddCell(vhs, size, IG_PYRAMID);
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
                mesh->AddCell(vhs, size, IG_QUADRATIC_EDGE);
                break;
            case iGame::VTKAbstractReader::QUADRATIC_TRIANGLE:
                mesh->AddCell(vhs, size, IG_QUADRATIC_TRIANGLE);
                break;
            case iGame::VTKAbstractReader::QUADRATIC_QUAD:
                mesh->AddCell(vhs, size, IG_QUADRATIC_QUAD);
                break;
            case iGame::VTKAbstractReader::QUADRATIC_TETRA:
                mesh->AddCell(vhs, size, IG_QUADRATIC_TETRA);
                break;
            case iGame::VTKAbstractReader::QUADRATIC_HEXAHEDRON:
                mesh->AddCell(vhs, size, IG_QUADRATIC_HEXAHEDRON);
                break;
            case iGame::VTKAbstractReader::QUADRATIC_WEDGE:
                mesh->AddCell(vhs, size, IG_QUADRATIC_PRISM);
                break;
            case iGame::VTKAbstractReader::QUADRATIC_PYRAMID:
                mesh->AddCell(vhs, size, IG_QUADRATIC_PYRAMID);
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
                mesh->AddCell(vhs, size, IG_POLYHEDRON);
            } break;
            case iGame::VTKAbstractReader::LAGRANGE_TETRAHEDRON:
                mesh->AddCell(vhs, size, IG_LAGRANGE_TETRAHEDRON);
                break;
            case iGame::VTKAbstractReader::LAGRANGE_HEXAHEDRON:
                mesh->AddCell(vhs, size, IG_LAGRANGE_HEXAHEDRON);
                break;
            case iGame::VTKAbstractReader::LAGRANGE_PRISM:
                mesh->AddCell(vhs, size, IG_LAGRANGE_PRISM);
                break;
            case iGame::VTKAbstractReader::LAGRANGE_PYRAMID:
                mesh->AddCell(vhs, size, IG_LAGRANGE_PYRAMID);
                break;
            case iGame::VTKAbstractReader::LAGRANGE_QUADRILATERAL:
                mesh->AddCell(vhs, size, IG_LAGRANGE_QUADRILATERAL);
                break;
            case iGame::VTKAbstractReader::LAGRANGE_TRIANGLE:
                mesh->AddCell(vhs, size, IG_LAGRANGE_TRIANGLE);
                break;
            default:
                break;
        }
    }
    _mesh = mesh;
}


IGAME_NAMESPACE_END
