/**
 * @class    Painter2D
 * @brief    Painter2D类用于在场景中渲染图形元素的2D绘制工具类。
 *
 * 坐标系统：
 * - 原点(0, 0)位于屏幕的左下角。
 * - 所有坐标值使用像素来表示，代表屏幕上的绝对位置。
 * - x 轴正方向指向右。
 * - y 轴正方向指向上。
 * - 坐标的有效范围取决于屏幕的分辨率。
 *
 * 坐标轴示意图：
 *
 *    +Y (0, screenHeight)
 *    ^
 *    |
 *    |
 *    |
 *    +----------> +X (screenWidth, 0)
 *  (0, 0)
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "DataSource/iGameDataSource2D.h"
#include "iGamePainterBase.h"

IGAME_NAMESPACE_BEGIN
class Scene;

class Painter2D : public PainterBase {
public:
    I_OBJECT(Painter2D);
    static Pointer New() { return new Painter2D; }

    /**
     * @brief 绘制场景中的图形元素。
     * @param scene 指向需要绘制的场景对象。
     */
    void Draw(Scene*) override;

    // 绘制点的方法，返回一个无符号整型数值
    IGuint DrawPoint(const Vector2ui& point);

    // 绘制直线的方法，返回一个无符号整型数值
    IGuint DrawLine(const Vector2ui& p1, const Vector2ui& p2);

    // 绘制三角形的方法，返回一个无符号整型数值
    IGuint DrawTriangle(const Vector2ui& p1, const Vector2ui& p2,
                        const Vector2ui& p3);

    // 绘制矩形的方法，返回一个无符号整型数值
    IGuint DrawRect(const Vector2ui& p1, const Vector2ui& p3);

    // 绘制圆形的方法，返回一个无符号整型数值
    IGuint DrawCircle(const Vector2ui& center, double radius, int resolution);

protected:
    Painter2D();
    ~Painter2D() override;
};

IGAME_NAMESPACE_END