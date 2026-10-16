#pragma once
#include <iGameFilter.h>
#include <iGameDataObject.h>

IGAME_NAMESPACE_BEGIN

class iGameGenerateIdsFilter : public Filter {
public:
    I_OBJECT(iGameGenerateIdsFilter);
    static Pointer New(IGenum dataType) { return new iGameGenerateIdsFilter(dataType); }

    /**
     * @brief 执行滤波:在输入对象之外生成独立输出(深拷贝几何与属性后再挂载 Id 数组),
     *        输入数据对象保持不变,输出可作为新节点加入模型树/场景。
     */
    bool Execute() override;

    void SetArrayName(const std::string& name) { m_ArrayName = name; }
    void SetStartId(long long start) { m_StartId = start; }

private:
    bool Run();

    /** 构建独立输出对象(深拷贝输入的几何与属性),失败返回 nullptr。 */
    DataObject::Pointer BuildIndependentOutput(DataObject::Pointer input);

protected:
    iGameGenerateIdsFilter(IGenum dataType);
    ~iGameGenerateIdsFilter() override = default;

private:
    IGenum m_DataType{}; // IG_POINT 或 IG_CELL

    std::string m_ArrayName{"Ids"};
    long long m_StartId{0};
};

IGAME_NAMESPACE_END
