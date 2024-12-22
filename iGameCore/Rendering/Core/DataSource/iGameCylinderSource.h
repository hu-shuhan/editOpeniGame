/**
 * @class    CylinderSource
 * @brief    CylinderSource类提供三维渲染数据生成。
 *
 * CylinderSource类提供静态方法来生成类圆柱体的顶点数据和索引，
 * 圆柱体可以由其底部中心、高度、半径和分辨率等定义。
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "iGameDataSource.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

class CylinderSource : public DataSource {
public:
    I_OBJECT(CylinderSource);

    /**
     * @brief 生成圆柱网格数据。
     * @param center 圆柱底部的中心点。
     * @param normal 定义圆柱轴向方向的法线向量。
     * @param height 圆柱的高度。
     * @param radius 圆柱底部的半径。
     * @param resolution 圆柱周围的分段数量，值越高圆柱越平滑。
     * @param offset 顶点索引的偏移量，用于合并多个网格数据时避免索引冲突。
     * @return 包含圆柱顶点和索引数据的结构体。
     */
    static DataSourceOutputInfo
    RequestCylinder(const Point& center, const Vector3f& normal, float height,
                    float radius, unsigned int resolution, size_t offset = 0);

protected:
    CylinderSource();
    ~CylinderSource() override;
};

IGAME_NAMESPACE_END
