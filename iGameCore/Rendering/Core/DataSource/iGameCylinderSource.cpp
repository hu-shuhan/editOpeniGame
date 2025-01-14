/**
 * @class    iGameCylinderSource
 * @brief    iGameCylinderSource's brief
 */

#include "iGameCylinderSource.h"
#include <algorithm>

IGAME_NAMESPACE_BEGIN

CylinderSource::CylinderSource() {}

CylinderSource::~CylinderSource() {}

DataSource::DataSourceOutputInfo
CylinderSource::RequestCylinder(const Point& center, const Vector3f& normal,
                                float height, float radius,
                                unsigned int resolution, size_t offset) {
    DataSource::DataSourceOutputInfo output;

    Points points;
    std::vector<iguIndex> pointIndices;
    std::vector<iguIndex> lineIndices;
    std::vector<iguIndex> indices;

    normal.normalized();
    Vector3f topCenter = center + normal * height;

    Vector3f up(0, 0, 1);
    if (std::abs(normal.dot(up)) > 0.999f) { up = Vector3f(0, 1, 0); }

    Vector3f u = (normal.cross(up)).normalized();
    Vector3f v = (normal.cross(u)).normalized();

    // add bottom vertex
    for (int i = 1; i <= resolution; ++i) {
        float angle = 2.0 * IGM_PI * i / resolution;
        float x_offset = radius * std::cos(angle);
        float y_offset = radius * std::sin(angle);

        Point p = center + u * x_offset + v * y_offset;
        points.push_back(p);
        pointIndices.push_back(points.size() - 1);
    }

    // add top vertex
    for (int i = 1; i <= resolution; ++i) {
        float angle = 2.0 * IGM_PI * i / resolution;
        float x_offset = radius * std::cos(angle);
        float y_offset = radius * std::sin(angle);

        Point p = topCenter + u * x_offset + v * y_offset;
        points.push_back(p);
        pointIndices.push_back(points.size() - 1);
    }

    // add bottom surface
    points.push_back(center);
    auto centerIndex = points.size() - 1;
    for (int i = 0; i < resolution; ++i) {
        int j = (i + 1) % resolution;
        indices.push_back(centerIndex);
        indices.push_back(i);
        indices.push_back(j);
        lineIndices.push_back(i);
        lineIndices.push_back(j);
    }

    // add top surface
    points.push_back(topCenter);
    auto topCenterIndex = points.size() - 1;
    for (int i = 0; i < resolution; ++i) {
        int j = (i + 1) % resolution;
        indices.push_back(topCenterIndex);
        indices.push_back(i + resolution);
        indices.push_back(j + resolution);
        lineIndices.push_back(i + resolution);
        lineIndices.push_back(j + resolution);
    }

    // add side surface
    for (int i = 0; i < resolution; ++i) {
        int k1 = i;
        int k2 = (i + 1) % resolution;
        int k3 = resolution + k1;
        int k4 = resolution + k2;
        // 4 vertices of a quad
        // k3--k4
        // | / |
        // k1--k2
        indices.push_back(k1);
        indices.push_back(k2);
        indices.push_back(k4);
        indices.push_back(k1);
        indices.push_back(k3);
        indices.push_back(k4);
        lineIndices.push_back(k1);
        lineIndices.push_back(k3);
    }

    output.points.insert(output.points.end(), points.begin(), points.end());

    std::for_each(pointIndices.begin(), pointIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), pointIndices.begin(),
                             pointIndices.end());

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

IGAME_NAMESPACE_END