//
// Created by Sumzeek on 11/18/2024.
//

#pragma once

#include "iGameObject.h"
#include "iGamePoints.h"
#include "igm/igm.h"

IGAME_NAMESPACE_BEGIN

class DataSource : public Object {
public:
    I_OBJECT(DataSource);

    using Points = std::vector<Point>;
    using Indices = std::array<std::vector<iguIndex>, 3>;
    struct DataSourceOutputInfo {
        Points points;
        Indices indices;
    };

    static DataSourceOutputInfo RequestPoint(const Point& point,
                                             const size_t offset = 0);
    static DataSourceOutputInfo RequestLine(const Point& p1, const Point& p2,
                                            const size_t offset = 0);
    static DataSourceOutputInfo RequestTriangle(const Point& p1,
                                                const Point& p2,
                                                const Point& p3,
                                                const size_t offset = 0);

    /* rectangle
    * p2 +------------+ p3
    *    |            |
    *    |            |
    *    |            |
    * p1 +------------+ p4
    */
    static DataSourceOutputInfo RequestRect(const Point& p1, const Point& p3,
                                            const size_t offset = 0);

    /* cube
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
    */
    static DataSourceOutputInfo RequestCube(const Point& p1, const Point& p7,
                                            const size_t offset = 0);

    /* draw circle (on plane)
    *            *******
    *         **         **
    *       **             **
    *      *                 *
    *     *        * center   *
    *      *                 *
    *       **             **
    *         **         **
    *            *******
    *
    * @param center: Center of a circle
    * @param normal: The normal vector of the plane in which the circle is located
    * @param radius: The radius of circle
    * @param resolution: The number of discrete points on the edge of a circle
    */
    static DataSourceOutputInfo RequestCircle(const Point& center,
                                              const Vector3f& normal,
                                              double radius, int resolution,
                                              const size_t offset = 0);

    /* draw sphere (3D)
    *            *******
    *         **         **
    *       **             **
    *      *                 *
    *     *        * center   *
    *      *                 *
    *       **             **
    *         **         **
    *            *******
    *
    * @param center: Center of a sphere
    * @param radius: The radius of sphere
    * @param n: The degree of discretization
    */
    //static DataSourceOutputInfo DrawSphere(const Point& center, double radius,
    //                                       const size_t offset = 0);

protected:
    DataSource();
    ~DataSource() override;
};

IGAME_NAMESPACE_END