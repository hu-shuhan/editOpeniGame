//
// Created by Sumzeek on 11/19/2024.
//

#pragma once

#include "iGameDataSource.h"
#include "iGameDataSource3D.h"

IGAME_NAMESPACE_BEGIN

/**
 * @class DataSource2D
 * @brief A class for generating 2D geometry primitives as data sources.
 *
 * **Output Data**:
 * - The generated `DataSourceOutputInfo` represents the geometry in 3D space.
 * - Since this is a 2D system, the z-coordinate of all points is set to `0`.
 */
class DataSource2D : public DataSource {
public:
    I_OBJECT(DataSource2D);

    using Point2D = Vector2f; ///< Alias for a 2D point using a 2D vector type.
    using Point3D = Vector3f; ///< Alias for a 3D point using a 3D vector type.

    /**
     * @brief Generates data for a point in 2D space.
     *
     * Visualization:
     * ```
     * p1 +
     * ```
     *
     * @param point The 2D coordinates of the point to be rendered.
     * @param offset Offset in the data buffer for additional customization (default is 0).
     * @return Information about the generated point data.
     */
    static DataSourceOutputInfo RequestPoint(const Point2D& point,
                                             size_t offset = 0);

    /**
     * @brief Generates data for a line segment in 2D space defined by two points.
     *
     * Visualization:
     * ```
     * p1 +------------+ p2
     * ```
     *
     * @param p1 The starting point of the line segment.
     * @param p2 The ending point of the line segment.
     * @param offset Offset in the data buffer for additional customization (default is 0).
     * @return Information about the generated line segment data.
     */
    static DataSourceOutputInfo
    RequestLine(const Point2D& p1, const Point2D& p2, size_t offset = 0);

    /**
     * @brief Generates the data for a triangle in 2D space defined by three vertices.
     *
     * Visualization:
     * ```
     *       p2
     *       /\
     *      /  \
     *     /    \
     *    /______\
     *   p1       p3
     * ```
     *
     * @param p1 The first vertex of the triangle.
     * @param p2 The second vertex of the triangle.
     * @param p3 The third vertex of the triangle.
     * @param offset Offset in the data buffer for additional customization (default is 0).
     * @return Information about the generated triangle data.
     */
    static DataSourceOutputInfo RequestTriangle(const Point2D& p1,
                                                const Point2D& p2,
                                                const Point2D& p3,
                                                size_t offset = 0);

    /**
     * @brief Generates data for a rectangle in 2D space defined by two diagonal corners.
     *
     * Visualization:
     * ```
     * p2 +------------+ p3
     *    |            |
     *    |            |
     *    |            |
     * p1 +------------+ p4
     * ```
     * The rectangle is defined by points p1 (bottom-left) and p3 (top-right).
     * Points p2 and p4 are computed internally.
     *
     * @param p1 The bottom-left corner of the rectangle.
     * @param p3 The top-right corner of the rectangle.
     * @param offset Offset in the data buffer for additional customization (default is 0).
     * @return Information about the generated rectangle data.
     */
    static DataSourceOutputInfo
    RequestRect(const Point2D& p1, const Point2D& p3, size_t offset = 0);

    /**
     * @brief Generates data for a circle in 2D space.
     *
     * Visualization:
     * ```
     *            ******
     *         **       **
     *       **           **
     *      *     center    *
     *       **           **
     *         **       **
     *            ******
     * ```
     * The circle is defined by a center point and a radius.
     *
     * @param center The center of the circle.
     * @param radius The radius of the circle.
     * @param resolution The number of discrete points used to approximate the circle's edge.
     * @param offset Offset in the data buffer for additional customization (default is 0).
     * @return Information about the generated circle data.
     */
    static DataSourceOutputInfo RequestCircle(const Point2D& center,
                                              float radius, int resolution,
                                              size_t offset = 0);

protected:
    /**
     * @brief Default constructor for DataSource2D.
     */
    DataSource2D();

    /**
     * @brief Destructor for DataSource2D.
     */
    ~DataSource2D() override;
};

IGAME_NAMESPACE_END
