#pragma once

#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameStreamingData.h>

#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * 时间平移 / 缩放。
 *
 * 对时间轴做一次线性变换：t' = (t + PreShift) * Scale + PostShift。
 * 输入需带时间序列（DataObject::GetTimeFrames() 非空）；输出为同类型的独立对象，
 * 只重写每帧的时间值，帧数 / 元数据 / 帧类型 / 缓存状态与几何、属性都保持不变。
 */
class TemporalShiftScaleFilter : public Filter {
public:
    I_OBJECT(TemporalShiftScaleFilter);
    static Pointer New() { return new TemporalShiftScaleFilter; }

    /* 参数：t' = (t + PreShift) * Scale + PostShift */
    void SetPreShift(float shift) { m_PreShift = shift; }
    void SetPostShift(float shift) { m_PostShift = shift; }
    void SetScale(float scale) { m_Scale = scale; }

    float GetPreShift() const { return m_PreShift; }
    float GetPostShift() const { return m_PostShift; }
    float GetScale() const { return m_Scale; }

    /* 执行结果查询 */
    int GetNumberOfTimeSteps() const { return static_cast<int>(m_InTimeValues.size()); }
    const std::vector<float>& GetInTimeValues() const { return m_InTimeValues; }
    const std::vector<float>& GetOutTimeValues() const { return m_OutTimeValues; }

    bool Execute() override;

protected:
    TemporalShiftScaleFilter();
    ~TemporalShiftScaleFilter() override = default;

private:
    // 新建与输入同类型的输出对象，并只读共享几何 / 属性（只搬指针，不复制数据）
    DataObject::Pointer CreateOutputLike(const DataObject::Pointer& input);

    float m_PreShift{0.0f};
    float m_PostShift{0.0f};
    float m_Scale{1.0f};

    std::vector<float> m_InTimeValues;  // 变换前的时间值
    std::vector<float> m_OutTimeValues; // 变换后的时间值
};

IGAME_NAMESPACE_END
