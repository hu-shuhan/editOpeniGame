/**
 * @class    Pen
 * @brief    Pen类是一个画刷类，其保存画刷的参数，用于填充图形对象。
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "iGameColorUtils.h"
#include "iGameObject.h"
#include "iGamePoints.h"

IGAME_NAMESPACE_BEGIN

class Brush : public Object {
public:
    I_OBJECT(Brush);
    static Pointer New() { return new Brush; }

    /**
     * @enum Style
     * @brief 定义画刷的样式类型。
     *
     * 目前仅实现了 `SolidPattern`。
     */
    enum class Style {
        NoBrush = 0,  ///< 无画刷（不填充）。
        SolidPattern, ///< 实色填充样式。
        //Dense1Pattern,    ///< 密集填充样式 1（未实现）。
        //Dense2Pattern,    ///< 密集填充样式 2（未实现）。
        //Dense3Pattern,    ///< 密集填充样式 3（未实现）。
        //Dense4Pattern,    ///< 密集填充样式 4（未实现）。
        //Dense5Pattern,    ///< 密集填充样式 5（未实现）。
        //Dense6Pattern,    ///< 密集填充样式 6（未实现）。
        //Dense7Pattern,    ///< 密集填充样式 7（未实现）。
        //HorPattern,       ///< 水平线填充样式（未实现）。
        //VerPattern,       ///< 垂直线填充样式（未实现）。
        //CrossPattern,     ///< 十字线填充样式（未实现）。
        //BDiagPattern,     ///< 反斜线填充样式（未实现）。
        //FDiagPattern,     ///< 正斜线填充样式（未实现）。
        //DiagCrossPattern, ///< 斜十字线填充样式（未实现）。
        //LinearGradientPattern, ///< 线性渐变填充样式（未实现）。
        //RadialGradientPattern, ///< 辐射渐变填充样式（未实现）。
        //ConicalGradientPattern,///< 锥形渐变填充样式（未实现）。
        //TexturePattern = 24  ///< 纹理填充样式（未实现）。
    };

    /**
     * @brief 设置画刷的颜色。
     * @param color 使用 `Color` 对象设置颜色。
     */
    void SetColor(const Color& color);

    /**
     * @brief 设置画刷的颜色。
     * @param red 红色分量，取值范围为 0-255。
     * @param green 绿色分量，取值范围为 0-255。
     * @param blue 蓝色分量，取值范围为 0-255。
     */
    void SetColor(int red, int green, int blue);

    /**
     * @brief 设置画刷的颜色。
     * @param red 红色分量，取值范围为 0.0-1.0。
     * @param green 绿色分量，取值范围为 0.0-1.0。
     * @param blue 蓝色分量，取值范围为 0.0-1.0。
     */
    void SetColor(float red, float green, float blue);

    /**
     * @brief 获取画刷的颜色。
     * @return 返回颜色值（Vector3f 类型），分别表示 RGB 分量。
     */
    Vector3f GetColor() const;

    /**
     * @brief 设置画刷的样式。
     * @param style 使用 `Brush::Style` 枚举类型设置样式。
     */
    void SetStyle(Brush::Style style);

    /**
     * @brief 获取画刷的样式。
     * @return 返回当前画刷样式。
     */
    Brush::Style GetStyle() const;

    // 以下功能未实现：
    //void SetOpacity(float opacity);
    //float GetOpacity() const;

protected:
    Brush();
    ~Brush() override;

    Vector3f m_BrushColor;
    Brush::Style m_BrushStyle;
    float m_BrushOpacity;
};

IGAME_NAMESPACE_END