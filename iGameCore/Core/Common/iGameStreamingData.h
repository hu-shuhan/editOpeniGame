//
// Created by m_ky on 2024/7/27.
//

/**
 * @class   iGameStreamingData
 * @brief   iGameStreamingData's brief
 */
#pragma once

#include <utility>
#include <list>
#include <unordered_map>

#include "iGameStringArray.h"
#include "iGameElementArray.h"
IGAME_NAMESPACE_BEGIN


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

        public:
            TimeFrame()= default;
            TimeFrame(float _t,  StringArray::Pointer f_names, StreamingType _type)
                    : m_TimeValue(_t), m_metaData(std::move(f_names)), m_type(_type){}
            TimeFrame(float _t,  StringArray::Pointer f_names) : m_TimeValue(_t), m_metaData(std::move(f_names)), m_type(StreamingType::NONE){}

            float GetTimeValue(){return m_TimeValue;}
            StringArray::Pointer GetMetaData(){return m_metaData;}
            StreamingType GetFrameType(){return m_type;}

            std::vector<iGame::Object::Pointer> GetCachedData();

            void SetCachedStatus(bool cached);

            bool GetISCached() const {return m_IsCached;}
            bool SetCache(std::vector<iGame::Object::Pointer> cache);

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
    /*获得指定时间的时间值*/
    float GetTargetTimeValue(unsigned int index);
    /*获取时间帧总数*/
    size_t GetTimeNum(){return m_Data.size();}
    /*获取时间帧数组，未来可删*/
    std::vector<TimeFrame>& GetArrays() { return m_Data;}

    /* Cache Stuff */
    /*开启动画缓存
     * @param maxCacheSize 最大缓存帧数，超过此数量将淘汰最久未使用的缓存
     */
    void EnableCache(unsigned int maxCacheSize);
    /*禁用动画缓存功能，同时清除缓存*/
    void DisableCache();
    /*清除缓存*/
    void ClearCache();

    /*获取当前缓存帧数*/
    unsigned int GetCurrentCacheCount() const { return m_Cache_AllocatedNum; }
    /*获取最大缓存帧数*/
    unsigned int GetMaxCacheSize() const { return m_Cache_MAXSize; }

    TimeFrame& GetTargetTimeFrame(unsigned int index);
    const TimeFrame& GetTargetTimeFrame(unsigned int index) const;

protected:

    StreamingData() = default;
    ~StreamingData() = default;
    std::vector<TimeFrame> m_Data;

    bool m_Enable_Cache{false};
    unsigned int m_Cache_MAXSize{0};
    unsigned int m_Cache_AllocatedNum{0};

    // LRU缓存管理
    std::list<unsigned int> m_LRUOrder;  // LRU顺序列表（存储帧索引，头部为最近使用）
    std::unordered_map<unsigned int, std::list<unsigned int>::iterator> m_LRUMap; // 帧索引到LRU位置的映射

    /*淘汰最久未使用的缓存（列表尾部）*/
    void EvictLRUCache();
};

IGAME_NAMESPACE_END