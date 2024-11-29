//
// Created by Sumzeek on 11/19/2024.
//

#pragma once

#include "iGameDataSource.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

/**
 * @class SphereSource
 * @brief A data source for generating spherical geometries.
 *
 * SphereSource provides static methods to create different types of spheres:
 * - Standard sphere (latitude/longitude grid).
 * - Icosphere (subdivided icosahedron).
 * - Cube-mapped sphere (generated from cube faces mapped to a sphere).
 */
class SphereSource : public DataSource {
public:
    I_OBJECT(SphereSource);

    /**
     * @brief Generate a standard sphere based on sector and stack division.
     *
     * @param center The center of the sphere in 3D space.
     * @param radius The radius of the sphere.
     * @param sectorCount Number of sectors (longitude divisions).
     * @param stackCount Number of stacks (latitude divisions).
     * @param offset Index offset for vertices.
     * @return DataSourceOutputInfo containing the vertices and indices.
     */
    static DataSourceOutputInfo RequestSphere(const Point& center, float radius,
                                              unsigned int stackCount,
                                              unsigned int sectorCount,
                                              size_t offset = 0);

    /**
     * @brief Generate an icosphere by recursively subdividing an icosahedron.
     *
     * @param center The center of the sphere in 3D space.
     * @param radius The radius of the sphere.
     * @param subdivision Number of recursive subdivisions.
     * @param offset Index offset for vertices.
     * @return DataSourceOutputInfo containing the vertices and indices.
     */
    static DataSourceOutputInfo RequestIcoSphere(const Point& center,
                                                 float radius,
                                                 unsigned int subdivision,
                                                 size_t offset = 0);

    /**
     * @brief Generate a cube-mapped sphere by projecting cube faces onto a sphere.
     *
     * @param center The center of the sphere in 3D space.
     * @param radius The radius of the sphere.
     * @param vertexCountPerRow Number of vertices per row on each cube face.
     * @param offset Index offset for vertices.
     * @return DataSourceOutputInfo containing the vertices and indices.
     */
    static DataSourceOutputInfo
    RequestCubeSphere(const Point& center, float radius,
                      unsigned int vertexCountPerRow, size_t offset = 0);

protected:
    // Protected constructor and destructor to prevent direct instantiation.
    SphereSource();
    ~SphereSource() override;

    ///////////////////////////////////////////////////////////////////////////////
    // Helper methods for internal geometry computation
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Calculate the scale factor to normalize a vector to a given length.
     *
     * @param v The vector to be scaled.
     * @param length The desired length of the vector.
     * @return The scaling factor to normalize the vector.
     */
    static float computeScaleForLength(const Point v, float length);

    /**
     * @brief Interpolate between two vertices with normalization to a specific length.
     *
     * @param v1 The first vertex.
     * @param v2 The second vertex.
     * @param t Interpolation factor (0.0 to 1.0).
     * @param length The desired length of the interpolated vertex.
     * @param newV The result of the interpolation (output parameter).
     */
    static void interpolateVertex(const Point v1, const Point v2, float t,
                                  float length, Point& newV);

    /**
     * @brief Perform linear interpolation between two scalar values.
     *
     * @param from The starting value.
     * @param to The ending value.
     * @param alpha The interpolation factor (0.0 to 1.0).
     * @return The interpolated value.
     */
    static float lerp(float from, float to, float alpha);

    /**
     * @brief Compute the 12 vertices of an icosahedron.
     *
     * The icosahedron is constructed using spherical coordinates:
     * - The north pole is at (0, 0, r) and the south pole at (0, 0, -r).
     * - Five vertices are placed by rotating 72° at an elevation of 26.57° (atan(1/2)).
     * - Another five vertices are placed similarly at an elevation of -26.57°.
     *
     * @param radius The radius of the icosahedron.
     * @return Points representing the 12 vertices of the icosahedron.
     */
    static DataSource::Points computeIcosahedronVertices(float radius);

    /**
     * @brief Generate unit-length points for a single face (+X face) of a cube.
     *
     * These points are distributed in a regular grid, normalized to form a spherical surface.
     *
     * @param pointsPerRow The number of points per row in the grid.
     * @return Points representing the vertices on the +X face of the cube.
     */
    static DataSource::Points getUnitPositiveX(unsigned int pointsPerRow);
};

IGAME_NAMESPACE_END
