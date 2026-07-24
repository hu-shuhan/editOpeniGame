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
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <limits>
IGAME_NAMESPACE_BEGIN

namespace {

DataObject::Pointer ReadStreamingFrameFile(const std::string& fileName) {
    auto extension = std::filesystem::path(fileName).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (extension == ".vtu") {
        auto reader = iGameVTUReader::New();
        reader->SetUpdateProgressIndependent(true);
        reader->SetFilePath(fileName);
        return reader->Execute() ? reader->GetOutput() : DataObject::Pointer{};
    }
    return FileIO::ReadFile(fileName);
}

} // namespace

StreamingData::~StreamingData() = default;
bool iGame::StreamingData::TimeFrame::SetCache(StreamingFrameCacheEntry cache) {
    if(m_IsCached) return false;
    m_CachedData = std::move(cache.data);
    m_CachedResource = std::move(cache.resource);
    m_IsCached = true;
    return true;
}

StreamingFrameCacheEntry StreamingData::TimeFrame::GetCacheEntry() const {
    return StreamingFrameCacheEntry{
        .data = m_CachedData,
        .resource = m_CachedResource,
    };
}

StreamingFrameCacheEntry StreamingData::TimeFrame::TakeCacheEntry() {
    StreamingFrameCacheEntry entry{
        .data = std::move(m_CachedData),
        .resource = std::move(m_CachedResource),
    };
    m_IsCached = false;
    return entry;
}

void StreamingData::TimeFrame::ClearCachedData() {
    (void)TakeCacheEntry();
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

    StreamingFrameCacheEntry cachedEntry;
    if (FindCachedFrame(index, cachedEntry)) {
        return std::move(cachedEntry.data);
    }
    if (m_FrameProvider != nullptr) {
        auto provided = m_FrameProvider->RequestFrame(index);
        m_Cache_AllocatedNum.store(static_cast<unsigned int>(m_FrameProvider->CachedFrameCount()));
        return provided;
    }
    auto frameData = currentFrame.GetMetaData();
    std::vector<iGame::Object::Pointer> target_time_data;
    /* If the timeframe data store MultiSubFile's Path, the job is to Parse the sub File. */
    if(currentFrame.GetFrameType() == StreamingType::MultiSubFiles)
    {
        std::vector<std::future<iGame::DataObject::Pointer>> tasks;
        std::vector<iGame::DataObject::Pointer> results(frameData->GetNumberOfElements());
        if(frameData->GetNumberOfElements() == 1){
            results[0] = ReadStreamingFrameFile(frameData->GetElement(0));
        }
        else
        {
            for (int i = 0; i < frameData->GetNumberOfElements(); i++)
            {
                tasks.emplace_back(ThreadPool::Instance()->Commit([i, &results](const std::string& fileName){
                    DataObject::Pointer newObj = ReadStreamingFrameFile(fileName);
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
    if(m_Enable_Cache && m_FrameProvider == nullptr && !currentFrame.GetISCached()){
        (void)StoreCachedFrame(index, StreamingFrameCacheEntry{.data = target_time_data});
    }
    return target_time_data;
}


void StreamingData::ClearCache() {
    if (m_FrameProvider != nullptr) { m_FrameProvider->ClearCachedFrames(); }
    std::vector<StreamingFrameCacheEntry> released;
    {
        std::lock_guard<std::mutex> lock(m_CacheMutex);
        m_Cache_AllocatedNum.store(0u);
        m_LRUOrder.clear();
        m_LRUMap.clear();
        for(auto& timeFrames : m_Data){
            if(timeFrames.GetISCached()){
                released.push_back(timeFrames.TakeCacheEntry());
            }
        }
    }
}

StreamingType StreamingData::GetTargetFrameType(unsigned int index) {
    assert(index < m_Data.size());
    return m_Data[index].GetFrameType();
}

void StreamingData::NotifyFramePresented(const unsigned int index) {
    if (index >= m_Data.size() || m_FrameProvider == nullptr) { return; }
    m_FrameProvider->NotifyFramePresented(index);
    m_Cache_AllocatedNum.store(static_cast<unsigned int>(m_FrameProvider->CachedFrameCount()));
}

void StreamingData::EnableCache(unsigned int maxCacheSize) {
    m_Enable_Cache = true;
    const auto maximumBufferedFrames = GetTimeNum() > 0u ? GetTimeNum() - 1u : 0u;
    m_Cache_MAXSize = static_cast<unsigned int>(std::min<std::size_t>(
        maxCacheSize, maximumBufferedFrames));
    if (m_FrameProvider != nullptr) {
        m_FrameProvider->ConfigureCacheCapacity(m_Cache_MAXSize);
        m_Cache_AllocatedNum.store(static_cast<unsigned int>(m_FrameProvider->CachedFrameCount()));
    }
}

void StreamingData::DisableCache() {
    m_Enable_Cache = false;
    m_Cache_MAXSize = 0;
    if (m_FrameProvider != nullptr) {
        m_FrameProvider->ConfigureCacheCapacity(0u);
    }
    ClearCache();
}

void StreamingData::SetFrameProvider(IStreamingFrameProvider::Pointer provider) {
    m_FrameProvider = std::move(provider);
    m_Cache_AllocatedNum.store(m_FrameProvider != nullptr
        ? static_cast<unsigned int>(m_FrameProvider->CachedFrameCount())
        : 0u);
}

bool StreamingData::FindCachedFrame(
        const unsigned int index,
        StreamingFrameCacheEntry& entry) {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    if (!m_Enable_Cache || index >= m_Data.size() || !m_Data[index].GetISCached()) { return false; }
    TouchLRULocked(index);
    entry = m_Data[index].GetCacheEntry();
    return true;
}

bool StreamingData::StoreCachedFrame(
        const unsigned int index,
        StreamingFrameCacheEntry entry,
        const bool mostRecent) {
    std::vector<StreamingFrameCacheEntry> released;
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    if (!m_Enable_Cache || index >= m_Data.size() || entry.data.empty()) { return false; }
    auto& frame = m_Data[index];
    if (frame.GetISCached()) {
        released.push_back(frame.TakeCacheEntry());
        const auto iterator = m_LRUMap.find(index);
        if (iterator != m_LRUMap.end()) {
            m_LRUOrder.erase(iterator->second);
            m_LRUMap.erase(iterator);
        }
    }
    const auto residentLimit = static_cast<std::size_t>(m_Cache_MAXSize) + 1u;
    while (m_LRUMap.size() >= residentLimit && !m_LRUOrder.empty()) {
        EvictLRUCacheLocked(released);
    }
    if (!frame.SetCache(std::move(entry))) { return false; }
    if (mostRecent) {
        m_LRUOrder.push_front(index);
        m_LRUMap[index] = m_LRUOrder.begin();
    } else {
        m_LRUOrder.push_back(index);
        auto position = m_LRUOrder.end();
        --position;
        m_LRUMap[index] = position;
    }
    m_Cache_AllocatedNum.store(static_cast<unsigned int>(m_LRUMap.size()));
    return true;
}

bool StreamingData::EraseCachedFrame(const unsigned int index) {
    StreamingFrameCacheEntry released;
    {
        std::lock_guard<std::mutex> lock(m_CacheMutex);
        if (index >= m_Data.size() || !m_Data[index].GetISCached()) { return false; }
        released = m_Data[index].TakeCacheEntry();
        const auto iterator = m_LRUMap.find(index);
        if (iterator != m_LRUMap.end()) {
            m_LRUOrder.erase(iterator->second);
            m_LRUMap.erase(iterator);
        }
        m_Cache_AllocatedNum.store(static_cast<unsigned int>(m_LRUMap.size()));
    }
    return true;
}

std::vector<unsigned int> StreamingData::CachedFrameIndices() const {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    std::vector<unsigned int> indices;
    indices.reserve(m_LRUMap.size());
    for (const auto& [index, position] : m_LRUMap) {
        (void)position;
        indices.push_back(index);
    }
    std::sort(indices.begin(), indices.end());
    return indices;
}

std::uint64_t StreamingData::CachedResidentSizeHint() const {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    std::uint64_t bytes = 0u;
    for (const auto& frame : m_Data) {
        if (!frame.GetISCached()) { continue; }
        const auto resource = frame.GetCachedResource();
        if (resource == nullptr) { continue; }
        const auto residentBytes = resource->ResidentSizeHint();
        bytes = residentBytes > std::numeric_limits<std::uint64_t>::max() - bytes
            ? std::numeric_limits<std::uint64_t>::max()
            : bytes + residentBytes;
    }
    return bytes;
}

void StreamingData::TouchLRULocked(const unsigned int index) {
    const auto iterator = m_LRUMap.find(index);
    if (iterator != m_LRUMap.end()) { m_LRUOrder.erase(iterator->second); }
    m_LRUOrder.push_front(index);
    m_LRUMap[index] = m_LRUOrder.begin();
}

void StreamingData::EvictLRUCacheLocked(std::vector<StreamingFrameCacheEntry>& released) {
    if(m_LRUOrder.empty()) return;

    const unsigned int oldestIndex = m_LRUOrder.back();
    m_LRUOrder.pop_back();
    m_LRUMap.erase(oldestIndex);

    auto& frame = GetTargetTimeFrame(oldestIndex);
    if(frame.GetISCached()) {
        released.push_back(frame.TakeCacheEntry());
    }
    m_Cache_AllocatedNum.store(static_cast<unsigned int>(m_LRUMap.size()));
}

float StreamingData::GetTargetTimeValue(unsigned int index) {
    assert(index < m_Data.size());
    return m_Data[index].GetTimeValue();
}


IGAME_NAMESPACE_END
