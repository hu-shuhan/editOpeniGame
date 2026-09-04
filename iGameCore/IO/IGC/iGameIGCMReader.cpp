#include "iGameIGCMReader.h"

#include "iGameFileIO.h"
#include "iGameFileSystem.h"
#include "iGameDrawObject.h"

#include <algorithm>
#include <cfloat>
#include <cstring>
#include <filesystem>
#include <string>
#include <tinyxml2.h>
#include <vector>

IGAME_NAMESPACE_BEGIN

static bool IsSafeRelativePath(const std::string& file) {
    if (file.empty()) {
        return false;
    }

    std::filesystem::path p = FileSystem::PathFromUtf8(file);

    // 拒绝绝对路径/盘符/UNC 等形式
    if (p.is_absolute() || p.has_root_name() || p.has_root_directory()) {
        return false;
    }

    // 拒绝路径穿越
    for (const auto& part : p) {
        if (part == "..") {
            return false;
        }
    }

    return true;
}

bool IGCMReader::Parsing() {
    if (!root) {
        return false;
    }

    m_dataSetCount = 0;

    if (std::strcmp(root->Value(), "IGCManifest") != 0) {
        return false;
    }

    const char* typeAttr = root->Attribute("type");
    const char* versionAttr = root->Attribute("version");
    if (!versionAttr || std::strcmp(versionAttr, "1.0") != 0) {
        return false;
    }

    const std::filesystem::path manifestDir = FileSystem::PathFromUtf8(m_FilePath).parent_path();

    if (!typeAttr) {
        return false;
    }
    if (std::strcmp(typeAttr, "iGameMultiBlock") == 0) {
        return ParseMultiBlock(manifestDir);
    }
    if (std::strcmp(typeAttr, "iGameTimeSeries") == 0) {
        return ParseTimeSeries(manifestDir);
    }
    return false;
}

bool IGCMReader::ParseMultiBlock(const std::filesystem::path& manifestDir) {
    auto* multiBlockElem = root->FirstChildElement("MultiBlock");
    if (!multiBlockElem) {
        return false;
    }

    std::vector<DataObject::Pointer> topNodes;
    if (!ParseChildren(multiBlockElem, manifestDir, topNodes)) {
        return false;
    }

    if (topNodes.empty()) {
        return false;
    }
    if (m_dataSetCount <= 0) {
        return false;
    }

    if (topNodes.size() == 1) {
        parseData = topNodes.front();
        return true;
    }

    auto rootObj = DrawObject::New();
    for (const auto& node : topNodes) {
        rootObj->AddSubDataObject(node);
    }
    parseData = rootObj;
    return true;
}

bool IGCMReader::ParseTimeSeries(const std::filesystem::path& manifestDir) {
    auto* timeSeriesElem = root->FirstChildElement("TimeSeries");
    if (!timeSeriesElem) {
        return false;
    }

    struct ElemInfo {
        tinyxml2::XMLElement* elem{};
        bool hasIndex{false};
        int index{0};
        int documentOrder{0};
    };

    std::vector<ElemInfo> frames;
    int docOrder = 0;
    for (auto* elem = timeSeriesElem->FirstChildElement(); elem; elem = elem->NextSiblingElement(), ++docOrder) {
        const char* value = elem->Value();
        if (!value) {
            return false;
        }
        if (std::strcmp(value, "Frame") != 0) {
            return false;
        }

        ElemInfo info;
        info.elem = elem;
        info.documentOrder = docOrder;
        int idx = 0;
        if (elem->QueryIntAttribute("index", &idx) == tinyxml2::XML_SUCCESS) {
            info.hasIndex = true;
            info.index = idx;
        }
        frames.emplace_back(info);
    }

    if (frames.empty()) {
        return false;
    }

    std::stable_sort(frames.begin(), frames.end(), [](const ElemInfo& a, const ElemInfo& b) {
        if (a.hasIndex && b.hasIndex) {
            return a.index < b.index;
        }
        if (a.hasIndex != b.hasIndex) {
            return a.hasIndex; // 有 index 的优先
        }
        return a.documentOrder < b.documentOrder;
    });

    auto rootObj = DrawObject::New();

    for (const auto& frame : frames) {
        auto* frameElem = frame.elem;
        if (!frameElem) {
            return false;
        }

        float timeVal = 0.0f;
        if (frameElem->Attribute("timestep")) {
            if (frameElem->QueryFloatAttribute("timestep", &timeVal) != tinyxml2::XML_SUCCESS) {
                return false;
            }
        }

        std::vector<ElemInfo> dataSets;
        int dsDocOrder = 0;
        for (auto* elem = frameElem->FirstChildElement(); elem; elem = elem->NextSiblingElement(), ++dsDocOrder) {
            const char* value = elem->Value();
            if (!value) {
                return false;
            }
            if (std::strcmp(value, "DataSet") != 0) {
                return false;
            }

            ElemInfo info;
            info.elem = elem;
            info.documentOrder = dsDocOrder;
            int idx = 0;
            if (elem->QueryIntAttribute("index", &idx) == tinyxml2::XML_SUCCESS) {
                info.hasIndex = true;
                info.index = idx;
            }
            dataSets.emplace_back(info);
        }

        if (dataSets.empty()) {
            return false;
        }

        std::stable_sort(dataSets.begin(), dataSets.end(), [](const ElemInfo& a, const ElemInfo& b) {
            if (a.hasIndex && b.hasIndex) {
                return a.index < b.index;
            }
            if (a.hasIndex != b.hasIndex) {
                return a.hasIndex; // 有 index 的优先
            }
            return a.documentOrder < b.documentOrder;
        });

        auto files = StringArray::New();
        for (const auto& ds : dataSets) {
            const char* fileAttr = ds.elem->Attribute("file");
            if (!fileAttr) {
                return false;
            }

            std::string relFile(fileAttr);
            if (!IsSafeRelativePath(relFile)) {
                return false;
            }

            std::filesystem::path igcPath = manifestDir / FileSystem::PathFromUtf8(relFile);
            files->AddElement(FileSystem::PathToUtf8(igcPath));
            m_dataSetCount++;
        }

        rootObj->GetTimeFrames()->AddTimeStep(timeVal, files, StreamingType::MultiSubFiles);
    }

    if (rootObj->GetTimeFrames()->GetTimeNum() <= 0) {
        return false;
    }
    if (m_dataSetCount <= 0) {
        return false;
    }

    auto& firstFrame = rootObj->GetTimeFrames()->GetTargetTimeFrame(0);
    auto firstFrameFiles = firstFrame.GetMetaData();
    if (!firstFrameFiles || firstFrameFiles->GetNumberOfElements() <= 0) {
        return false;
    }

    for (int i = 0; i < firstFrameFiles->GetNumberOfElements(); ++i) {
        auto partObj = FileIO::ReadFile(firstFrameFiles->GetElement(i));
        if (!partObj) {
            return false;
        }
        rootObj->AddSubDataObject(partObj);
    }

    bool attributesExist = rootObj->HasSubDataObject() && rootObj->SubDataObjectIteratorBegin()->second->GetAttributeSet();
    if (attributesExist) {
        auto firstAttrSet = rootObj->SubDataObjectIteratorBegin()->second->GetAttributeSet();
        for (int k = 0; k < firstAttrSet->GetAllAttributes()->GetNumberOfElements(); ++k) {
            auto attribute = firstAttrSet->GetAttribute(k);
            int dim = attribute.pointer->GetDimension();
            int attachmentType = attribute.attachmentType;

            DoubleArray::Pointer array = DoubleArray::New();
            array->SetName(attribute.pointer->GetName());
            array->SetDimension(dim);

            DoubleArray::Pointer parentDataRange = DoubleArray::New();
            parentDataRange->SetDimension(2);
            parentDataRange->Resize(dim + 1);
            for (int j = 0; j < dim + 1; ++j) {
                parentDataRange->SetElement(j, {DBL_MIN, DBL_MAX});
            }

            switch (attribute.type) {
                case IG_SCALAR:
                    rootObj->GetAttributeSet()->AddScalar(attachmentType, array, parentDataRange);
                    break;
                case IG_VECTOR:
                    rootObj->GetAttributeSet()->AddVector(attachmentType, array, parentDataRange);
                    break;
                default:
                    break;
            }
        }

        rootObj->ReCollectSubDataObjectDataRange();
        rootObj->UpdateSubDataObjectDataRange();
    }

    parseData = rootObj;
    return true;
}

bool IGCMReader::ParseChildren(tinyxml2::XMLElement* parentElem, const std::filesystem::path& manifestDir,
                              std::vector<DataObject::Pointer>& outNodes) {
    struct ChildInfo {
        tinyxml2::XMLElement* elem{};
        bool hasIndex{false};
        int index{0};
        int documentOrder{0};
    };

    std::vector<ChildInfo> children;
    int docOrder = 0;
    for (auto* elem = parentElem->FirstChildElement(); elem; elem = elem->NextSiblingElement(), ++docOrder) {
        const char* value = elem->Value();
        if (!value) {
            return false;
        }
        if (std::strcmp(value, "Block") != 0 && std::strcmp(value, "DataSet") != 0) {
            return false;
        }

        ChildInfo info;
        info.elem = elem;
        info.documentOrder = docOrder;
        int idx = 0;
        if (elem->QueryIntAttribute("index", &idx) == tinyxml2::XML_SUCCESS) {
            info.hasIndex = true;
            info.index = idx;
        }
        children.emplace_back(info);
    }

    std::stable_sort(children.begin(), children.end(), [](const ChildInfo& a, const ChildInfo& b) {
        if (a.hasIndex && b.hasIndex) {
            return a.index < b.index;
        }
        if (a.hasIndex != b.hasIndex) {
            return a.hasIndex; // 有 index 的优先
        }
        return a.documentOrder < b.documentOrder;
    });

    outNodes.clear();
    outNodes.reserve(children.size());
    for (const auto& child : children) {
        DataObject::Pointer node;
        if (!ParseElement(child.elem, manifestDir, node)) {
            return false;
        }
        if (!node) {
            return false;
        }
        outNodes.emplace_back(node);
    }

    return true;
}

bool IGCMReader::ParseElement(tinyxml2::XMLElement* elem, const std::filesystem::path& manifestDir,
                             DataObject::Pointer& outObj) {
    outObj = nullptr;

    const char* value = elem->Value();
    if (!value) {
        return false;
    }

    if (std::strcmp(value, "Block") == 0) {
        auto blockObj = DrawObject::New();
        const char* nameAttr = elem->Attribute("name");
        if (nameAttr) {
            blockObj->SetName(std::string(nameAttr));
        }

        std::vector<DataObject::Pointer> childNodes;
        if (!ParseChildren(elem, manifestDir, childNodes)) {
            return false;
        }
        for (const auto& node : childNodes) {
            blockObj->AddSubDataObject(node);
        }

        outObj = blockObj;
        return true;
    }

    if (std::strcmp(value, "DataSet") == 0) {
        const char* fileAttr = elem->Attribute("file");
        if (!fileAttr) {
            return false;
        }

        std::string relFile(fileAttr);
        if (!IsSafeRelativePath(relFile)) {
            return false;
        }

        std::filesystem::path igcPath = manifestDir / FileSystem::PathFromUtf8(relFile);
        auto dataObj = FileIO::ReadFile(FileSystem::PathToUtf8(igcPath));
        if (!dataObj) {
            return false;
        }

        const char* nameAttr = elem->Attribute("name");
        if (nameAttr) {
            dataObj->SetName(std::string(nameAttr));
        }

        outObj = dataObj;
        m_dataSetCount++;
        return true;
    }

    return false;
}

bool IGCMReader::CreateDataObject() {
    if (!parseData) {
        return false;
    }
    m_Output = parseData;
    return true;
}

IGAME_NAMESPACE_END
