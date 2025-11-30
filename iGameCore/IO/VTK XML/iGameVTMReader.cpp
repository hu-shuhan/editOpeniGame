//
// Created by m_ky on 2024/7/21.
//

/**
 * @class   iGameVTMReader
 * @brief   iGameVTMReader's brief
 */

#include "iGameVTMReader.h"
#include "VTK/iGameVTKReader.h"
#include "iGameVTSReader.h"
#include "iGameVTUReader.h"
#include "CGNS/iGameCGNSReader.h"
#include <tinyxml2.h>


IGAME_NAMESPACE_BEGIN
bool iGameVTMReader::Parsing() {
    std::string fileDir = this->m_FilePath.substr(0, this->m_FilePath.find_last_of('/') + 1);
    const char* existAttribute;

    tinyxml2::XMLElement* vtkMultiBlockElem = FindTargetItem(root, "vtkMultiBlockDataSet");
    if (!vtkMultiBlockElem) { return false; }

    // Support both <Block> and <DataSet> formats
    tinyxml2::XMLElement* BlockElem = vtkMultiBlockElem->FirstChildElement("Block");
    tinyxml2::XMLElement* DataSetElem = vtkMultiBlockElem->FirstChildElement("DataSet");

    std::vector<DataObject::Pointer> overall_multiBlock;
    DataObject::Pointer newObj;

    // Handle <Block> format (nested structure)
    if (BlockElem) {
        while (BlockElem) {
            tinyxml2::XMLElement* elem = BlockElem->FirstChildElement();

            // 因为Scene中都用到了DrawObject的属性，后续多块要特殊处理
            // DataObject::Pointer curMultiBlock = DataObject::New();
            DataObject::Pointer curMultiBlock = DrawObject::New();

            while (elem) {
                // TODO: Not finish Reading the Nested block.
                //            std::string elemType(elem->Value());

                existAttribute = elem->Attribute("file");
                if (existAttribute) {
                    std::string fileName(existAttribute);
                    std::string fileSuffix;
                    const char* pos = strrchr(fileName.data(), '.');
                    if (pos != nullptr) {
                        const char* fileEnd = fileName.data() + fileName.size();
                        fileSuffix = std::string(pos + 1, fileEnd);
                    }
                    if (fileSuffix == "vts") {
                        iGameVTSReader::Pointer rd = iGameVTSReader::New();
                        rd->SetFilePath(fileDir + std::string(existAttribute));
                        rd->Execute();
                        newObj = rd->GetOutput();
                    } else if (fileSuffix == "vtu") {
                        iGameVTUReader::Pointer rd = iGameVTUReader::New();
                        rd->SetFilePath(fileDir + std::string(existAttribute));
                        rd->Execute();
                        newObj = rd->GetOutput();
                    } else if (fileSuffix == "vtk") {
                        VTKReader::Pointer rd = VTKReader::New();
                        rd->SetFilePath(fileDir + std::string(existAttribute));
                        rd->Execute();
                        newObj = rd->GetOutput();
                    }
                    #if defined(CGNS_ENABLE)
                    else if (fileSuffix == "cgns") {
                        iGameCGNSReader::Pointer reader = iGameCGNSReader::New();
                        newObj = reader->ReadFile(fileDir + std::string(existAttribute));
                    }
                    #endif
                    else {
                        newObj = nullptr;
                    }

                    if (newObj) {
                        // Set the sub-data object's name to the file's base name (without extension)
                        std::string baseName = fileName;
                        size_t slashPos = baseName.find_last_of("/\\");
                        if (slashPos != std::string::npos) baseName = baseName.substr(slashPos + 1);
                        size_t dotPos = baseName.find_last_of('.');
                        if (dotPos != std::string::npos) baseName = baseName.substr(0, dotPos);
                        newObj->SetName(baseName);
                        curMultiBlock->AddSubDataObject(newObj);
                    }
                }
                elem = elem->NextSiblingElement();
            }

            overall_multiBlock.push_back(curMultiBlock);
            BlockElem = BlockElem->NextSiblingElement("Block");
        }
    }
    // Handle <DataSet> format (flat structure)
    else if (DataSetElem) {
        DataObject::Pointer curMultiBlock = DrawObject::New();

        while (DataSetElem) {
            existAttribute = DataSetElem->Attribute("file");
            if (existAttribute) {
                std::string fileName(existAttribute);
                std::string fileSuffix;
                const char* pos = strrchr(fileName.data(), '.');
                if (pos != nullptr) {
                    const char* fileEnd = fileName.data() + fileName.size();
                    fileSuffix = std::string(pos + 1, fileEnd);
                }
                if (fileSuffix == "vts") {
                    iGameVTSReader::Pointer rd = iGameVTSReader::New();
                    rd->SetFilePath(fileDir + std::string(existAttribute));
                    rd->Execute();
                    newObj = rd->GetOutput();
                } else if (fileSuffix == "vtu") {
                    iGameVTUReader::Pointer rd = iGameVTUReader::New();
                    rd->SetFilePath(fileDir + std::string(existAttribute));
                    rd->Execute();
                    newObj = rd->GetOutput();
                } else if (fileSuffix == "vtk") {
                    VTKReader::Pointer rd = VTKReader::New();
                    rd->SetFilePath(fileDir + std::string(existAttribute));
                    rd->Execute();
                    newObj = rd->GetOutput();
                }
#if defined(CGNS_ENABLE)
                else if (fileSuffix == "cgns") {
                    iGameCGNSReader::Pointer reader = iGameCGNSReader::New();
                    newObj = reader->ReadFile(fileDir + std::string(existAttribute));
                }
#endif
                else {
                    newObj = nullptr;
                }

                if (newObj) {
                    // Set the sub-data object's name to the file's base name (without extension)
                    std::string baseName = fileName;
                    size_t slashPos = baseName.find_last_of("/\\");
                    if (slashPos != std::string::npos) baseName = baseName.substr(slashPos + 1);
                    size_t dotPos = baseName.find_last_of('.');
                    if (dotPos != std::string::npos) baseName = baseName.substr(0, dotPos);
                    newObj->SetName(baseName);
                    curMultiBlock->AddSubDataObject(newObj);
                }
            }
            DataSetElem = DataSetElem->NextSiblingElement("DataSet");
        }

        overall_multiBlock.push_back(curMultiBlock);
    }


    if (overall_multiBlock.empty()) {
        return false;
    } else if (overall_multiBlock.size() == 1) {
        parseData = overall_multiBlock.front();
        return true;
    } else {
        DataObject::Pointer rootDataObject = DataObject::New();
        for (const auto& mp: overall_multiBlock) rootDataObject->AddSubDataObject(mp);

        parseData = rootDataObject;
        return true;
    }
}

bool iGameVTMReader::CreateDataObject() {
    m_Output = parseData;

    return true;
}


IGAME_NAMESPACE_END
