//
// Created by Sumzeek on 11/20/2024.
//

#pragma once

#include "iGameDataSource.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

/**
 * @class CylinderSource
 * @brief A data source class for generating cylinder geometry.
 *
 * This class provides static methods to generate the vertex data and indices for
 * a 3D cylinder. The cylinder can be defined by its base center, height, radius,
 * and resolution. The generated data can be used for rendering, physics simulations,
 * or other geometric operations.
 */
class CylinderSource : public DataSource {
public:
    /**
     * @brief Macro to declare the class as a game object type.
     */
    I_OBJECT(CylinderSource);

    /**
     * @brief Generates a cylinder mesh.
     * @param center The center point of the cylinder's base.
     * @param normal The normal vector defining the cylinder's axis direction.
     * @param height The height of the cylinder.
     * @param radius The radius of the cylinder's base.
     * @param resolution The number of subdivisions around the cylinder's circumference.
     *                   Higher values increase the smoothness of the cylinder.
     * @param offset The offset for vertex indices in the final mesh. This is useful
     *               when combining multiple meshes into a single geometry.
     * @return A structure containing the cylinder's vertex and index data.
     *
     * The method computes:
     * - The circular base and top of the cylinder.
     * - The side faces formed by connecting the base and top vertices.
     * - Optional indices for wireframe or solid rendering modes.
     */
    static DataSourceOutputInfo
    RequestCylinder(const Point& center, const Vector3f& normal, float height,
                    float radius, unsigned int resolution, size_t offset = 0);

protected:
    /**
     * @brief Constructor for CylinderSource.
     *
     * Protected to ensure that instances are created only through static methods.
     */
    CylinderSource();

    /**
     * @brief Destructor for CylinderSource.
     */
    ~CylinderSource() override;
};

IGAME_NAMESPACE_END
