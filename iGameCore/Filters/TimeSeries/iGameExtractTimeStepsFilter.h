#pragma once

#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameStreamingData.h>

#include <vector>

IGAME_NAMESPACE_BEGIN


class ExtractTimeStepsFilter : public Filter {
public:
    I_OBJECT(ExtractTimeStepsFilter);
    static Pointer New() { return new ExtractTimeStepsFilter; }

    // 模式
    enum SelectionMode {
        SELECT_TIME_STEPS = 0, // 按索引列表保留
        SELECT_TIME_RANGE = 1  // 按索引区间 + 步长保留
    };


    void SetSelectionMode(SelectionMode mode) { m_SelectionMode = mode; }
    SelectionMode GetSelectionMode() const { return m_SelectionMode; }

    // 索引列表（0 起）；空列表 = 全部时间步都保留
    void SetTimeStepIndices(const std::vector<int>& indices) { m_TimeStepIndices = indices; }
    const std::vector<int>& GetTimeStepIndices() const { return m_TimeStepIndices; }

    // 索引区间（闭区间）+ 步长
    void SetTimeStepRange(int begin, int end) {
        m_RangeBegin = begin;
        m_RangeEnd = end;
    }
    void SetTimeStepRange(const int range[2]) {
        m_RangeBegin = range[0];
        m_RangeEnd = range[1];
    }
    void SetTimeStepInterval(int interval) { m_TimeStepInterval = interval; } // <= 0 视为 1

    int GetNumberOfKeptTimeSteps() const { return static_cast<int>(m_KeptIndices.size()); }
    const std::vector<int>& GetKeptTimeStepIndices() const { return m_KeptIndices; }
    std::vector<float> GetKeptTimeValues() const { return m_KeptTimeValues; }

    bool Execute() override;

protected:
    ExtractTimeStepsFilter();
    ~ExtractTimeStepsFilter() override = default;

private:
    // 规范化索引：越界丢弃 -> 去重 -> 升序；结果为空时返回全部索引
    std::vector<int> BuildKeptIndices(int timeStepCount) const;

    // 新建与输入同类型的输出对象并共享几何 / 属性（只搬指针，不复制数据）
    DataObject::Pointer CreateOutputLike(const DataObject::Pointer& input);

    // 新建时间轴：把保留帧的时间值 / 元数据 / 帧类型 / 已缓存数据搬过去
    StreamingData::Pointer BuildOutputTimeFrames(const StreamingData::Pointer& inputFrames,
                                                const std::vector<int>& keptIndices);

    SelectionMode m_SelectionMode{SELECT_TIME_STEPS};

    std::vector<int> m_TimeStepIndices;

    int m_RangeBegin{0};
    int m_RangeEnd{-1};
    int m_TimeStepInterval{1};

    std::vector<int> m_KeptIndices;
    std::vector<float> m_KeptTimeValues;
};

IGAME_NAMESPACE_END
