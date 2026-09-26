#pragma once
#ifndef EOIGAME_IGAMECORE_ATTRIBUTE_IGAMEEXTRACTCOMPONENTFILTER_H
#define EOIGAME_IGAMECORE_ATTRIBUTE_IGAMEEXTRACTCOMPONENTFILTER_H

#include <iGameFilter.h>

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @class   ExtractComponentFilter
 * @brief   从多分量数组（向量 / 张量）中提取 1~3 个分量，生成同类型的数组
 *
 * 对齐 VTK vtkImageExtractComponents 的分量语义：默认提取第 0 个分量，也可以用
 * SetComponents(c1) / (c1, c2) / (c1, c2, c3) 一次提取多个分量，输出数组的维度等于提取的
 * 分量个数，类型与输入数组保持一致（Int / LongLong 等不会被隐式转成 double）。
 *
 * 输出为新的数据对象：几何（点、单元）与输入共享，属性集默认深拷贝输入属性（结果与输入完全
 * 解耦）再加上新数组，输入对象本身不被修改；GUI 会把结果作为独立节点加入模型树。
 *
 * 输入数组按「名字 + 挂载类型」定位：名字为空时取属性集中第一个匹配挂载限制的数组；挂载类型
 * （IG_POINT / IG_CELL）用于区分同名的点 / 单元数组。
 */
class ExtractComponentFilter : public Filter {
public:
    I_OBJECT(ExtractComponentFilter)

    static Pointer New() { return new ExtractComponentFilter; }

    bool Execute() override;

    // 输入数组名称，为空时使用属性集中第一个匹配挂载限制的数组（对应 DIME「可选」）
    void SetInputArrayName(const std::string& name) { m_InputArrayName = name; }
    const std::string& GetInputArrayName() const { return m_InputArrayName; }

    // 输入数组挂载类型限制（IG_POINT / IG_CELL）；IG_NONE 表示不限制。
    // 用于区分同名的点 / 单元数组。
    void SetInputAttachmentType(IGenum type) { m_InputAttachmentType = type; }
    IGenum GetInputAttachmentType() const { return m_InputAttachmentType; }

    // 输出数组名称，默认 "Result"；名字为空时 Execute 直接失败
    void SetOutputArrayName(const std::string& name) { m_OutputArrayName = name; }
    const std::string& GetOutputArrayName() const { return m_OutputArrayName; }

    // 要提取的分量索引，0 = X, 1 = Y, 2 = Z（等价于 SetComponents(comp)，单个分量）
    void SetComponent(int comp) { SetComponents(comp); }
    int GetComponent() const { return m_Components[0]; }

    // 一次提取 1~3 个分量；输出维度 = 分量个数（对齐 VTK vtkImageExtractComponents::SetComponents）
    void SetComponents(int c1);
    void SetComponents(int c1, int c2);
    void SetComponents(int c1, int c2, int c3);

    int GetNumberOfComponents() const { return m_NumberOfComponents; }
    const int* GetComponents() const { return m_Components; }

    // 结果属性集是否共享输入数组（只读），默认 false = 深拷贝。
    // 深拷贝让结果与输入完全解耦（输入后续被改动不会污染结果），代价是属性内存翻倍；
    // 大模型上更在意内存 / 耗时，可显式开启共享（结果只新增自己的数组，从不修改已有数组）。
    void SetShallowCopyAttributes(bool b) { m_ShallowCopyAttributes = b; }
    bool GetShallowCopyAttributes() const { return m_ShallowCopyAttributes; }

    const std::string& GetMessage() const { return m_Message; }

protected:
    ExtractComponentFilter();
    ~ExtractComponentFilter() override = default;

    std::string m_InputArrayName{};
    std::string m_OutputArrayName{"Result"};
    int m_Components[3]{0, 1, 2};
    int m_NumberOfComponents{1};
    IGenum m_InputAttachmentType{IG_NONE};
    bool m_ShallowCopyAttributes{false};
    std::string m_Message;
};

IGAME_NAMESPACE_END
#endif
