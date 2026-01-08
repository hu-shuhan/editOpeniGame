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

void StreamingData::TimeFrame::ClearCachedData() {
    m_CachedData.clear();
    m_CachedData.shrink_to_fit();
    m_IsCached = false;
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
    
    // 如果启用缓存且已缓存，更新LRU顺序并返回缓存数据
    if(m_Enable_Cache && currentFrame.GetISCached()) {
        // 更新LRU顺序：移动到列表头部
        auto it = m_LRUMap.find(index);
        if(it != m_LRUMap.end()) {
            m_LRUOrder.erase(it->second);
            m_LRUOrder.push_front(index);
            m_LRUMap[index] = m_LRUOrder.begin();
        }
        return currentFrame.GetCachedData();
    }
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
    if(m_Enable_Cache && !currentFrame.GetISCached()){
        // 如果缓存已满，淘汰最久未使用的缓存
        if(m_Cache_AllocatedNum >= m_Cache_MAXSize) {
            EvictLRUCache();
        }
        
        // 缓存当前帧数据
        if(m_Cache_AllocatedNum < m_Cache_MAXSize) {
            currentFrame.SetCache(target_time_data);
            m_Cache_AllocatedNum++;
            
            // 添加到LRU列表头部（最近使用）
            m_LRUOrder.push_front(index);
            m_LRUMap[index] = m_LRUOrder.begin();
        }
    }
    return target_time_data;
}


void StreamingData::ClearCache() {
    m_Cache_AllocatedNum = 0;
    m_LRUOrder.clear();
    m_LRUMap.clear();
    for(auto& timeFrames : m_Data){
        if(timeFrames.GetISCached()){
            timeFrames.ClearCachedData();
        }
    }
}

StreamingType StreamingData::GetTargetFrameType(unsigned int index) {
    assert(index < m_Data.size());
    return m_Data[index].GetFrameType();
}

void StreamingData::EnableCache(unsigned int maxCacheSize) {
    m_Enable_Cache = true;
    m_Cache_MAXSize = std::min((size_t)maxCacheSize, this->GetTimeNum());
}

void StreamingData::DisableCache() {
    m_Enable_Cache = false;
    m_Cache_MAXSize = 0;
    ClearCache();
}

void StreamingData::EvictLRUCache() {
    // 淘汰列表尾部的缓存（最久未使用的）
    if(m_LRUOrder.empty()) return;
    
    unsigned int oldestIndex = m_LRUOrder.back();
    m_LRUOrder.pop_back();
    m_LRUMap.erase(oldestIndex);
    
    auto& frame = GetTargetTimeFrame(oldestIndex);
    if(frame.GetISCached()) {
        frame.ClearCachedData();
        m_Cache_AllocatedNum--;
    }
}

float StreamingData::GetTargetTimeValue(unsigned int index) {
    assert(index < m_Data.size());
    return m_Data[index].GetTimeValue();
}


IGAME_NAMESPACE_END