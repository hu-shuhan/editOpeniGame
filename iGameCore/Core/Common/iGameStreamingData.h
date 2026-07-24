//
// Created by m_ky on 2024/7/27.
//

/**
 * @class   iGameStreamingData
 * @brief   iGameStreamingData's brief
 */
#pragma once

#include <utility>
#include <atomic>
#include <cstdint>
#include <list>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "iGameStringArray.h"
#include "iGameElementArray.h"

IGAME_NAMESPACE_BEGIN

class IStreamingFrameProvider {
public:
    using Pointer = std::shared_ptr<IStreamingFrameProvider>;

    virtual ~IStreamingFrameProvider() = default;
    [[nodiscard]] virtual std::vector<Object::Pointer> RequestFrame(unsigned int ordinal) = 0;
    virtual void NotifyFramePresented(unsigned int ordinal) = 0;
    virtual void ConfigureCacheCapacity(unsigned int) {}
    virtual void ClearCachedFrames() = 0;
    [[nodiscard]] virtual std::size_t CachedFrameCount() const = 0;
};

class IStreamingFrameCacheResource {
public:
    using Pointer = std::shared_ptr<IStreamingFrameCacheResource>;

    virtual ~IStreamingFrameCacheResource() = default;
    [[nodiscard]] virtual std::uint64_t ResidentSizeHint() const noexcept = 0;
};

struct StreamingFrameCacheEntry {
    std::vector<Object::Pointer> data;
    IStreamingFrameCacheResource::Pointer resource;
};

class StreamingData : public Object{
public:
    I_OBJECT(StreamingData)

    /*Internal data class in Streaming Data */
    class TimeFrame{
        protected:
            float m_TimeValue{-1};
            StringArray::Pointer m_metaData{nullptr};
            StreamingType m_type {StreamingType::NONE};
            bool m_IsCached {false};

        private:
            std::vector<iGame::Object::Pointer> m_CachedData {};
            IStreamingFrameCacheResource::Pointer m_CachedResource;

        public:
            TimeFrame()= default;
            TimeFrame(float _t,  StringArray::Pointer f_names, StreamingType _type)
                    : m_TimeValue(_t), m_metaData(std::move(f_names)), m_type(_type){}
            TimeFrame(float _t,  StringArray::Pointer f_names) : m_TimeValue(_t), m_metaData(std::move(f_names)), m_type(StreamingType::NONE){}

            float GetTimeValue() const {return m_TimeValue;}
            StringArray::Pointer GetMetaData() const {return m_metaData;}
            StreamingType GetFrameType() const {return m_type;}

            std::vector<iGame::Object::Pointer> GetCachedData();

            void SetCachedStatus(bool cached);

            bool GetISCached() const {return m_IsCached;}
            bool SetCache(StreamingFrameCacheEntry cache);
            [[nodiscard]] StreamingFrameCacheEntry GetCacheEntry() const;
            [[nodiscard]] StreamingFrameCacheEntry TakeCacheEntry();
            [[nodiscard]] IStreamingFrameCacheResource::Pointer GetCachedResource() const {
                return m_CachedResource;
            }

            // 清除缓存数据
            void ClearCachedData();
    };

    static Pointer New() { return new StreamingData; }

    /*添加动画关键帧
     * Add animation keyframes
     * */
    void AddTimeStep(float timeVal, StringArray::Pointer f_names, StreamingType type);
    /*获得指定时间帧的数据类型：目前有：多个子文件、单个场数据类型*/
    StreamingType GetTargetFrameType(unsigned int index);
    /*获得指定时间帧的数据*/
    std::vector<iGame::Object::Pointer> GetTargetTimeFrameData(unsigned int index);
    void NotifyFramePresented(unsigned int index);
    /*获得指定时间的时间值*/
    float GetTargetTimeValue(unsigned int index);
    /*获取时间帧总数*/
    size_t GetTimeNum(){return m_Data.size();}
    /*获取时间帧数组，未来可删*/
    std::vector<TimeFrame>& GetArrays() { return m_Data;}

    /* Cache Stuff */
    /*开启动画缓存
     * @param maxCacheSize 当前帧之外允许保留的缓存帧数
     */
    void EnableCache(unsigned int maxCacheSize);
    /*禁用动画缓存功能，同时清除缓存*/
    void DisableCache();
    /*清除缓存*/
    void ClearCache();

    /*获取当前缓存帧数*/
    unsigned int GetCurrentCacheCount() const { return m_Cache_AllocatedNum.load(); }
    /*获取播放缓存是否开启*/
    bool IsCacheEnabled() const { return m_Enable_Cache; }
    /*获取最大缓存帧数*/
    unsigned int GetMaxCacheSize() const { return m_Cache_MAXSize; }

    void SetFrameProvider(IStreamingFrameProvider::Pointer provider);

    [[nodiscard]] bool FindCachedFrame(unsigned int index, StreamingFrameCacheEntry& entry);
    [[nodiscard]] bool StoreCachedFrame(
        unsigned int index,
        StreamingFrameCacheEntry entry,
        bool mostRecent = true);
    [[nodiscard]] bool EraseCachedFrame(unsigned int index);
    [[nodiscard]] std::vector<unsigned int> CachedFrameIndices() const;
    [[nodiscard]] std::uint64_t CachedResidentSizeHint() const;

    TimeFrame& GetTargetTimeFrame(unsigned int index);
    const TimeFrame& GetTargetTimeFrame(unsigned int index) const;

protected:

    StreamingData() = default;
    ~StreamingData();
    std::vector<TimeFrame> m_Data;

    bool m_Enable_Cache{false};
    unsigned int m_Cache_MAXSize{0};
    std::atomic_uint m_Cache_AllocatedNum{0};

    // LRU缓存管理
    std::list<unsigned int> m_LRUOrder;  // LRU顺序列表（存储帧索引，头部为最近使用）
    std::unordered_map<unsigned int, std::list<unsigned int>::iterator> m_LRUMap; // 帧索引到LRU位置的映射
    mutable std::mutex m_CacheMutex;
    IStreamingFrameProvider::Pointer m_FrameProvider;

    /*淘汰最久未使用的缓存（列表尾部）*/
    void EvictLRUCacheLocked(std::vector<StreamingFrameCacheEntry>& released);
    void TouchLRULocked(unsigned int index);
};

IGAME_NAMESPACE_END
