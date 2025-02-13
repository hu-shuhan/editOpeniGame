/**
 * @class    ConeSource
 * @brief    ConeSource类提供三维渲染数据生成。
 *
 * ConeSource类提供静态方法来生成类圆锥体的顶点数据和索引，
 * 圆锥体可以由其底部中心、高度、半径和分辨率等定义。
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "iGameDataSource.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

class ConeSource : public DataSource {
public:
    I_OBJECT(ConeSource);

    /**
     * @brief 生成一个圆锥网格。
     * @param center 圆锥底面的中心点。
     * @param normal 圆锥底面的法向量。
     * @param height 圆锥从底面到尖端的高度。
     * @param radius 圆锥底面的半径。
     * @param resolution 圆锥周长的分割数。
     * @param offset 顶点的索引偏移量。
     * @return 包含圆锥顶点和索引数据的结构体。
     */
    static DataSourceOutputInfo
    RequestCone(const Point& center, const Vector3f& normal, float height,
                float radius, unsigned int resolution, size_t offset = 0);

    /**
     * @brief 生成一个金字塔网格。
     * @param center 金字塔底面的中心点。
     * @param normal 金字塔底面的法向量。
     * @param height 金字塔从底面到高点的高度。
     * @param radius 金字塔底面的半径（从中心到角的距离）。
     * @param stackCount 金字塔高度方向的分层数。
     * @param sectorCount 金字塔底面的边数（或分部数）。
     * @param offset 顶点的索引偏移量。
     * @return 包含金字塔顶点和索引数据的结构体。
     */
    static DataSourceOutputInfo
    RequestPyramid(const Point& center, const Vector3f& normal, float height,
                   float radius, unsigned int stackCount,
                   unsigned int sectorCount, size_t offset = 0);

    /**
     * @brief 生成一个视锥体网格。
     * @param center 视锥体底面的中心点。
     * @param normal 视锥体底面的法向量。
     * @param height 视锥体的高度。
     * @param baseRadius 视锥体的半径。
     * @param topRadius 视锥体的半径。
     * @param resolution 视锥体周长的分割数。
     * @param offset 顶点的索引偏移量。
     * @return 包含视锥体顶点和索引数据的结构体。
     */
    static DataSourceOutputInfo
    RequestFrustum(const Point& center, const Vector3f& normal, float height,
                   float baseRadius, float topRadius, unsigned int resolution,
                   size_t offset = 0 /*, bool capped = true*/);

protected:
    ConeSource();

    ~ConeSource() override;
};

IGAME_NAMESPACE_END
