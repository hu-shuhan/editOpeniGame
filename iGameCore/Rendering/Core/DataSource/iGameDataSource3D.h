//
// Created by Sumzeek on 11/18/2024.
//

#pragma once

#include "iGameDataSource.h"

IGAME_NAMESPACE_BEGIN

class DataSource3D : public DataSource {
public:
    I_OBJECT(DataSource3D);

    /**
     * @brief Generates data for a point in 3D space.
     *
     * Visualization:
     * ```
     * p1 +
     * ```
     *
     * @param point The coordinates of the point to be rendered.
     * @param offset Offset in the data buffer for additional customization (default is 0).
     * @return Information about the generated point data.
     */
    static DataSourceOutputInfo RequestPoint(const Point& point,
                                             size_t offset = 0);

    /**
     * @brief Generates data for a line segment in 3D space defined by two points.
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
    static DataSourceOutputInfo RequestLine(const Point& p1, const Point& p2,
                                            size_t offset = 0);

    /**
     * @brief Generates the data for a triangle in 3D space defined by three vertices.
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
    static DataSourceOutputInfo RequestTriangle(const Point& p1,
                                                const Point& p2,
                                                const Point& p3,
                                                size_t offset = 0);

    /**
     * @brief Generates data for a rectangle in 3D space defined by two diagonal corners.
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
    static DataSourceOutputInfo RequestRect(const Point& p1, const Point& p3,
                                            size_t offset = 0);

    /**
     * @brief Generates data for a cube in 3D space defined by two diagonal corners.
     *
     * Visualization:
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
     * The cube is defined by points p1 (bottom-left-front) and p7 (top-right-back).
     *
     * @param p1 The bottom-left-front corner of the cube.
     * @param p7 The top-right-back corner of the cube.
     * @param offset Offset in the data buffer for additional customization (default is 0).
     * @return Information about the generated cube data.
     */
    static DataSourceOutputInfo RequestCube(const Point& p1, const Point& p7,
                                            size_t offset = 0);

    /**
     * @brief Generates data for a circle in 3D space on a given plane.
     *
     * Visualization (top-down view):
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
     * The circle is defined by a center point, a normal vector specifying the plane, and a radius.
     *
     * @param center The center of the circle.
     * @param normal The normal vector of the plane in which the circle lies.
     * @param radius The radius of the circle.
     * @param resolution The number of discrete points used to approximate the circle's edge.
     * @param offset Offset in the data buffer for additional customization (default is 0).
     * @return Information about the generated circle data.
     */
    static DataSourceOutputInfo RequestCircle(const Point& center,
                                              const Vector3f& normal,
                                              double radius, int resolution,
                                              size_t offset = 0);

protected:
    /**
     * @brief Default constructor for DataSource3D.
     */
    DataSource3D();

    /**
     * @brief Destructor for DataSource3D.
     */
    ~DataSource3D() override;
};

IGAME_NAMESPACE_END
