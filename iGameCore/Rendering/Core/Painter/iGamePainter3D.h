/**
 * @class    Painter3D
 * @brief    Painter3D类用于在场景中渲染图形元素的3D绘制工具类。
 *
 * 提供了绘制各种3D图元（如点、线、三角形、矩形、立方体和圆形）的方法。
 * 它与场景（Scene）对象协同工作。
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "DataSource/iGameConeSource.h"
#include "DataSource/iGameCylinderSource.h"
#include "DataSource/iGameDataSource3D.h"
#include "DataSource/iGameSphereSource.h"
#include "iGamePainterBase.h"

IGAME_NAMESPACE_BEGIN
class Scene;

class Painter3D : public PainterBase {
public:
    I_OBJECT(Painter3D);
    static Pointer New() { return new Painter3D; }

    /**
     * @brief Painter3D的枚举类型。用于在Model中的区分
     */
    enum class Usage : unsigned int {
        Default,
        BoundingBox,
        SelectedPoint,
        SelectedCell,
        SelectionBox
    };

    /**
     * @brief 绘制场景中的图形元素。
     * @warning 调用该函数前需要确保与其绑定的场景（Scene）OpenGL上下文处于活跃状态。
     */
    void Draw() override;

    /**
     * @brief 绘制一个点。
     * @param point 点的坐标。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawPoint(const Point& point);

    /**
     * @brief 绘制一条线段。
     * @param p1 线段起点。
     * @param p2 线段终点。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawLine(const Point& p1, const Point& p2);

    /**
     * @brief 绘制一个三角形。
     * @param p1 三角形的第一个顶点。
     * @param p2 三角形的第二个顶点。
     * @param p3 三角形的第三个顶点。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawTriangle(const Point& p1, const Point& p2, const Point& p3);

    /**
     * @brief 绘制一个矩形。
     * @param p1 矩形的一角。
     * @param p3 对角线另一端的顶点。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawRect(const Point& p1, const Point& p3);

    /**
     * @brief 绘制一个立方体。
     * @param p1 立方体的一角。
     * @param p7 对角线另一端的顶点。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawCube(const Point& p1, const Point& p7);

    /**
     * @brief 绘制一个圆。
     * @param center 圆的中心点。
     * @param normal 圆所在平面的法向量。
     * @param radius 圆的半径。
     * @param resolution 圆的分辨率（多边形的边数）。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawCircle(const Point& center, const Vector3f& normal, float radius,
                      int resolution);

    /**
     * @brief 绘制一个球体。
     * @param center 球心位置。
     * @param radius 球体半径。
     * @param stackCount 垂直方向的分段数。
     * @param sectorCount 水平方向的分段数。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawSphere(const Point& center, float radius,
                      unsigned int stackCount, unsigned int sectorCount);

    /**
     * @brief 绘制一个正二十面体球（IcoSphere）。
     * @param center 球心位置。
     * @param radius 球体半径。
     * @param subdivision 分割次数，用于增加球体细节。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawIcoSphere(const Point& center, float radius,
                         unsigned int subdivision);

    /**
     * @brief 绘制一个由立方体生成的球体（CubeSphere）。
     * @param center 球心位置。
     * @param radius 球体半径。
     * @param vertexCountPerRow 每行的顶点数量。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawCubeSphere(const Point& center, float radius,
                          unsigned int vertexCountPerRow);

    /**
     * @brief 绘制一个圆柱体。
     * @param center 圆柱体的底面中心点。
     * @param normal 圆柱体的轴向。
     * @param height 圆柱体的高度。
     * @param radius 圆柱体的半径。
     * @param resolution 圆柱体底面的分辨率（多边形的边数）。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawCylinder(const Point& center, const Vector3f& normal,
                        float height, float radius, unsigned int resolution);

    /**
     * @brief 绘制一个圆锥体。
     * @param center 圆锥体的底面中心点。
     * @param normal 圆锥体的轴向。
     * @param height 圆锥体的高度。
     * @param radius 圆锥体的底面半径。
     * @param resolution 圆锥体底面的分辨率（多边形的边数）。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawCone(const Point& center, const Vector3f& normal, float height,
                    float radius, unsigned int resolution);

    /**
     * @brief 绘制一个金字塔。
     * @param center 金字塔底面中心点。
     * @param normal 金字塔的轴向。
     * @param height 金字塔的高度。
     * @param radius 金字塔底面的半径。
     * @param stackCount 底面到顶点的分层数。
     * @param sectorCount 每层的分段数。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawPyramid(const Point& center, const Vector3f& normal,
                       float height, float radius, unsigned int stackCount,
                       unsigned int sectorCount);

    /**
     * @brief 绘制一个截头圆锥体（Frustum）。
     * @param center 截头圆锥体底面中心点。
     * @param normal 截头圆锥体的轴向。
     * @param height 截头圆锥体的高度。
     * @param baseRadius 底面半径。
     * @param topRadius 顶面半径。
     * @param resolution 底面和顶面的分辨率（多边形的边数）。
     * @return 返回绘制对象的句柄。
     */
    IGuint DrawFrustum(const Point& center, const Vector3f& normal,
                       float height, float baseRadius, float topRadius,
                       unsigned int resolution);


protected:
    Painter3D();
    ~Painter3D() override;
};

IGAME_NAMESPACE_END