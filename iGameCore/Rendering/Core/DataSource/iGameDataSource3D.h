/**
 * @class    DataSource3D
 * @brief    DataSource3D类提供三维渲染数据生成。
 *
 * DataSource3D类提供静态方法来生成常见三维数据的顶点数据和索引。
 * 包括点、线段、三角形、矩形、立方体和圆形。
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */
#pragma once

#include "iGameDataSource.h"

IGAME_NAMESPACE_BEGIN

class DataSource3D : public DataSource {
public:
    I_OBJECT(DataSource3D);

    /**
     * @brief 生成一个三维空间中的点数据。
     *
     * 可视化：
     * ```
     * p1 +
     * ```
     *
     * @param point 要渲染的点的坐标。
     * @param offset 顶点的索引偏移量。
     * @return 包含生成点数据的信息。
     */
    static DataSourceOutputInfo RequestPoint(const Point& point,
                                             size_t offset = 0);

    /**
     * @brief 生成由两个点定义的三维空间线段的数据。
     *
     * 可视化：
     * ```
     * p1 +------------+ p2
     * ```
     *
     * @param p1 线段的起点。
     * @param p2 线段的终点。
     * @param offset 顶点的索引偏移量。
     * @return 包含生成线段数据的信息。
     */
    static DataSourceOutputInfo RequestLine(const Point& p1, const Point& p2,
                                            size_t offset = 0);

    /**
     * @brief 生成由三个顶点定义的三维空间三角形的数据。
     *
     * 可视化：
     * ```
     *       p2
     *       /\
     *      /  \
     *     /    \
     *    /______\
     *   p1       p3
     * ```
     *
     * @param p1 三角形的第一个顶点。
     * @param p2 三角形的第二个顶点。
     * @param p3 三角形的第三个顶点。
     * @param offset 顶点的索引偏移量。
     * @return 包含生成三角形数据的信息。
     */
    static DataSourceOutputInfo RequestTriangle(const Point& p1,
                                                const Point& p2,
                                                const Point& p3,
                                                size_t offset = 0);

    /**
     * @brief 生成由两个对角点定义的三维空间矩形的数据。
     *
     * 可视化：
     * ```
     * p2 +------------+ p3
     *    |            |
     *    |            |
     *    |            |
     * p1 +------------+ p4
     * ```
     * 矩形由 p1（左下角）和 p3（右上角）定义，
     * p2 和 p4 的坐标由内部计算。
     *
     * @param p1 矩形的左下角点。
     * @param p3 矩形的右上角点。
     * @param offset 顶点的索引偏移量。
     * @return 包含生成矩形数据的信息。
     */
    static DataSourceOutputInfo RequestRect(const Point& p1, const Point& p3,
                                            size_t offset = 0);

    /**
     * @brief 生成由两个对角点定义的三维空间立方体的数据。
     *
     * 可视化：
     * ```
     *     p6+-----------+ p7
     *      /|          /|
     *     / |         / |
     *    /  |        /  |
     * p5+-----------+p8 |
     *   |   |       |   |
     *   | p2+-------|---+ p3
     *   |  /        |  /
     *   | /         | /
     *   |/          |/
     * p1+-----------+ p4
     * ```
     * 立方体由 p1（左下前角）和 p7（右上后角）定义。
     *
     * @param p1 立方体的左下前角点。
     * @param p7 立方体的右上后角点。
     * @param offset 顶点的索引偏移量。
     * @return 包含生成立方体数据的信息。
     */
    static DataSourceOutputInfo RequestCube(const Point& p1, const Point& p7,
                                            size_t offset = 0);

    /**
     * @brief 生成位于指定平面上的三维空间圆形的数据。
     *
     * 可视化（俯视图）：
     * ```
     *            *******
     *         **         **
     *       **             **
     *      *                 *
     *     *        * center   *
     *      *                 *
     *       **             **
     *         **         **
     *            *******
     * ```
     * 圆由中心点、法线向量（指定圆所在的平面）和半径定义。
     *
     * @param center 圆的中心点。
     * @param normal 圆所在平面的法线向量。
     * @param radius 圆的半径。
     * @param resolution 用于近似圆边界的离散点数。
     * @param offset 顶点的索引偏移量。
     * @return 包含生成圆形数据的信息。
     */
    static DataSourceOutputInfo RequestCircle(const Point& center,
                                              const Vector3f& normal,
                                              float radius, int resolution,
                                              size_t offset = 0);

protected:
    DataSource3D();
    ~DataSource3D() override;
};

IGAME_NAMESPACE_END
