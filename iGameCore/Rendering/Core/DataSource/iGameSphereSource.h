/**
 * @class    SphereSource
 * @brief    SphereSource类提供三维渲染数据生成。
 *
 * SphereSource 提供了以下类型球体的生成方法：
 * - 标准球（纬度/经度分割）。
 * - 十二面体球（递序分裂十二面体）。
 * - 格子映射球（由格子面映射实现）。
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "iGameDataSource.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

class SphereSource : public DataSource {
public:
    I_OBJECT(SphereSource);

    /**
     * @brief 通过分区线约生成标准球。
     *
     * @param center 球体在三维空间中的中心。
     * @param radius 球体的半径。
     * @param sectorCount 分区格子数（经度分割）。
     * @param stackCount 分区格子数（纬度分割）。
     * @param offset 顶点的索引偏移量。
     * @return DataSourceOutputInfo 包含顶点和索引的结构体。
     */
    static DataSourceOutputInfo RequestSphere(const Point& center, float radius,
                                              unsigned int stackCount,
                                              unsigned int sectorCount,
                                              size_t offset = 0);

    /**
     * @brief 通过递序分裂十二面体生成十二面体球。
     *
     * @param center 球体在三维空间中的中心。
     * @param radius 球体的半径。
     * @param subdivision 递序分裂次数。
     * @param offset 顶点的索引偏移量。
     * @return DataSourceOutputInfo 包含顶点和索引的结构体。
     */
    static DataSourceOutputInfo RequestIcoSphere(const Point& center,
                                                 float radius,
                                                 unsigned int subdivision,
                                                 size_t offset = 0);

    /**
     * @brief 通过格子面映射到球体生成格子球。
     *
     * @param center 球体在三维空间中的中心。
     * @param radius 球体的半径。
     * @param vertexCountPerRow 每个格子面上的顶点数。
     * @param offset 顶点的索引偏移量。
     * @return DataSourceOutputInfo 包含顶点和索引的结构体。
     */
    static DataSourceOutputInfo
    RequestCubeSphere(const Point& center, float radius,
                      unsigned int vertexCountPerRow, size_t offset = 0);

protected:
    SphereSource();
    ~SphereSource() override;
    
    /**
     * @brief 将向量调整到指定长度所需的放缩尺度。
     *
     * @param v 将被调整的向量。
     * @param length 应该调整到的向量长度。
     * @return 向量所需的放缩尺度。
     */
    static float computeScaleForLength(const Point v, float length);

    /**
     * @brief 在两个顶点之间进行插值计算，并将结果放缩到指定长度。
     *
     * @param v1 第一个顶点。
     * @param v2 第二个顶点。
     * @param t 接近因子（0.0 到 1.0）。
     * @param length 插值结果需要放缩到的长度。
     * @param newV 接近计算的结果（输出参数）。
     */
    static void interpolateVertex(const Point v1, const Point v2, float t,
                                  float length, Point& newV);

    /**
     * @brief 在两个标量值之间进行线性接近计算。
     *
     * @param from 起始值。
     * @param to 结束值。
     * @param alpha 接近因子（0.0 到 1.0）。
     * @return 接近计算结果。
     */
    static float lerp(float from, float to, float alpha);

    /**
     * @brief 计算十二面体的 12 个顶点。
     *
     * 十二面体通过以下方法构造：
     * - 北极位于 (0, 0, r)，南极位于 (0, 0, -r)。
     * - 五个顶点通过回转 72° 位于十二面体层高级点于 26.57° (余差现象 atan(1/2))。
     * - 另五个顶点位于低级点，半度为 -26.57°。
     *
     * @param radius 十二面体的半径。
     * @return 表示十二面体顶点的点集。
     */
    static DataSource::Points computeIcosahedronVertices(float radius);

    /**
     * @brief 生成格子球在 +X 面上的单个面的单位长度顶点。
     *
     * 这些点在角度获得的格子网格中分布，并求值为球面。
     *
     * @param pointsPerRow 格子面上每一行的顶点数。
     * @return 表示 +X 面顶点的点集合。
     */
    static DataSource::Points getUnitPositiveX(unsigned int pointsPerRow);
};

IGAME_NAMESPACE_END
