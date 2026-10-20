/**
 * @class   WarpByVectorFilter
 * @brief   按向量场对几何做形变(Warp by vector)。
 *
 * 新坐标 = 原坐标 + scale * 向量(逐点计算),输出为**独立数据对象**,
 * 输入模型保持不变(与 iGameGenerateIdsFilter 的输出约定一致)。
 *
 * 向量场从输入的 AttributeSet 中按数组名获取(优先点关联数组),
 * 取属性方式参考 Filters/Deformation;几何拷贝与类型化属性拷贝参考
 * Filters/GenerateIds 的实现。
 *
 * 说明:
 *   - 包围盒无需手动更新:DataObject::GetBoundingBox() 内部会调用
 *     ComputeBoundingBox();
 *   - 法线无需手动重算:面法线由 Face::GetNormal() 按当前点坐标按需计算,
 *     顶点法线在重建可绘制数据时生成,因此形变后调用
 *     ForceReConvertToDrawableData() 即可。
 */
#pragma once
#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class WarpByVectorFilter : public Filter {
public:
    I_OBJECT(WarpByVectorFilter);
    static Pointer New() { return new WarpByVectorFilter; }
    ~WarpByVectorFilter() override = default;

    /**
     * @brief 执行形变。输出为独立对象,失败返回 false。
     */
    bool Execute() override;

    ///@{ 形变向量数组名(默认 "Velocity";置空则自动取第一个向量数组)
    void SetVectorArrayName(const std::string& name) { m_VectorName = name; }
    const std::string& GetVectorArrayName() const { return m_VectorName; }
    ///@}

    ///@{ 缩放系数;开启自动缩放时忽略该值,按模型尺度归一化(参考 Deformation 的 D_model)
    void SetScaleFactor(double scale) { m_Scale = scale; }
    double GetScaleFactor() const { return m_Scale; }
    void SetAutoScale(bool on) { m_AutoScale = on; }
    bool GetAutoScale() const { return m_AutoScale; }
    ///@}

    ///@{ 单点位移上限(相对模型对角线的比例,0 表示不限制)
    void SetMaxDisplacementRatio(double ratio) { m_MaxRatio = ratio; }
    double GetMaxDisplacementRatio() const { return m_MaxRatio; }
    ///@}

    /** 形变后的非结构化网格(失败时为 nullptr) */
    UnstructuredMesh::Pointer GetWarpMesh() { return DynamicCast<UnstructuredMesh>(GetOutput()); }

    /** 最近一次执行实际使用的缩放系数(自动缩放时由内部算出) */
    double GetAppliedScale() const { return m_AppliedScale; }

protected:
    WarpByVectorFilter();

private:
    bool Run();

    /** 按名字(优先点关联)查找形变向量数组;找不到返回 nullptr 并回填 attachment */
    ArrayObject::Pointer FindVectorArray(DataObject::Pointer input, IGenum& attachment) const;

    std::string m_VectorName{"Velocity"};

    double m_Scale{1.0};
    bool m_AutoScale{false};
    double m_MaxRatio{0.0};

    double m_AppliedScale{1.0};
};

IGAME_NAMESPACE_END
