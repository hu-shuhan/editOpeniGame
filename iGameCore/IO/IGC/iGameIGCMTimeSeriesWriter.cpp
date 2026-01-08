#include "iGameIGCMTimeSeriesWriter.h"

#include "iGameFileIO.h"
#include "iGameIGCWriter.h"
#include "Log/iGameLogger.h"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <tinyxml2.h>
#include <vector>

IGAME_NAMESPACE_BEGIN

static std::string FormatIndex4(int index) {
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << index;
    return oss.str();
}

void IGCMTimeSeriesWriter::CollectLeafParts(DataObject::Pointer obj, std::vector<DataObject::Pointer>& outParts) {
    if (!obj) {
        return;
    }
    if (!obj->HasSubDataObject()) {
        outParts.emplace_back(obj);
        return;
    }
    for (auto it = obj->SubDataObjectIteratorBegin(); it != obj->SubDataObjectIteratorEnd(); ++it) {
        CollectLeafParts(it->second, outParts);
    }
}

bool IGCMTimeSeriesWriter::GenerateBuffers() {
    if (!m_DataObject) {
        IGAME_CORE_ERROR("IGCMTimeSeriesWriter: 输入数据对象为空");
        return false;
    }

    auto timeFrames = m_DataObject->PeekTimeFrames();
    if (!timeFrames || timeFrames->GetTimeNum() <= 0) {
        IGAME_CORE_ERROR("IGCMTimeSeriesWriter: 未检测到 TimeFrames（可能不是序列数据）");
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
    root->SetAttribute("type", "iGameTimeSeries");
    root->SetAttribute("version", "1.0");
    doc.InsertEndChild(root);

    auto* timeSeries = doc.NewElement("TimeSeries");
    root->InsertEndChild(timeSeries);

    int totalParts = 0;
    const int frameCount = static_cast<int>(timeFrames->GetTimeNum());
    for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        const auto frameType = timeFrames->GetTargetFrameType(frameIndex);
        if (frameType != StreamingType::MultiSubFiles) {
            IGAME_CORE_ERROR("IGCMTimeSeriesWriter: 帧 {} 类型不支持，type={}", frameIndex, static_cast<int>(frameType));
            return false;
        }

        auto* frameElem = doc.NewElement("Frame");
        frameElem->SetAttribute("index", frameIndex);
        frameElem->SetAttribute("timestep", timeFrames->GetTargetTimeValue(frameIndex));
        timeSeries->InsertEndChild(frameElem);

        std::vector<DataObject::Pointer> frameParts;
        frameParts.reserve(8);

        auto appendLeafPartsFromObject = [&](DataObject::Pointer obj) -> bool {
            std::vector<DataObject::Pointer> leafParts;
            CollectLeafParts(obj, leafParts);
            if (leafParts.empty()) {
                return false;
            }
            for (auto& p : leafParts) {
                frameParts.emplace_back(p);
            }
            return true;
        };

        auto& frame = timeFrames->GetTargetTimeFrame(frameIndex);
        auto frameFiles = frame.GetMetaData();
        if (frameFiles && frameFiles->GetNumberOfElements() > 0) {
            const int inputCount = frameFiles->GetNumberOfElements();
            for (int i = 0; i < inputCount; ++i) {
                const auto& inputPath = frameFiles->GetElement(i);
                if (m_ProgressObserver) {
                    m_ProgressObserver->UpdateText(
                        "多帧读取 帧 " + std::to_string(frameIndex + 1) + "/" + std::to_string(frameCount) +
                        " 输入 " + std::to_string(i + 1) + "/" + std::to_string(inputCount));
                }
                auto partObj = FileIO::ReadFile(inputPath);
                if (!partObj) {
                    IGAME_CORE_ERROR("IGCMTimeSeriesWriter: 读取帧 {} 输入文件失败: {}", frameIndex, inputPath);
                    return false;
                }
                if (!appendLeafPartsFromObject(partObj)) {
                    IGAME_CORE_ERROR("IGCMTimeSeriesWriter: 收集帧 {} 输入文件叶子部件失败: {}", frameIndex, inputPath);
                    return false;
                }
            }
        } else {
            // 兜底：若该帧没有文件列表，则尝试通过切帧把数据组织到 subdataobj 后再编码
            m_DataObject->UpdateAnimation(frameIndex);
            if (m_DataObject->HasSubDataObject()) {
                for (auto it = m_DataObject->SubDataObjectIteratorBegin(); it != m_DataObject->SubDataObjectIteratorEnd(); ++it) {
                    if (!appendLeafPartsFromObject(it->second)) {
                        IGAME_CORE_ERROR("IGCMTimeSeriesWriter: 兜底切帧收集叶子部件失败，frameIndex={}", frameIndex);
                        return false;
                    }
                }
                m_DataObject->ClearSubDataObject();
            } else {
                if (!appendLeafPartsFromObject(m_DataObject)) {
                    IGAME_CORE_ERROR("IGCMTimeSeriesWriter: 兜底收集叶子部件失败（单对象），frameIndex={}", frameIndex);
                    return false;
                }
            }
        }

        const int partCount = static_cast<int>(frameParts.size());
        if (partCount <= 0) {
            IGAME_CORE_ERROR("IGCMTimeSeriesWriter: 帧 {} 未产生任何部件输出", frameIndex);
            return false;
        }

        const std::string frameIdx4 = FormatIndex4(frameIndex);
        for (int partIndex = 0; partIndex < partCount; ++partIndex) {
            auto part = frameParts[partIndex];
            if (!part) {
                return false;
            }

            if (m_ProgressObserver) {
                m_ProgressObserver->UpdateText(
                    "多帧编码 帧 " + std::to_string(frameIndex + 1) + "/" + std::to_string(frameCount) +
                    " 部件 " + std::to_string(partIndex + 1) + "/" + std::to_string(partCount));
            }

            const std::string fileName = [&] {
                if (partCount == 1) {
                    return baseName + "__frame_" + frameIdx4 + ".igc";
                }
                const std::string partIdx4 = FormatIndex4(partIndex);
                return baseName + "__frame_" + frameIdx4 + "__part_" + partIdx4 + ".igc";
            }();

            auto* dsElem = doc.NewElement("DataSet");
            dsElem->SetAttribute("index", partIndex);
            std::string displayName = part->GetName();
            if (!displayName.empty()) {
                dsElem->SetAttribute("name", displayName.c_str());
            }
            dsElem->SetAttribute("file", fileName.c_str());
            frameElem->InsertEndChild(dsElem);

            IGCWriter::Pointer writer = IGCWriter::New();
            if (m_hasCodecParams) {
                writer->SetCodecControlParams(m_CodecParams);
            }

            const std::filesystem::path partPath = outputDir / fileName;
            if (!writer->WriteToFile(part, partPath.string())) {
                IGAME_CORE_ERROR("IGCMTimeSeriesWriter: 写出失败，frameIndex={}, partIndex={}, out={}", frameIndex, partIndex,
                                 partPath.string());
                return false;
            }

            AppendPartReport(frameIndex, partIndex, displayName, fileName, writer->GetReport());
        }

        totalParts += partCount;
    }

    m_report.emplace_back("帧数", std::to_string(frameCount));
    m_report.emplace_back("部件总数", std::to_string(totalParts));

    if (m_ProgressObserver) {
        m_ProgressObserver->UpdateText("");
    }

    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);

    auto buffer = CharArray::New();
    std::string xmlContent(printer.CStr());
    AddStringToBuffer(xmlContent, buffer);
    m_TemporaryBuffers.emplace_back(buffer);
    TransferBuffer();

    return true;
}

void IGCMTimeSeriesWriter::AppendPartReport(int frameIndex, int partIndex, const std::string& displayName,
                                           const std::string& fileName,
                                           const std::vector<std::pair<std::string, std::string>>& partReport) {
    if (partReport.empty()) {
        return;
    }

    std::string title = "帧 " + std::to_string(frameIndex) + " 部件 " + std::to_string(partIndex);
    if (!displayName.empty()) {
        title += " " + displayName;
    }
    m_report.emplace_back(title, fileName);
    for (const auto& kv : partReport) {
        m_report.emplace_back("  " + kv.first, kv.second);
    }
}

IGAME_NAMESPACE_END
