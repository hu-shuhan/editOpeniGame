/**
 * @class    DataSource2D
 * @brief    DataSource2D类提供二维渲染数据生成。
 *
 * DataSource2D类提供静态方法来生成常见二维数据的顶点数据和索引。
 * 由于这是二维系统，所有点的 z 坐标将被设置为0，并且坐标数据所代表的含义为像素坐标。
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "iGameDataSource.h"
#include "iGameDataSource3D.h"

IGAME_NAMESPACE_BEGIN

class DataSource2D : public DataSource {
public:
    I_OBJECT(DataSource2D);

    using Point2D = Vector2f; ///< 定义 2D 点的别名，使用二维向量类型。
    using Point3D = Vector3f; ///< 定义 3D 点的别名，使用三维向量类型。

    /**
     * @brief 生成二维空间中的点数据。
     *
     * 可视化：
     * ```
     * p1 +
     * ```
     *
     * @param point 要渲染的点的二维坐标。
     * @param offset 顶点的索引偏移量。
     * @return 包含生成点数据的信息。
     */
    static DataSourceOutputInfo RequestPoint(const Point2D& point,
                                             size_t offset = 0);

    /**
     * @brief 生成由两个点定义的二维空间线段的数据。
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
    static DataSourceOutputInfo
    RequestLine(const Point2D& p1, const Point2D& p2, size_t offset = 0);

    /**
     * @brief 生成由三个顶点定义的二维空间三角形的数据。
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
    static DataSourceOutputInfo RequestTriangle(const Point2D& p1,
                                                const Point2D& p2,
                                                const Point2D& p3,
                                                size_t offset = 0);

    /**
     * @brief 生成由两个对角点定义的二维空间矩形的数据。
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
    static DataSourceOutputInfo
    RequestRect(const Point2D& p1, const Point2D& p3, size_t offset = 0);

    /**
     * @brief 生成二维空间中的圆形数据。
     *
     * 可视化：
     * ```
     *            ******
     *         **       **
     *       **           **
     *      *     center    *
     *       **           **
     *         **       **
     *            ******
     * ```
     * 圆由中心点和半径定义。
     *
     * @param center 圆的中心点。
     * @param radius 圆的半径。
     * @param resolution 用于近似圆边界的离散点数。
     * @param offset 顶点的索引偏移量。
     * @return 包含生成圆形数据的信息。
     */
    static DataSourceOutputInfo RequestCircle(const Point2D& center,
                                              float radius, int resolution,
                                              size_t offset = 0);

protected:
    DataSource2D();
    ~DataSource2D() override;
};

IGAME_NAMESPACE_END
