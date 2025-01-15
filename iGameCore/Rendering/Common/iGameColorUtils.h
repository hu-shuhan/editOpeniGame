/**
 * @class    ColorUtils
 * @brief    ColorUtils类是一个用于颜色相关操作的工具类。
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "iGameObject.h"
#include "iGameVector.h"
#include "igm/igm.h"

IGAME_NAMESPACE_BEGIN

class ColorUtils : public Object {
public:
    /**
     * @brief 检查给定的颜色向量是否有效。
     * @param color 三维向量，表示颜色值。
     * @return 如果颜色有效，返回 true；否则返回 false。
     */
    static bool IsValid(const igm::vec3& color);

    /**
     * @brief 检查给定的颜色向量是否有效。
     * @param color 三维浮点向量，表示颜色值。
     * @return 如果颜色有效，返回 true；否则返回 false。
     */
    static bool IsValid(Vector3f color);

    /**
     * @brief 检查给定的 RGB 浮点值是否有效。
     * @param red 红色分量。
     * @param green 绿色分量。
     * @param blue 蓝色分量。
     * @return 如果颜色有效，返回 true；否则返回 false。
     */
    static bool IsValid(float red, float green, float blue);

    /**
     * @brief 检查给定的 RGB 整数值是否有效。
     * @param red 红色分量。
     * @param green 绿色分量。
     * @param blue 蓝色分量。
     * @return 如果颜色有效，返回 true；否则返回 false。
     */
    static bool IsValid(int red, int green, int blue);

    /**
     * @brief 将颜色对象映射到一个三维向量。
     * @param color 颜色对象。
     * @return 对应的三维向量。
     */
    static igm::vec3 Map(Color color);

protected:
    /**
     * @brief 构造一个 ColorUtils 对象。
     */
    ColorUtils();

    /**
     * @brief 销毁 ColorUtils 对象。
     */
    ~ColorUtils() override;
};

IGAME_NAMESPACE_END
