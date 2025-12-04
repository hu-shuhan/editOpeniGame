/**
 * @class   iGameStreamingData
 * @brief   iGameStreamingData's brief
 */

#include "iGameStreamingData.h"
#include "iGameDataObject.h"
#include "iGameThreadPool.h"
#include "iGameFileIO.h"
#include "Abaqus/iGameODBReader.h"
#include "VTK XML/iGameVTSReader.h"
#include "VTK XML/iGameVTUReader.h"
#include "VTK XML/iGameVTMReader.h"
#include "VTK XML/iGamePVDReader.h"
#include "VTK/iGameVTKReader.h"

#include <future>
IGAME_NAMESPACE_BEGIN
bool iGame::StreamingData::TimeFrame::SetCache(std::vector<iGame::Object::Pointer> cache) {
    if(m_IsCached) return false;
    m_CachedData = cache;
    m_IsCached = true;
    return true;
}

std::vector<iGame::Object::Pointer> StreamingData::TimeFrame::GetCachedData() {
    return m_CachedData;
}

void StreamingData::TimeFrame::SetCachedStatus(bool cached) {
    m_IsCached = cached;
}


void StreamingData::AddTimeStep(float timeVal, StringArray::Pointer f_names, StreamingType type) {
    m_Data.emplace_back(timeVal, f_names, type);
}

StreamingData::TimeFrame &StreamingData::GetTargetTimeFrame(unsigned int index){
    return m_Data[index];
}

const StreamingData::TimeFrame &StreamingData::GetTargetTimeFrame(unsigned int index) const {
    return m_Data[index];
}

std::vector<iGame::Object::Pointer> StreamingData::GetTargetTimeFrameData(unsigned int index) {
    auto& currentFrame = GetTargetTimeFrame(index);
    if(m_Enable_Cache && currentFrame.GetISCached()) return currentFrame.GetCachedData();
    auto frameData = currentFrame.GetMetaData();
    std::vector<iGame::Object::Pointer> target_time_data;
    /* If the timeframe data store MultiSubFile's Path, the job is to Parse the sub File. */
    if(currentFrame.GetFrameType() == StreamingType::MultiSubFiles)
    {
        std::vector<std::future<iGame::DataObject::Pointer>> tasks;
        std::vector<iGame::DataObject::Pointer> results(frameData->GetNumberOfElements());
        if(frameData->GetNumberOfElements() == 1){
            results[0] = FileIO::ReadFile(frameData->GetElement(0));
        }
        else
        {
            for (int i = 0; i < frameData->GetNumberOfElements(); i++)
            {
                tasks.emplace_back(ThreadPool::Instance()->Commit([i, &results](const std::string& fileName){
                    //                DataObject::Pointer newObj = FileIO::ReadFile(fileName);
                    DataObject::Pointer newObj;
                    const char* pos = strrchr(fileName.data(), '.');
                    std::string fileSuffix;
                    const char *fileEnd = fileName.data() + fileName.size();
                    fileSuffix = std::string(pos + 1, fileEnd);
                    if(fileSuffix == "vts"){
                        iGameVTSReader::Pointer rd = iGameVTSReader::New();
                        rd->SetFilePath(fileName);
                        rd->Execute();
                        newObj = rd->GetOutput();
                    }
                    else if(fileSuffix == "vtu"){
                        iGameVTUReader::Pointer rd = iGameVTUReader::New();
                        rd->SetUpdateProgressIndependent(true);
                        rd->SetFilePath(fileName);
                        rd->Execute();
                        newObj = rd->GetOutput();
                    } else if(fileSuffix == "pvd"){
                        iGamePVDReader::Pointer rd = iGamePVDReader::New();
                        rd->SetFilePath(fileName);
                        rd->Execute();
                        newObj = rd->GetOutput();
                    }
                    results[i] = newObj;
                    return newObj;
                }, frameData->GetElement(i)));
            }

            //            tasks.emplace_back(iGame::ThreadPool::Instance()->Commit(
            //                    [i, &results](const std::string &fileName) {
            //                        auto res = FileIO::ReadFile(fileName);
            //                        results[i] = res;
            //                        return res;
            //                    },
            //                    frameData->GetElement(i)));

            //        for(int i = 0; i < frameData->GetNumberOfElements(); i ++){
            //            results[i] = FileIO::ReadFile(frameData->GetElement(i));
            //        }
            for (auto& task: tasks) {
                task.get();
            }
        }
        for(auto& subObj : results){
            target_time_data.emplace_back(subObj);
        }
    } /* If the timeframe data store SingleField Attributes' Path,
    *   the job is to Parse the target File's Field Attribute replace of the original one. */
    else if(currentFrame.GetFrameType() == StreamingType::SingleFieldAttributes)
    {
        const auto& filePath = currentFrame.GetMetaData()->GetElement(0);
        auto fileType = FileIO::GetFileType(filePath);
        if(fileType == FileIO::FileType::ODB){
#if defined(AbqSDK_ENABLE)
            ODBReader::Pointer reader = ODBReader::New();
            auto attributeSet = reader->ReadOdbFieldData(filePath, index);
            target_time_data.emplace_back(attributeSet);
#endif
        }
    }
    if(m_Enable_Cache && m_Cache_AllocatedNum < m_Cache_MAXSize && !currentFrame.GetISCached()){
        m_Cache_AllocatedNum ++;
        currentFrame.SetCache(target_time_data);
    }
    return target_time_data;
}


void StreamingData::ClearCache() {
    m_Cache_AllocatedNum = 0;
    for(auto timeFrames : m_Data){
        if(timeFrames.GetISCached()){
            timeFrames.GetCachedData().clear();
            timeFrames.SetCachedStatus(false);
        }
    }
}

StreamingType StreamingData::GetTargetFrameType(unsigned int index) {
    assert(index < m_Data.size());
    return m_Data[index].GetFrameType();
}

void StreamingData::EnableCache(unsigned int maxCacheSize) {
    m_Enable_Cache = true;
    m_Cache_MAXSize = std::max(maxCacheSize, (uint32_t)m_Data.size());
}

void StreamingData::DisableCache() {
    m_Enable_Cache = false;
    m_Cache_MAXSize = 0;
    ClearCache();
}

float StreamingData::GetTargetTimeValue(unsigned int index) {
    assert(index < m_Data.size());
    return m_Data[index].GetTimeValue();
}


IGAME_NAMESPACE_END