/**
 * @class    Pen
 * @brief    Pen类是一个画笔类，其保存画笔的参数，用于绘制图形边框。
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "iGameColorUtils.h"
#include "iGameObject.h"
#include "iGamePoints.h"

IGAME_NAMESPACE_BEGIN

class Pen : public Object {
public:
    I_OBJECT(Pen);
    static Pointer New() { return new Pen; }

    /**
     * @enum Style
     * @brief 定义画笔的样式类型。
     *
     * 目前仅实现了 `SolidLine`。
     */
    enum class Style {
        NoPen = 0, ///< 无画笔（不绘制）。
        SolidLine, ///< 实线样式。
        //DashLine, ///< 虚线样式。
        //DotLine, ///< 点线样式。
        //DashDotLine, ///< 点划线样式。
        //DashDotDotLine, ///< 双点划线样式。
        //CustomDashLine ///< 自定义虚线样式。
    };

    /**
     * @brief 设置画笔的颜色。
     * @param color 使用 `Color` 对象设置颜色。
     */
    void SetColor(const Color& color);

    /**
     * @brief 设置画笔的颜色。
     * @param red 红色分量，取值范围为 0-255。
     * @param green 绿色分量，取值范围为 0-255。
     * @param blue 蓝色分量，取值范围为 0-255。
     */
    void SetColor(int red, int green, int blue);

    /**
     * @brief 设置画笔的颜色。
     * @param red 红色分量，取值范围为 0.0-1.0。
     * @param green 绿色分量，取值范围为 0.0-1.0。
     * @param blue 蓝色分量，取值范围为 0.0-1.0。
     */
    void SetColor(float red, float green, float blue);

    /**
     * @brief 获取画笔的颜色。
     * @return 返回颜色值（Vector3f 类型），分别表示 RGB 分量。
     */
    Vector3f GetColor() const;

    /**
     * @brief 设置画笔的宽度。
     * @param width 画笔的宽度值。
     */
    void SetWidth(float width);

    /**
     * @brief 获取画笔的宽度。
     * @return 返回画笔的宽度值。
     */
    float GetWidth() const;

    /**
     * @brief 设置画笔的样式。
     * @param style 使用 `Style` 枚举类型设置样式。
     */
    void SetStyle(Style style);

    /**
     * @brief 获取画笔的样式。
     * @return 返回当前画笔样式。
     */
    Style GetStyle() const;

    // 以下功能未实现：
    //void SetOpacity(float opacity);
    //float GetOpacity() const;

protected:
    Pen();
    ~Pen() override;

    Vector3f m_PenColor;
    float m_PenWidth;
    Pen::Style m_PenStyle;
    float m_PenOpacity;
};

IGAME_NAMESPACE_END