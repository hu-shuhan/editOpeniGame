#include "iGameIGCMWriter.h"

#include "iGameIGCWriter.h"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <tinyxml2.h>

IGAME_NAMESPACE_BEGIN

static std::string FormatIndex4(int index) {
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << index;
    return oss.str();
}

bool IGCMWriter::GenerateBuffers() {
    if (!m_DataObject) {
        return false;
    }
    if (!m_DataObject->HasSubDataObject()) {
        return false;
    }

    std::filesystem::path igcmPath(m_FilePath);
    const std::filesystem::path outputDir = igcmPath.parent_path();
    const std::string baseName = igcmPath.stem().string();
    if (baseName.empty()) {
        return false;
    }

    m_report.clear();

    tinyxml2::XMLDocument doc;
    doc.InsertFirstChild(doc.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    auto* root = doc.NewElement("IGCManifest");
    root->SetAttribute("type", "iGameMultiBlock");
    root->SetAttribute("version", "1.0");
    doc.InsertEndChild(root);

    auto* multiBlock = doc.NewElement("MultiBlock");
    root->InsertEndChild(multiBlock);

    auto* rootBlock = doc.NewElement("Block");
    rootBlock->SetAttribute("index", 0);
    std::string rootName = m_DataObject->GetName();
    if (rootName.empty()) {
        rootName = "Block_0";
    }
    rootBlock->SetAttribute("name", rootName.c_str());
    multiBlock->InsertEndChild(rootBlock);

    int leafPreorderIndex = 0;
    if (!WriteSubBlocksToDiskAndManifest(doc, rootBlock, m_DataObject, outputDir, baseName, leafPreorderIndex)) {
        return false;
    }
    if (leafPreorderIndex <= 0) {
        return false;
    }

    m_report.emplace_back("叶子块数量", std::to_string(leafPreorderIndex));

    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);

    auto buffer = CharArray::New();
    std::string xmlContent(printer.CStr());
    AddStringToBuffer(xmlContent, buffer);
    m_TemporaryBuffers.emplace_back(buffer);
    TransferBuffer();

    return true;
}

bool IGCMWriter::WriteSubBlocksToDiskAndManifest(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parentElem,
                                                DataObject::Pointer obj, const std::filesystem::path& outputDir,
                                                const std::string& baseName, int& leafPreorderIndex) {
    int siblingIndex = 0;
    for (auto it = obj->SubDataObjectIteratorBegin(); it != obj->SubDataObjectIteratorEnd(); ++it, ++siblingIndex) {
        auto child = it->second;
        if (!child) {
            return false;
        }

        if (child->HasSubDataObject()) {
            auto* blockElem = doc.NewElement("Block");
            blockElem->SetAttribute("index", siblingIndex);
            std::string name = child->GetName();
            if (name.empty()) {
                name = "Block_" + std::to_string(siblingIndex);
            }
            blockElem->SetAttribute("name", name.c_str());
            parentElem->InsertEndChild(blockElem);

            if (!WriteSubBlocksToDiskAndManifest(doc, blockElem, child, outputDir, baseName, leafPreorderIndex)) {
                return false;
            }
            continue;
        }

        const std::string idx4 = FormatIndex4(leafPreorderIndex);
        std::string displayName = child->GetName();
        if (displayName.empty()) {
            displayName = "block_" + idx4;
        }
        const std::string fileName = baseName + "__block_" + idx4 + ".igc";

        auto* dsElem = doc.NewElement("DataSet");
        dsElem->SetAttribute("index", siblingIndex);
        dsElem->SetAttribute("name", displayName.c_str());
        dsElem->SetAttribute("file", fileName.c_str());
        parentElem->InsertEndChild(dsElem);

        IGCWriter::Pointer writer = IGCWriter::New();
        if (m_hasCodecParams) {
            writer->SetCodecControlParams(m_CodecParams);
        }

        const std::filesystem::path leafPath = outputDir / fileName;
        if (!writer->WriteToFile(child, leafPath.string())) {
            return false;
        }

        AppendLeafReport(displayName, fileName, writer->GetReport());
        leafPreorderIndex++;
    }

    return true;
}

void IGCMWriter::AppendLeafReport(const std::string& displayName, const std::string& fileName,
                                 const std::vector<std::pair<std::string, std::string>>& leafReport) {
    if (leafReport.empty()) {
        return;
    }

    m_report.emplace_back("块 " + displayName, fileName);
    for (const auto& kv : leafReport) {
        m_report.emplace_back("  " + kv.first, kv.second);
    }
}

IGAME_NAMESPACE_END

