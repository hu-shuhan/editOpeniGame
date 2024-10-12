//
// Created by m_ky on 2024/7/27.
//

/**
 * @class   iGameStreamingData
 * @brief   iGameStreamingData's brief
 */
#pragma once

#include "iGameStringArray.h"
#include "iGameElementArray.h"

IGAME_NAMESPACE_BEGIN
class StreamingData : public Object{
public:
    I_OBJECT(StreamingData)
    struct TimeFrame{
        float timeValue{-1};
        StringArray::Pointer SubFileNames{nullptr};

        TimeFrame()= default;

        TimeFrame(float _t,  StringArray::Pointer f_names) : timeValue(_t), SubFileNames(std::move(f_names)){}
    };

    static Pointer New() { return new StreamingData; }

    void AddTimeStep(float timeVal, const StringArray::Pointer& f_names)
    {
        m_Data.emplace_back(timeVal, f_names);
    }


    TimeFrame& GetTargetTimeFrame(unsigned int index) { return m_Data[index]; }
    const TimeFrame& GetTargetTimeFrame(unsigned int  index) const { return m_Data[index]; }

//    DataArray* GetAttribute(IGenum type, IGenum attachmentType, const std::string& name)
//    {
//        for (int i = 0; i < this->m_Data.size(); i++)
//        {
//            if (m_Data[i].active && m_Data[i].attachmentType == attachmentType && m_Data[i].array->GetName() == name)
//            {
//                return m_Data[i].array.get();
//            }
//        }
//        return nullptr;
//    }
//
//    void DeleteAttribute(int i)
//    {
//        if (i < 0 || i >= m_Data.size())
//        {
//            return;
//        }
//        m_Data[i].active = false;
//        m_Data[i].array = nullptr;
//    }
    size_t GetTimeNum(){return m_Data.size();}

    std::vector<TimeFrame>& GetArrays() {
        return m_Data;
    }

protected:

    StreamingData() = default;
    ~StreamingData() = default;
    std::vector<TimeFrame> m_Data;
};

IGAME_NAMESPACE_END