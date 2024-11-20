//
// Created by Sumzeek on 11/19/2024.
//

/**
 * @class    iGameSphereSource
 * @brief    iGameSphereSource's brief
 */
#include "iGameSphereSource.h"

IGAME_NAMESPACE_BEGIN

SphereSource::SphereSource() {}

SphereSource::~SphereSource() {}

DataSource::DataSourceOutputInfo
SphereSource::RequestSphere(const Point& center, float radius,
                            unsigned int stackCount, unsigned int sectorCount,
                            size_t offset) {
    DataSource::DataSourceOutputInfo output;

    Points points;
    float x, y, z, xy; // vertex position

    float sectorStep = 2 * IGM_PI / sectorCount;
    float stackStep = IGM_PI / stackCount;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stackCount; ++i) {
        stackAngle = IGM_PI / 2 - i * stackStep; // starting from pi/2 to -pi/2
        xy = radius * std::cos(stackAngle);      // r * cos(u)
        z = radius * std::sin(stackAngle);       // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // first and last vertices have same position and normal, but different tex coords
        for (int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep; // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)
            points.push_back(Point{x, y, z});
        }
    }

    output.points.insert(output.points.end(), points.begin(), points.end());

    // generate CCW index list of sphere triangles
    // k1--k1+1
    // |  / |
    // | /  |
    // k2--k2+1
    std::vector<iguIndex> indices;
    std::vector<iguIndex> lineIndices;
    int k1, k2;
    for (int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1); // beginning of current stack
        k2 = k1 + sectorCount + 1;  // beginning of next stack

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            // k1+1 => k2 => k2+1
            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }

            // store indices for lines
            // vertical lines for all stacks, k1 => k2
            lineIndices.push_back(k1);
            lineIndices.push_back(k2);
            if (i != 0) // horizontal lines except 1st stack, k1 => k+1
            {
                lineIndices.push_back(k1);
                lineIndices.push_back(k1 + 1);
            }
        }
    }

    std::vector<iguIndex> index0;
    for (int i = 0; i < output.points.size(); ++i) { index0.push_back(i); }
    std::for_each(index0.begin(), index0.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), index0.begin(),
                             index0.end());

    std::for_each(lineIndices.begin(), lineIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[1].insert(output.indices[1].end(), lineIndices.begin(),
                             lineIndices.end());

    std::for_each(indices.begin(), indices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[2].insert(output.indices[2].end(), indices.begin(),
                             indices.end());

    return output;
}

DataSource::DataSourceOutputInfo
SphereSource::RequestIcoSphere(const Point& center, float radius,
                               unsigned int subdivision, size_t offset) {
    DataSourceOutputInfo output;

    Points points;
    std::vector<iguIndex> indices;
    std::vector<iguIndex> lineIndices;

    // compute 12 vertices of icosahedron
    auto tmpVertices = computeIcosahedronVertices(radius);

    Point v0, v1, v2, v3, v4, v11; // vertex positions
    unsigned int index = 0;

    // compute 20 tiangles of icosahedron first
    v0 = tmpVertices[0];   // 1st vertex (north pole)
    v11 = tmpVertices[11]; // 12th vertex (south pole)
    for (int i = 1; i <= 5; ++i) {
        // 4 vertices in the 2nd row
        v1 = tmpVertices[i];
        if (i < 5) {
            v2 = tmpVertices[i + 1];
        } else {
            v2 = tmpVertices[1];
        }

        v3 = tmpVertices[i + 5];
        if ((i + 5) < 10) {
            v4 = tmpVertices[i + 6];
        } else {
            v4 = tmpVertices[6];
        }

        // add a triangle in 1st row
        points.push_back(v0);
        points.push_back(v1);
        points.push_back(v2);
        indices.push_back(index);
        indices.push_back(index + 1);
        indices.push_back(index + 2);

        // add 2 triangles in 2nd row
        points.push_back(v1);
        points.push_back(v3);
        points.push_back(v2);
        indices.push_back(index + 3);
        indices.push_back(index + 4);
        indices.push_back(index + 5);

        points.push_back(v2);
        points.push_back(v3);
        points.push_back(v4);
        indices.push_back(index + 6);
        indices.push_back(index + 7);
        indices.push_back(index + 8);

        // add a triangle in 3rd row
        points.push_back(v3);
        points.push_back(v11);
        points.push_back(v4);
        indices.push_back(index + 9);
        indices.push_back(index + 10);
        indices.push_back(index + 11);

        // add 6 edge lines per iteration
        //  i
        //  /   /   /   /   /       : (i, i+1)                              //
        // /__ /__ /__ /__ /__                                              //
        // \  /\  /\  /\  /\  /     : (i+3, i+4), (i+3, i+5), (i+4, i+5)    //
        //  \/__\/__\/__\/__\/__                                            //
        //   \   \   \   \   \      : (i+9,i+10), (i+9, i+11)               //
        //    \   \   \   \   \                                             //
        lineIndices.push_back(index);
        lineIndices.push_back(index + 1);
        lineIndices.push_back(index + 3);
        lineIndices.push_back(index + 4);
        lineIndices.push_back(index + 3);
        lineIndices.push_back(index + 5);
        lineIndices.push_back(index + 4);
        lineIndices.push_back(index + 5);
        lineIndices.push_back(index + 9);
        lineIndices.push_back(index + 10);
        lineIndices.push_back(index + 9);
        lineIndices.push_back(index + 11);

        // next index
        index += 12;
    }

    // subdivide icosahedron

    if (subdivision <= 1) {
        igDebug("Subdivision is less than or equal to 1. Skipping further "
                "processing.");
    } else {
        unsigned int subVertexCount = (subdivision + 1) * (subdivision + 2) / 2;
        Points newVs(subVertexCount);

        Points tmpVertices;
        std::vector<iguIndex> tmpIndices;
        Point v1, v2, v3;          // ptr to original vertices of a triangle
        Point newV1, newV2, newV3; // new vertex positions
        int i, j, k;
        float a;                // lerp alpha
        unsigned int index = 0; // new index value
        unsigned int i1, i2;    // indices

        // copy prev arrays
        tmpVertices = points;
        tmpIndices = indices;

        // clear prev arrays
        points.clear();
        indices.clear();
        lineIndices.clear();

        int indexCount = (int) tmpIndices.size();
        for (i = 0; i < indexCount; i += 3) {
            // get 3 vertice and texcoords of a triangle of icosahedron
            v1 = tmpVertices[tmpIndices[i]];
            v2 = tmpVertices[tmpIndices[i + 1]];
            v3 = tmpVertices[tmpIndices[i + 2]];

            // add top vertex and textcoord (x,y,z), (s,t)
            newVs.clear();
            newVs.push_back(v1);

            // find new vertices by subdividing edges
            for (j = 1; j <= subdivision; ++j) {
                a = (float) j / subdivision; // lerp alpha

                // find 2 end vertices on the edges of the current row
                //          v1           //
                //         / \           // if N = 3,
                //        *---*          // lerp alpha = 1 / N
                //       / \ / \         //
                // newV1*---*---* newV2  // lerp alpha = 2 / N
                //     / \newV3/ \       //
                //    v2--*---*---v3     //
                interpolateVertex(v1, v2, a, radius, newV1);
                interpolateVertex(v1, v3, a, radius, newV2);
                for (k = 0; k <= j; ++k) {
                    if (k == 0) // new vertex on the left edge, newV1
                    {
                        newVs.push_back(newV1);
                    } else if (k == j) // new vertex on the right edge, newV2
                    {
                        newVs.push_back(newV2);
                    } else // new vertices between newV1 and newV2
                    {
                        a = (float) k / j;
                        interpolateVertex(newV1, newV2, a, radius, newV3);
                        newVs.push_back(newV3);
                    }
                }
            }

            // compute sub-triangles from new vertices
            //      /           //
            //   V1*---*-       // prev row
            //    / \ /         //
            // V2*---*V3-       // curr row
            //  /               //
            for (j = 1; j <= subdivision; ++j) {
                for (k = 0; k < j; ++k) {
                    // indices
                    i1 = (j - 1) * j / 2 + k; // index from prev row
                    i2 = j * (j + 1) / 2 + k; // index from curr row

                    v1 = newVs[i1];
                    v2 = newVs[i2];
                    v3 = newVs[i2 + 1];
                    points.push_back(v1);
                    points.push_back(v2);
                    points.push_back(v3);

                    // add indices
                    indices.push_back(index);
                    indices.push_back(index + 1);
                    indices.push_back(index + 2);

                    // add indices for edge lines
                    lineIndices.push_back(index);
                    lineIndices.push_back(index + 1);
                    lineIndices.push_back(index + 1);
                    lineIndices.push_back(index + 2);

                    index += 3; // next index

                    // if K is not the last, add adjacent triangle
                    if (k < (j - 1)) {
                        i2 = i1 + 1; // next of the prev row
                        v2 = newVs[i2];
                        points.push_back(v1);
                        points.push_back(v3);
                        points.push_back(v2);
                        indices.push_back(index);
                        indices.push_back(index + 1);
                        indices.push_back(index + 2);
                        lineIndices.push_back(index);
                        lineIndices.push_back(index + 1);
                        lineIndices.push_back(index);
                        lineIndices.push_back(index + 2);
                        index += 3;
                    }
                }
            }
        }
    }

    std::for_each(points.begin(), points.end(),
                  [center](Point& p) { p += center; });
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> index0;
    for (int i = 0; i < output.points.size(); ++i) { index0.push_back(i); }
    std::for_each(index0.begin(), index0.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), index0.begin(),
                             index0.end());

    std::for_each(lineIndices.begin(), lineIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[1].insert(output.indices[1].end(), lineIndices.begin(),
                             lineIndices.end());

    std::for_each(indices.begin(), indices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[2].insert(output.indices[2].end(), indices.begin(),
                             indices.end());

    return output;
}

DataSource::DataSourceOutputInfo
SphereSource::RequestCubeSphere(const Point& center, float radius,
                                unsigned int vertexCountPerRow, size_t offset) {
    DataSourceOutputInfo output;

    Points points;
    std::vector<iguIndex> indices;
    std::vector<iguIndex> lineIndices;

    // generate unit-length verties in +X face
    Points unitVertices = getUnitPositiveX(vertexCountPerRow);

    unsigned int k = 0, k1, k2, i1, i2; // indices
    Point v1, v2, v3, v4;               // tmp vertices

    // +X face
    for (unsigned int i = 0; i < vertexCountPerRow - 1; ++i) {
        k1 = i * vertexCountPerRow;  // index at curr row
        k2 = k1 + vertexCountPerRow; // index at next row

        for (unsigned int j = 0; j < vertexCountPerRow - 1; ++j, ++k1, ++k2) {
            i1 = k1;
            i2 = k2;

            // 4 vertices of a quad
            // v1--v3
            // | / |
            // v2--v4
            v1 = unitVertices[i1];
            v2 = unitVertices[i2];
            v3 = unitVertices[i1 + 1];
            v4 = unitVertices[i2 + 1];

            // resize vertices by sphere radius
            v1 = v1 * radius;
            v2 = v2 * radius;
            v3 = v3 * radius;
            v4 = v4 * radius;

            // add 4 vertex attributes
            points.push_back(v1);
            points.push_back(v2);
            points.push_back(v3);
            points.push_back(v4);

            // add indices of 2 triangles
            indices.push_back(k);
            indices.push_back(k + 1);
            indices.push_back(k + 2);
            indices.push_back(k + 2);
            indices.push_back(k + 1);
            indices.push_back(k + 3);

            // add line indices; top and left
            lineIndices.push_back(k); // left
            lineIndices.push_back(k + 1);
            lineIndices.push_back(k); // top
            lineIndices.push_back(k + 2);

            k += 4; // next
        }
    }

    // array size and index for building next face
    unsigned int startIndex;              // starting index for next face
    int vertexSize = (int) points.size(); // vertex array size of +X face
    int indexSize = (int) indices.size(); // index array size of +X face
    int lineIndexSize = (int) lineIndices.size(); // line index size of +X face

    // build -X face by negating x and z values
    startIndex = points.size();
    for (int i = 0; i < vertexSize; i++) {
        auto p = points[i];
        points.push_back(Point{-p[0], p[1], -p[2]});
    }
    for (int i = 0; i < indexSize; ++i) {
        indices.push_back(startIndex + indices[i]);
    }
    for (int i = 0; i < lineIndexSize; i += 4) {
        // left and bottom lines
        lineIndices.push_back(startIndex + i); // left
        lineIndices.push_back(startIndex + i + 1);
        lineIndices.push_back(startIndex + i + 1); // bottom
        lineIndices.push_back(startIndex + i + 3);
    }

    // build +Y face by swapping x=>y, y=>-z, z=>-x
    startIndex = points.size();
    for (int i = 0; i < vertexSize; i++) {
        auto p = points[i];
        points.push_back(Point{-p[2], p[0], -p[1]});
    }
    for (int i = 0; i < indexSize; ++i) {
        indices.push_back(startIndex + indices[i]);
    }
    for (int i = 0; i < lineIndexSize; ++i) {
        // top and left lines (same as +X)
        lineIndices.push_back(startIndex + lineIndices[i]);
    }

    // build -Y face by swapping x=>-y, y=>z, z=>-x
    startIndex = points.size();
    for (int i = 0; i < vertexSize; i++) {
        auto p = points[i];
        points.push_back(Point{-p[2], -p[0], p[1]});
    }
    for (int i = 0; i < indexSize; ++i) {
        indices.push_back(startIndex + indices[i]);
    }
    for (int i = 0; i < lineIndexSize; i += 4) {
        // top and right lines
        lineIndices.push_back(startIndex + i); // top
        lineIndices.push_back(startIndex + i + 2);
        lineIndices.push_back(startIndex + 2 + i); // right
        lineIndices.push_back(startIndex + 3 + i);
    }

    // build +Z face by swapping x=>z, z=>-x
    startIndex = points.size();
    for (int i = 0; i < vertexSize; i++) {
        auto p = points[i];
        points.push_back(Point{-p[2], p[1], p[0]});
    }
    for (int i = 0; i < indexSize; ++i) {
        indices.push_back(startIndex + indices[i]);
    }
    for (int i = 0; i < lineIndexSize; ++i) {
        // top and left lines (same as +X)
        lineIndices.push_back(startIndex + lineIndices[i]);
    }

    // build -Z face by swapping x=>-z, z=>x
    startIndex = points.size();
    for (int i = 0; i < vertexSize; i++) {
        auto p = points[i];
        points.push_back(Point{p[2], p[1], -p[0]});
    }
    for (int i = 0; i < indexSize; ++i) {
        indices.push_back(startIndex + indices[i]);
    }
    for (int i = 0; i < lineIndexSize; i += 4) {
        // left and bottom lines
        lineIndices.push_back(startIndex + i); // left
        lineIndices.push_back(startIndex + i + 1);
        lineIndices.push_back(startIndex + i + 1); // bottom
        lineIndices.push_back(startIndex + i + 3);
    }

    std::for_each(points.begin(), points.end(),
                  [center](Point& p) { p += center; });
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> index0;
    for (int i = 0; i < output.points.size(); ++i) { index0.push_back(i); }
    std::for_each(index0.begin(), index0.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), index0.begin(),
                             index0.end());

    std::for_each(lineIndices.begin(), lineIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[1].insert(output.indices[1].end(), lineIndices.begin(),
                             lineIndices.end());

    std::for_each(indices.begin(), indices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[2].insert(output.indices[2].end(), indices.begin(),
                             indices.end());

    return output;
}

float SphereSource::computeScaleForLength(const Point v, float length) {
    // and normalize the vector then re-scale to new radius
    return length / v.length();
}

void SphereSource::interpolateVertex(const Point v1, const Point v2, float t,
                                     float length, Point& newV) {
    newV[0] = SphereSource::lerp(v1[0], v2[0], t);
    newV[1] = SphereSource::lerp(v1[1], v2[1], t);
    newV[2] = SphereSource::lerp(v1[2], v2[2], t);
    float scale = SphereSource::computeScaleForLength(newV, length);
    newV = newV * scale;
}

float SphereSource::lerp(float from, float to, float alpha) {
    return from + alpha * (to - from);
}

DataSource::Points SphereSource::computeIcosahedronVertices(float radius) {
    Points points(12);

    const float PI = IGM_PI;
    const float H_ANGLE = PI / 180 * 72;   // 72 degree = 360 / 5
    const float V_ANGLE = atanf(1.0f / 2); // elevation = 26.565 degree

    int i1, i2;                            // indices
    float z, xy;                           // coords
    float hAngle1 = -PI / 2 - H_ANGLE / 2; // start from -126 deg at 2nd row
    float hAngle2 = -PI / 2;               // start from -90 deg at 3rd row

    // the first top vertex (0, 0, r)
    points[0] = Vector3f{0, 0, radius};

    // 10 vertices at 2nd and 3rd rows
    for (int i = 1; i <= 5; ++i) {
        i1 = i;     // for 2nd row
        i2 = i + 5; // for 3rd row

        z = radius * sinf(V_ANGLE); // elevaton
        xy = radius * cosf(V_ANGLE);

        points[i1] =
                Vector3f{xy * std::cos(hAngle1), xy * std::sin(hAngle1), z};
        points[i2] =
                Vector3f{xy * std::cos(hAngle2), xy * std::sin(hAngle2), -z};

        // next horizontal angles
        hAngle1 += H_ANGLE;
        hAngle2 += H_ANGLE;
    }

    // the last bottom vertex (0, 0, -r)
    i1 = 11;
    points[i1] = Vector3f{0, 0, -radius};

    return points;
}

DataSource::Points SphereSource::getUnitPositiveX(unsigned int pointsPerRow) {
    Points points;

    const float DEG2RAD = std::acos(-1.0f) / 180.0f;

    std::vector<float> vertices;
    Vector3f n1; // normal of longitudinal plane rotating along Y-axis
    Vector3f n2; // normal of latitudinal plane rotating along Z-axis
    Point v;     // direction vector intersecting 2 planes, n1 x n2
    float a1;    // longitudinal angle along y-axis
    float a2;    // latitudinal angle
    float scale;

    // rotate latitudinal plane from 45 to -45 degrees along Z-axis
    for (unsigned int i = 0; i < pointsPerRow; ++i) {
        // normal for latitudinal plane
        a2 = DEG2RAD * (45.0f - 90.0f * i / (pointsPerRow - 1));
        n2[0] = -std::sin(a2);
        n2[1] = std::cos(a2);
        n2[2] = 0;

        // rotate longitudinal plane from -45 to 45 along Y-axis
        for (unsigned int j = 0; j < pointsPerRow; ++j) {
            // normal for longitudinal plane
            a1 = DEG2RAD * (-45.0f + 90.0f * j / (pointsPerRow - 1));
            n1[0] = -std::sin(a1);
            n1[1] = 0;
            n1[2] = -std::cos(a1);

            // find direction vector of intersected line, n1 x n2
            v[0] = n1[1] * n2[2] - n1[2] * n2[1];
            v[1] = n1[2] * n2[0] - n1[0] * n2[2];
            v[2] = n1[0] * n2[1] - n1[1] * n2[0];

            // normalize direction vector
            scale = computeScaleForLength(v, 1);
            v = v * scale;

            points.push_back(v);
        }
    }

    return points;
}

IGAME_NAMESPACE_END
