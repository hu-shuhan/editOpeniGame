/**
 * @class    PainterBase
 * @brief    PainterBase类是基础绘制器类，用于管理绘制对象、笔和画刷等相关功能。
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include <unordered_map>

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLVertexArray.h"
#include "iGameBoundingBox.h"
#include "iGameBrush.h"
#include "iGameColorUtils.h"
#include "iGameHandlePool.h"
#include "iGameObject.h"
#include "iGamePen.h"
#include "iGamePoints.h"

IGAME_NAMESPACE_BEGIN
class Scene;

class PainterBase : public Object {
public:
    I_OBJECT(PainterBase);

    /**
     * @struct Primitive
     * @brief 表示一个基本绘制元素。
     */
    struct Primitive {
        float penWidth;
        std::vector<Vector3f> points;
        std::vector<Vector3f> colors;
        //std::vector<Vector3f> normals;
        std::array<std::vector<iguIndex>, 3> indices;
        bool visible = true;
        BoundingBox bounding;
    };

    /**
     * @brief 显示所有绘制元素。
     */
    void ShowAll();

    /**
     * @brief 隐藏所有绘制元素。
     */
    void HideAll();

    /**
     * @brief 显示指定的绘制元素。
     * @param handle 指定绘制元素的句柄。
     */
    void Show(IGuint handle);

    /**
     * @brief 隐藏指定的绘制元素。
     * @param handle 指定绘制元素的句柄。
     */
    void Hide(IGuint handle);

    /**
     * @brief 删除指定的绘制元素。
     * @param handle 指定绘制元素的句柄。
     */
    void Delete(IGuint handle);

    /**
     * @brief 设置画笔对象。
     * @param pen 指向 `Pen` 对象的智能指针。
     */
    void SetPen(const SmartPointer<Pen>& pen);

    /**
     * @brief 设置画笔的颜色。
     * @param color 使用 `Color` 对象设置颜色。
     */
    void SetPen(const Color& color);

    /**
     * @brief 设置画笔的颜色。
     * @param red 红色分量，取值范围为 0-255。
     * @param green 绿色分量，取值范围为 0-255。
     * @param blue 蓝色分量，取值范围为 0-255。
     */
    void SetPen(int red, int green, int blue);

    /**
     * @brief 设置画笔的颜色。
     * @param red 红色分量，取值范围为 0.0-1.0。
     * @param green 绿色分量，取值范围为 0.0-1.0。
     * @param blue 蓝色分量，取值范围为 0.0-1.0。
     */
    void SetPen(float red, float green, float blue);

    /**
     * @brief 设置画笔的样式。
     * @param style 画笔的样式，使用 `Pen::Style` 枚举类型。
     */
    void SetPen(const Pen::Style& style);

    /**
     * @brief 设置画笔的宽度。
     * @param width 画笔的宽度。
     */
    void SetPen(float width);

    /**
     * @brief 设置画刷对象。
     * @param brush 指向 `Brush` 对象的智能指针。
     */
    void SetBrush(const SmartPointer<Brush>& brush);

    /**
     * @brief 设置画刷的颜色。
     * @param color 使用 `Color` 对象设置颜色。
     */
    void SetBrush(const Color& color);

    /**
     * @brief 设置画刷的颜色。
     * @param red 红色分量，取值范围为 0-255。
     * @param green 绿色分量，取值范围为 0-255。
     * @param blue 蓝色分量，取值范围为 0-255。
     */
    void SetBrush(int red, int green, int blue);

    /**
     * @brief 设置画刷的颜色。
     * @param red 红色分量，取值范围为 0.0-1.0。
     * @param green 绿色分量，取值范围为 0.0-1.0。
     * @param blue 蓝色分量，取值范围为 0.0-1.0。
     */
    void SetBrush(float red, float green, float blue);

    /**
     * @brief 设置画刷的样式。
     * @param style 画刷的样式，使用 `Brush::Style` 枚举类型。
     */
    void SetBrush(const Brush::Style& style);

    /**
     * @brief 获取绘制对象的包围盒。
     * @return 返回当前的包围盒对象。
     */
    const BoundingBox& GetBoundingBox();

    /**
     * @brief 绘制场景中的元素。
     * @param scene 指向需要绘制的场景对象。
     */
    virtual void Draw(Scene* scene);

    /**
     * @brief 清除所有绘制元素。
     */
    void Clear();

protected:
    PainterBase();
    ~PainterBase() override;

    void ComputeBoundingBox();
    void CreateDrawBuffer(float penWidth);
    void PackDrawableData();

    SmartPointer<Pen> m_Pen;
    SmartPointer<Brush> m_Brush;

    BoundingBox m_Bounding;
    SmartPointer<Object> m_BoundingHelper;

    SmartPointer<Object> m_PrimitivesUpdateHelper;
    SmartPointer<HandlePool<Primitive>> m_PrimitivesPool;

    std::unordered_map<float, SmartPointer<GLVertexArray>> m_VAOs;
    std::unordered_map<float, SmartPointer<GLBuffer>> m_PositionVBOs;
    std::unordered_map<float, SmartPointer<GLBuffer>> m_ColorVBOs;
    //std::unordered_map<float, SmartPointer<GLBuffer>> m_NormalVBOs;
    std::unordered_map<float, SmartPointer<GLBuffer>> m_PointEBOs;
    std::unordered_map<float, SmartPointer<GLBuffer>> m_LineEBOs;
    std::unordered_map<float, SmartPointer<GLBuffer>> m_TriangleEBOs;

    std::unordered_map<float, IGsize> m_PointEBOSizes;
    std::unordered_map<float, IGsize> m_LineEBOSizes;
    std::unordered_map<float, IGsize> m_TriangleEBOSizes;
};

IGAME_NAMESPACE_END