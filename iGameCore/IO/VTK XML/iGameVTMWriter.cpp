#include "iGameVTMWriter.h"
#include "iGameVTSWriter.h"
#include "iGameVTUWriter.h"
#include <filesystem>
#include <tinyxml2.h>

IGAME_NAMESPACE_BEGIN

bool VTMWriter::GenerateBuffers() {
    if (!m_DataObject) return false;

    std::string fileDir = std::filesystem::path(m_FilePath).parent_path().string() + "/";

    tinyxml2::XMLDocument doc;

    // VTM 根节点
    auto* root = doc.NewElement("VTKFile");
    root->SetAttribute("type", "vtkMultiBlockDataSet");
    root->SetAttribute("version", "1.0");
    root->SetAttribute("byte_order", "LittleEndian");
    doc.InsertFirstChild(root);

    // MultiBlockDataSet 节点
    auto* multiBlock = doc.NewElement("vtkMultiBlockDataSet");
    root->InsertEndChild(multiBlock);

    // 写子块，直接把 DataObject 的结构写到 multiBlock 下面
    WriteSubBlocks(doc, multiBlock, m_DataObject, fileDir);

    // 打印到 buffer
    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);

    auto buffer = CharArray::New();
    std::string xmlContent(printer.CStr());
    AddStringToBuffer(xmlContent, buffer);
    m_TemporaryBuffers.emplace_back(buffer);
    TransferBuffer();

    return true; // 最终由 FileWriter::WriteToFile 调用 SaveBufferDataToFile()
}

void VTMWriter::WriteSubBlocks(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parentElem, DataObject::Pointer obj,
                               const std::string& fileDir) {
    int blockIndex = 0;
    if (obj->GetDataObjectType() == IG_MULTIBLOCK_MESH) {
        for (auto it = obj->SubDataObjectIteratorBegin(); it != obj->SubDataObjectIteratorEnd(); ++it, ++blockIndex) {
            auto child = it->second;

            if (child->HasSubDataObject()) {
                // 子容器 -> Block
                auto* blockElem = doc.NewElement("Block");
                blockElem->SetAttribute("index", blockIndex);
                blockElem->SetAttribute("name", ("Block_" + std::to_string(blockIndex)).c_str());
                parentElem->InsertEndChild(blockElem);

                // 递归写子对象
                WriteSubBlocks(doc, blockElem, child, fileDir);

            } else if (child->GetDataObjectType() == IG_STRUCTURED_MESH) {
                // StructuredMesh -> DataSet
                std::string fileName = "block_" + std::to_string(blockIndex) + ".vts";
                auto writer = VTSWriter::New();
                writer->SetInput(DynamicCast<StructuredMesh>(child));
                writer->SetFilePath(fileDir + fileName);
                writer->WriteToFile();

                auto* dsElem = doc.NewElement("DataSet");
                dsElem->SetAttribute("index", blockIndex);
                dsElem->SetAttribute("name", ("dom-" + std::to_string(blockIndex)).c_str());
                dsElem->SetAttribute("file", fileName.c_str());
                parentElem->InsertEndChild(dsElem);

            } else if (child->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
                // UnstructuredMesh -> DataSet
                std::string fileName = "block_" + std::to_string(blockIndex) + ".vtu";
                auto writer = VTUWriter::New();
                writer->SetInput(DynamicCast<UnstructuredMesh>(child));
                writer->SetFilePath(fileDir + fileName);
                writer->WriteToFile();

                auto* dsElem = doc.NewElement("DataSet");
                dsElem->SetAttribute("index", blockIndex);
                dsElem->SetAttribute("name", ("dom-" + std::to_string(blockIndex)).c_str());
                dsElem->SetAttribute("file", fileName.c_str());
                parentElem->InsertEndChild(dsElem);
            }
        }
    } else {
        if (obj->GetDataObjectType() == IG_STRUCTURED_MESH) {
            // StructuredMesh -> DataSet
            std::string fileName = "block_" + std::to_string(blockIndex) + ".vts";
            auto writer = VTSWriter::New();
            writer->SetInput(DynamicCast<StructuredMesh>(obj));
            writer->SetFilePath(fileDir + fileName);
            writer->WriteToFile();
            auto* dsElem = doc.NewElement("DataSet");
            dsElem->SetAttribute("index", blockIndex);
            dsElem->SetAttribute("name", ("dom-" + std::to_string(blockIndex)).c_str());
            dsElem->SetAttribute("file", fileName.c_str());
            parentElem->InsertEndChild(dsElem);
        } else if (obj->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
            // UnstructuredMesh -> DataSet
            std::string fileName = "block_" + std::to_string(blockIndex) + ".vtu";
            auto writer = VTUWriter::New();
            writer->SetInput(DynamicCast<UnstructuredMesh>(obj));
            writer->SetFilePath(fileDir + fileName);
            writer->WriteToFile();
            auto* dsElem = doc.NewElement("DataSet");
            dsElem->SetAttribute("index", blockIndex);
            dsElem->SetAttribute("name", ("dom-" + std::to_string(blockIndex)).c_str());
            dsElem->SetAttribute("file", fileName.c_str());
            parentElem->InsertEndChild(dsElem);
        }
    }
}

IGAME_NAMESPACE_END
