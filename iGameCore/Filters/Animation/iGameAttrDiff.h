#pragma once
#include "iGameDataObject.h"
#include "iGameFilter.h"

IGAME_NAMESPACE_BEGIN
class iGameAttrDiff : public Filter {
public:
    I_OBJECT(iGameAttrDiff);
    static Pointer New() { return new iGameAttrDiff; }

    void SetAttributeByName(const std::string& name) { m_AttrName = name; }
    void SetAttributeByIndex(int index) {
        m_AttrIndex = index;
        m_AttrName.clear();
    }
    void SetDiffMode(int mode) { m_DiffMode = mode; }
    void SetComponent(int component) { m_component = component; }
    void SetOutputName(const std::string& name) { m_outputName = name; }
    void SetFrameIndex(int index) { m_FrameIndex = index; }
    std::string GetMessage() const { return m_Message; }

    bool Execute() override;

protected:
    iGameAttrDiff();
    ~iGameAttrDiff() override = default;

private:
    bool ComputeFrame(DataObject::Pointer obj, StreamingData::Pointer frames, unsigned int frameIndex);
    // 把某一帧数据整合为AttributeSet,useMounted表示是否已挂载
    std::vector<AttributeSet::Pointer> CollectFrameAttrs(DataObject::Pointer obj, StreamingData::Pointer frames,
                                                         unsigned int index, bool useMounted);
    // 具体计算属性差值
    bool ApplyDiffToObject(AttributeSet::Pointer curAttrs, AttributeSet::Pointer prevAttrs, unsigned int frameIndex);
    static DoubleArray::Pointer ComputeRange(ArrayObject::Pointer arr);
    static double GetAttrValue(ArrayObject::Pointer arr, IGsize i, int dim, int component);
    // 异步更新，刷新模型树右侧属性列表
    void SyncParentAttribute(DataObject::Pointer obj, const std::string& outName);

    std::string m_AttrName{};   // 属性按名称解析
    int m_AttrIndex{-1};        // 属性按索引解析
    int m_DiffMode{0};          // 0: 符号差值，1: 绝对值差值，2：相对变化率
    int m_component{-1};        // -1：取模，0：x分量，1：y分量，2：z分量
    std::string m_outputName{}; // 输出新属性名
    int m_FrameIndex{-1};       // -1: 全帧模式，0~N: 单帧模式
    std::string m_Message{};    // 错误信息
};
IGAME_NAMESPACE_END
