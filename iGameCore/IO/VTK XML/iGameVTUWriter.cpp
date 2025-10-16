#include "iGameVTUWriter.h"

IGAME_NAMESPACE_BEGIN

bool VTUWriter::GenerateBuffers() {
    if (!m_UnstructuredMesh) return false;

    WriteHeaderToBuffer();
    WritePointsToBuffer(m_UnstructuredMesh->GetPoints());
    WriteCellsToBuffer(m_UnstructuredMesh->GetCells());
    WritePointsAttributesToBuffer(m_UnstructuredMesh->GetAttributeSet());
    WriteCellsAttributesToBuffer(m_UnstructuredMesh->GetAttributeSet());
    TransferBuffer();
    return true;
}

void VTUWriter::WriteHeaderToBuffer() {
    auto buffer = CharArray::New();
    std::string data = "<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"LittleEndian\">\n";
    data += "<UnstructuredGrid>\n";
    data += "<Piece NumberOfPoints=\"" + std::to_string(m_UnstructuredMesh->GetPoints()->GetNumberOfPoints()) +
            "\" NumberOfCells=\"" + std::to_string(m_UnstructuredMesh->GetCells()->GetNumberOfCells()) + "\">\n";
    AddStringToBuffer(data, buffer);
    m_TemporaryBuffers.emplace_back(buffer);
}

void VTUWriter::WritePointsToBuffer(Points::Pointer Points) {
    if (!Points) return;
    auto buffer = CharArray::New();
    AddStringToBuffer("<Points>\n<DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">\n", buffer);
    m_TemporaryBuffers.emplace_back(buffer);

    int nPoints = Points->GetNumberOfPoints();
    auto buffer2 = CharArray::New();
    for (int i = 0; i < nPoints; i++) {
        Point p = Points->GetPoint(i);
        AddStringToBuffer(std::to_string(p[0]) + " " + std::to_string(p[1]) + " " + std::to_string(p[2]) + "\n",
                          buffer2);
    }
    m_TemporaryBuffers.emplace_back(buffer2);

    auto buffer3 = CharArray::New();
    AddStringToBuffer("</DataArray>\n</Points>\n", buffer3);
    m_TemporaryBuffers.emplace_back(buffer3);
}

void VTUWriter::WriteCellsToBuffer(CellArray::Pointer Cells) {
    if (!Cells) return;

    int CellNum = Cells->GetNumberOfCells();
    if (CellNum == 0) return;

    // 创建三个 buffer：connectivity / offsets / types
    auto connectivityBuffer = CharArray::New();
    auto offsetsBuffer = CharArray::New();
    auto typesBuffer = CharArray::New();

    AddStringToBuffer("<Cells>\n", connectivityBuffer);
    AddStringToBuffer("<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">\n", connectivityBuffer);
    AddStringToBuffer("<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">\n", offsetsBuffer);
    AddStringToBuffer("<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n", typesBuffer);

    int offset = 0;
    for (int i = 0; i < CellNum; i++) {
        // 获取第 i 个单元的所有顶点索引
        igIndex vcnt = 0;
        igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};
        vcnt = Cells->GetCellIds(i, vhs);
        // connectivity 写入所有顶点索引
        for (int j = 0; j < vcnt; j++) { AddStringToBuffer(std::to_string(vhs[j]) + " ", connectivityBuffer); }
        AddStringToBuffer("\n", connectivityBuffer);

        // offsets 写入每个单元的结束索引
        offset += static_cast<int>(vcnt);
        AddStringToBuffer(std::to_string(offset) + "\n", offsetsBuffer);

        // types 写入每个单元类型
        auto CellsType = m_UnstructuredMesh->GetCellTypes();
        if (!CellNum) return;
        auto buffer = CharArray::New();

        igIndex vtkType;
        
        switch ((int) CellsType->GetValue(i)) {
            case IG_TRIANGLE:
                vtkType = VTKAbstractReader::TRIANGLE;
                break;
            case IG_QUAD:
                vtkType = VTKAbstractReader::QUAD;
                break;
            case IG_POLYGON:
                vtkType = VTKAbstractReader::POLYGON;
                break;
            case IG_TETRA:
                vtkType = VTKAbstractReader::TETRA;
                break;
            case IG_HEXAHEDRON:
                vtkType = VTKAbstractReader::HEXAHEDRON;
                break;
            case IG_PRISM:
                vtkType = VTKAbstractReader::WEDGE;
                break;
            case IG_PYRAMID:
                vtkType = VTKAbstractReader::PYRAMID;
                break;
            case IG_POLYHEDRON:
                vtkType = VTKAbstractReader::POLYHEDRON;
                break;
            default:
                vtkType = VTKAbstractReader::T0;
                break;
        }
        if (m_FileType == IGAME_ASCII) { AddStringToBuffer(std::to_string(vtkType) + " ", typesBuffer);
        } 
        
    }

    AddStringToBuffer("</DataArray>\n", connectivityBuffer);
    AddStringToBuffer("</DataArray>\n", offsetsBuffer);
    AddStringToBuffer("</DataArray>\n", typesBuffer);
    AddStringToBuffer("</Cells>\n", typesBuffer); // 用 connectivityBuffer 作为容器结束标签

    // 添加到临时缓冲区
    m_TemporaryBuffers.emplace_back(connectivityBuffer);
    m_TemporaryBuffers.emplace_back(offsetsBuffer);
    m_TemporaryBuffers.emplace_back(typesBuffer);
}



void VTUWriter::WritePointsAttributesToBuffer(AttributeSet::Pointer AttributeSet) {
    // TODO: 写入点属性
    if (!AttributeSet) return;

    auto buffer = CharArray::New();
    AddStringToBuffer("<PointData>\n", buffer);

    auto attributes = AttributeSet->GetAllAttributes();
    for (int i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto attr = attributes->GetElement(i);
        if (attr.GetAttachmentType() == IG_CELL) continue;
        std::string name = attr.pointer->GetName();
        int comp = attr.pointer->GetDimension(); // 维度
        AddStringToBuffer("<DataArray type=\"Float32\" Name=\"" + name + "\" NumberOfComponents=\"" +
                                  std::to_string(comp) + "\" format=\"ascii\">\n",
                          buffer);
        // 写入每个点的数据
        int nPoints = m_UnstructuredMesh->GetPoints()->GetNumberOfPoints();
        
        for (int j = 0; j < nPoints; ++j) {
            for (int k = 0; k < comp; ++k) {
                auto data = attr.pointer->GetValue(j*comp+k);
                AddStringToBuffer(std::to_string(data) + " ", buffer);
            }
            AddStringToBuffer("\n", buffer);
        }
        AddStringToBuffer("</DataArray>\n", buffer);
    }

    AddStringToBuffer("</PointData>\n", buffer);
    m_TemporaryBuffers.emplace_back(buffer);
}

void VTUWriter::WriteCellsAttributesToBuffer(AttributeSet::Pointer AttributeSet) {
    // TODO: 写入单元属性
    if (!AttributeSet) return;

    auto buffer = CharArray::New();
    AddStringToBuffer("<CellData>\n", buffer);

    auto attributes = AttributeSet->GetAllAttributes();
    for (int i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto attr = attributes->GetElement(i);
        if (attr.GetAttachmentType() == IG_POINT) continue;
        std::string name = attr.pointer->GetName();
        int comp = attr.pointer->GetDimension();
        AddStringToBuffer("<DataArray type=\"Float32\" Name=\"" + name + "\" NumberOfComponents=\"" +
                                  std::to_string(comp) + "\" format=\"ascii\">\n",
                          buffer);
        int nCells = m_UnstructuredMesh->GetCells()->GetNumberOfCells();
        for (int j = 0; j < nCells * comp; ++j) {
            
            for (int k = 0; k < comp; ++k) 
            { 
                auto data = attr.pointer->GetValue(j);
                AddStringToBuffer(std::to_string(data) + " ", buffer);
                j++;
            }
            AddStringToBuffer("\n", buffer);
        }
        AddStringToBuffer("</DataArray>\n", buffer);
    }

    AddStringToBuffer("</CellData>\n", buffer);
    AddStringToBuffer("</Piece>\n", buffer);
    AddStringToBuffer("</UnstructuredGrid>\n", buffer);
    AddStringToBuffer("</VTKFile>\n", buffer);
    m_TemporaryBuffers.emplace_back(buffer);
}

IGAME_NAMESPACE_END
