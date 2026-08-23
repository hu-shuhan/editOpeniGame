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
#include "Log/iGameLogger.h"
#include <cstddef>
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

    std::size_t totalFileCount = 0;
    if (BlockElem) {
        for (auto* block = BlockElem; block; block = block->NextSiblingElement("Block")) {
            for (auto* elem = block->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
                if (elem->Attribute("file")) { ++totalFileCount; }
            }
        }
    } else {
        for (auto* dataSet = DataSetElem; dataSet; dataSet = dataSet->NextSiblingElement("DataSet")) {
            if (dataSet->Attribute("file")) { ++totalFileCount; }
        }
    }
    std::size_t currentFileCount = 0;
    IGAME_CORE_INFO("[VTM] Found {} referenced files in {}", totalFileCount, m_FilePath);

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
                    ++currentFileCount;
                    std::string fileName(existAttribute);
                    IGAME_CORE_INFO("[VTM] Reading file {}/{}: {}", currentFileCount, totalFileCount, fileName);
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
                ++currentFileCount;
                std::string fileName(existAttribute);
                IGAME_CORE_INFO("[VTM] Reading file {}/{}: {}", currentFileCount, totalFileCount, fileName);
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

    IGAME_CORE_INFO("[VTM] Finished reading {}/{} referenced files from {}",
                    currentFileCount,
                    totalFileCount,
                    m_FilePath);


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
