#include <Convert/iGameConvertToSurfaceMeshFilter.h>
#include <OBJ/iGameOBJWriter.h>
IGAME_NAMESPACE_BEGIN


bool OBJWriter::GenerateBuffers() {
    switch (m_DataObject->GetDataObjectType()) {
        case IG_SURFACE_MESH:
            m_SurfaceMesh = DynamicCast<SurfaceMesh>(m_DataObject);
            break;
        case IG_UNSTRUCTURED_MESH: {
            auto converter = iGame::ConvertToSurfaceMeshFilter::New();
            converter->SetInput(m_DataObject);
            converter->Execute();
            m_SurfaceMesh = converter->GetSurfaceMesh();
            break;
        }
        default:
            return false;
    }
    if (!m_SurfaceMesh) { return false; }
    m_Buffers.resize(2, nullptr);
    WritePointsToBuffer(m_Buffers[0]);
    WriteFacesToBuffer(m_Buffers[1]);
    return true;
}

bool OBJWriter::WriteToMemory(DataObject::Pointer dataObject, std::vector<uint8_t>& outData) {
    SetDataObject(dataObject);
    m_DataObject = dataObject;  // 确保m_DataObject被设置
    if (!GenerateBuffers()) { return false; }

    size_t totalSize = 0;
    for (const auto& buf : m_Buffers) {
        if (buf) totalSize += buf->GetNumberOfValues();
    }

    outData.resize(totalSize);
    size_t offset = 0;
    for (const auto& buf : m_Buffers) {
        if (buf && buf->GetNumberOfValues() > 0) {
            size_t n = buf->GetNumberOfValues();
            std::memcpy(outData.data() + offset, buf->RawPointer(), n);
            offset += n;
        }
    }
    m_Buffers.clear();
    return true;
}

const void OBJWriter::WritePointsToBuffer(CharArray::Pointer& buffer) {
    if (buffer == nullptr) { buffer = CharArray::New(); }
    auto Points = m_SurfaceMesh->GetPoints();
    int VertexNum = Points->GetNumberOfPoints();
    std::string data;
    Point p;
    for (int i = 0; i < VertexNum; i++) {
        p = Points->GetPoint(i);
        buffer->AddValue('v');
        for (int j = 0; j < 3; j++) {
            buffer->AddValue(' ');
            data = std::to_string(p[j]);
            AddStringToBuffer(data, buffer);
        }
        buffer->AddValue('\n');
    }
}
const void OBJWriter::WriteFacesToBuffer(CharArray::Pointer& buffer) {
    if (buffer == nullptr) { buffer = CharArray::New(); }
    auto Faces = m_SurfaceMesh->GetFaces();
    int FaceNum = m_SurfaceMesh->GetNumberOfFaces();
    igIndex vhs[IGAME_CELL_MAX_SIZE];
    for (int i = 0; i < FaceNum; i++) {
        auto fn = Faces->GetCellIds(i, vhs);
        buffer->AddValue('f');
        for (int j = 0; j < fn; j++) {
            buffer->AddValue(' ');
            std::string data = std::to_string(vhs[j] + 1);
            AddStringToBuffer(data, buffer);
        }
        buffer->AddValue('\n');
    }
}


IGAME_NAMESPACE_END
