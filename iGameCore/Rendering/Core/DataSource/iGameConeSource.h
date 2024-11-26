//
// Created by Sumzeek on 11/20/2024.
//

#pragma once

#include "iGameDataSource.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

/**
 * @class ConeSource
 * @brief A data source class for generating geometric shapes such as cones, pyramids, and frustums.
 *
 * This class provides static methods to generate the vertex data and indices for 3D shapes,
 * including cones, pyramids, and frustums. The generated data can be used in rendering pipelines
 * or for collision and physics simulations.
 */
class ConeSource : public DataSource {
public:
    /**
     * @brief Macro to declare the class as a game object type.
     */
    I_OBJECT(ConeSource);

    /**
     * @brief Generates a cone mesh.
     * @param center The center point of the cone's base.
     * @param normal The normal vector defining the cone's direction.
     * @param height The height of the cone from base to tip.
     * @param radius The radius of the cone's base.
     * @param resolution The number of subdivisions around the cone's circumference.
     * @param offset The offset for vertex indices in the final mesh.
     * @return A structure containing the cone's vertex and index data.
     */
    static DataSourceOutputInfo
    RequestCone(const Point& center, const Vector3f& normal, float height,
                float radius, unsigned int resolution, size_t offset = 0);

    /**
     * @brief Generates a pyramid mesh.
     * @param center The center point of the pyramid's base.
     * @param normal The normal vector defining the pyramid's direction.
     * @param height The height of the pyramid from base to apex.
     * @param radius The radius of the base (distance from center to a corner).
     * @param stackCount The number of vertical subdivisions along the height.
     * @param sectorCount The number of sides (or sectors) of the pyramid base.
     * @param offset The offset for vertex indices in the final mesh.
     * @return A structure containing the pyramid's vertex and index data.
     */
    static DataSourceOutputInfo
    RequestPyramid(const Point& center, const Vector3f& normal, float height,
                   float radius, unsigned int stackCount,
                   unsigned int sectorCount, size_t offset = 0);

    /**
     * @brief Generates a frustum mesh.
     * @param center The center point of the frustum's base.
     * @param normal The normal vector defining the frustum's direction.
     * @param height The height of the frustum.
     * @param baseRadius The radius of the base of the frustum.
     * @param topRadius The radius of the top of the frustum.
     * @param resolution The number of subdivisions around the circumference.
     * @param offset The offset for vertex indices in the final mesh.
     * @return A structure containing the frustum's vertex and index data.
     */
    static DataSourceOutputInfo
    RequestFrustum(const Point& center, const Vector3f& normal, float height,
                   float baseRadius, float topRadius, unsigned int resolution,
                   size_t offset = 0 /*, bool capped = true*/);

protected:
    /**
     * @brief Constructor for ConeSource.
     *
     * Protected to ensure that instances are created only through static methods.
     */
    ConeSource();

    /**
     * @brief Destructor for ConeSource.
     */
    ~ConeSource() override;
};

IGAME_NAMESPACE_END
