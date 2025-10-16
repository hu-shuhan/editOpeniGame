#include "iGameVTSWriter.h"
#include <sstream>
#include <tinyxml2.h>

IGAME_NAMESPACE_BEGIN

bool VTSWriter::GenerateBuffers() {
    if (!m_StructuredMesh) return false;

    WriteHeaderToBuffer();
    WritePointsToBuffer(m_StructuredMesh->GetPoints());
    WritePointAttributesToBuffer(m_StructuredMesh->GetAttributeSet());
    WriteCellAttributesToBuffer(m_StructuredMesh->GetAttributeSet());

    this->TransferBuffer();
    return true;
}

void VTSWriter::WriteHeaderToBuffer() {
    auto buffer = CharArray::New();


    auto dims = m_StructuredMesh->GetDimensionSize(); // 假设 StructuredMesh 提供维度获取方法

    std::string data = "<VTKFile type=\"StructuredGrid\" version=\"1.0\" byte_order=\"LittleEndian\">\n";
    data += "<StructuredGrid WholeExtent=\"0 " + std::to_string(dims[0] - 1) + " 0 " + std::to_string(dims[1] - 1) +
            " 0 " + std::to_string(dims[2] - 1) + "\">\n";
    data += "<Piece Extent=\"0 " + std::to_string(dims[0] - 1) + " 0 " + std::to_string(dims[1] - 1) + " 0 " +
            std::to_string(dims[2] - 1) + "\">\n";

    AddStringToBuffer(data, buffer);
    m_TemporaryBuffers.emplace_back(buffer);
}

void VTSWriter::WritePointsToBuffer(Points::Pointer Points) {
    if (!Points) return;

    auto buffer = CharArray::New();
    AddStringToBuffer("<Points>\n<DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">\n", buffer);
    m_TemporaryBuffers.emplace_back(buffer);

    auto buffer2 = CharArray::New();
    int numPoints = Points->GetNumberOfPoints();
    for (int i = 0; i < numPoints; i++) {
        auto p = Points->GetPoint(i);
        AddStringToBuffer(std::to_string(p[0]) + " " + std::to_string(p[1]) + " " + std::to_string(p[2]) + "\n",
                          buffer2);
    }
    m_TemporaryBuffers.emplace_back(buffer2);

    auto buffer3 = CharArray::New();
    AddStringToBuffer("</DataArray>\n</Points>\n", buffer3);
    m_TemporaryBuffers.emplace_back(buffer3);
}

void VTSWriter::WritePointAttributesToBuffer(AttributeSet::Pointer attrSet) {
    if (!attrSet) return;
    auto buffer = CharArray::New();
    AddStringToBuffer("<PointData>\n", buffer);

    int numAttr = attrSet->GetNumberOfAttributes();
    for (int i = 0; i < numAttr; i++) {
        auto attr = attrSet->GetAttribute(i);
        if (attr.GetAttachmentType() == IG_CELL) continue;
        int comp = attr.pointer->GetDimension();

        AddStringToBuffer("<DataArray type=\"Float32\" Name=\"" + attr.pointer->GetName() + "\" NumberOfComponents=\"" +
                                  std::to_string(comp) + "\" format=\"ascii\">\n",
                          buffer);
        int numPoints = attr.pointer->GetNumberOfElements();
        for (int j = 0; j < numPoints; j++) {
            for (int k = 0; k < comp; k++) {
                AddStringToBuffer(std::to_string(attr.pointer->GetValue(j * comp + k)) + " ", buffer);
            }
            AddStringToBuffer("\n", buffer);
        }
        AddStringToBuffer("</DataArray>\n", buffer);
    }

    AddStringToBuffer("</PointData>\n", buffer);
    m_TemporaryBuffers.emplace_back(buffer);
}

void VTSWriter::WriteCellAttributesToBuffer(AttributeSet::Pointer attrSet) {
    if (!attrSet) return;
    auto buffer = CharArray::New();
    AddStringToBuffer("<CellData>\n", buffer);

    int numAttr = attrSet->GetNumberOfAttributes();
    for (int i = 0; i < numAttr; i++) {
        auto attr = attrSet->GetAttribute(i);
        if (attr.GetAttachmentType() == IG_POINT) continue;
        int comp = attr.pointer->GetDimension();

        AddStringToBuffer("<DataArray type=\"Float32\" Name=\"" + attr.pointer->GetName() + "\" NumberOfComponents=\"" +
                                  std::to_string(comp) + "\" format=\"ascii\">\n",
                          buffer);
        int numPoints = attr.pointer->GetNumberOfElements();
        for (int j = 0; j < numPoints; j++) {
            for (int k = 0; k < comp; k++) {
                AddStringToBuffer(std::to_string(attr.pointer->GetValue(j * comp + k)) + " ", buffer);
            }
            AddStringToBuffer("\n", buffer);
        }
        AddStringToBuffer("</DataArray>\n", buffer);
    }

    AddStringToBuffer("</CellData>\n", buffer);
    AddStringToBuffer("</Piece>\n</StructuredGrid>\n</VTKFile>\n", buffer);
    m_TemporaryBuffers.emplace_back(buffer);
}

IGAME_NAMESPACE_END
