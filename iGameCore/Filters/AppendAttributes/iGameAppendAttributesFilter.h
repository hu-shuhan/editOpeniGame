#ifndef iGameAppendAttributesFilter_h
#define iGameAppendAttributesFilter_h

#include "iGameFilter.h"
#include "iGameDataObject.h"

#include <vector>

IGAME_NAMESPACE_BEGIN

class AppendAttributes : public Filter {
public:
    I_OBJECT(AppendAttributes);
    static Pointer New() { return new AppendAttributes; }

    /// 追加一个输入。
    void AddInput(DataObject::Pointer data);

    /// 是否追加点属性。
    void SetAppendPointData(bool enable);
    bool GetAppendPointData() const { return m_AppendPointData; }

    /// 是否追加单元属性。
    void SetAppendCellData(bool enable);
    bool GetAppendCellData() const { return m_AppendCellData; }

    /// 最近一次执行收集到的有效输入个数。
    int GetNumberOfCollectedInputs() const { return m_CollectedInputCount; }

    bool Execute() override;

protected:
    AppendAttributes();
    ~AppendAttributes() override = default;

private:
    /// 递归收集输入。
    void CollectInputs(DataObject::Pointer obj, std::vector<DataObject::Pointer>& out);

    /// 取数据对象的单元数；点集没有单元时返回 0。
    static IGsize GetCellCount(DataObject::Pointer obj);

    /// 按输入类型创建空的输出几何。
    static DataObject::Pointer CreateOutputGeometry(DataObject::Pointer src);

    /// 合并属性：把每个输入里的点/单元属性依次复制进 outAttrSet。
    /// 同名（且同归属）属性全部保留，靠后的输入留用原名，靠前的改名为 "<原名>_input_<模型序号>"。
    void MergeAttributes(const std::vector<DataObject::Pointer>& inputs, AttributeSet::Pointer outAttrSet);

    bool m_AppendPointData{true};
    bool m_AppendCellData{true};
    int m_CollectedInputCount{0};
};

IGAME_NAMESPACE_END
#endif
