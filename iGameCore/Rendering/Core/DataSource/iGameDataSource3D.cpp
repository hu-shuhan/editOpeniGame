//
// Created by Sumzeek on 11/18/2024.
//

/**
 * @class    iGameDataSource3D
 * @brief    iGameDataSource3D's brief
 */
#include "iGameDataSource3D.h"

IGAME_NAMESPACE_BEGIN
DataSource3D::DataSource3D() {}

DataSource3D::~DataSource3D(){};

DataSource::DataSourceOutputInfo DataSource3D::RequestPoint(const Point& point,
                                                            size_t offset) {
    DataSource::DataSourceOutputInfo output;

    Points points = {point};
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> pointIndices = {0};
    std::for_each(pointIndices.begin(), pointIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), pointIndices.begin(),
                             pointIndices.end());

    return output;
}

DataSource::DataSourceOutputInfo
DataSource3D::RequestLine(const Point& p1, const Point& p2, size_t offset) {
    DataSource::DataSourceOutputInfo output;

    Points points = {p1, p2};
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> pointIndices = {0, 1};
    std::for_each(pointIndices.begin(), pointIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), pointIndices.begin(),
                             pointIndices.end());

    std::vector<iguIndex> lineIndices = {0, 1};
    std::for_each(lineIndices.begin(), lineIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[1].insert(output.indices[1].end(), lineIndices.begin(),
                             lineIndices.end());

    return output;
}

DataSource::DataSourceOutputInfo DataSource3D::RequestTriangle(const Point& p1,
                                                               const Point& p2,
                                                               const Point& p3,
                                                               size_t offset) {
    DataSource::DataSourceOutputInfo output;

    Points points = {p1, p2, p3};
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> pointIndices = {0, 1, 2};
    std::for_each(pointIndices.begin(), pointIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), pointIndices.begin(),
                             pointIndices.end());

    std::vector<iguIndex> lineIndices = {0, 1, 1, 2, 2, 0};
    std::for_each(lineIndices.begin(), lineIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[1].insert(output.indices[1].end(), lineIndices.begin(),
                             lineIndices.end());

    std::vector<iguIndex> indices = {0, 1, 2};
    std::for_each(indices.begin(), indices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[2].insert(output.indices[2].end(), indices.begin(),
                             indices.end());

    return output;
}

DataSource::DataSourceOutputInfo
DataSource3D::RequestRect(const Point& p1, const Point& p3, size_t offset) {
    DataSource::DataSourceOutputInfo output;

    Point p2 = Point{p1[0], p3[1], p1[2]};
    Point p4 = Point{p3[0], p1[1], p3[2]};

    Points points = {p1, p2, p3, p4};
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> pointIndices = {0, 1, 2, 3};
    std::for_each(pointIndices.begin(), pointIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), pointIndices.begin(),
                             pointIndices.end());

    std::vector<iguIndex> lineIndices = {0, 1, 1, 2, 2, 3, 3, 0};
    std::for_each(lineIndices.begin(), lineIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[1].insert(output.indices[1].end(), lineIndices.begin(),
                             lineIndices.end());

    std::vector<iguIndex> indices = {0, 1, 2, 2, 3, 0};
    std::for_each(indices.begin(), indices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[2].insert(output.indices[2].end(), indices.begin(),
                             indices.end());

    return output;
}

DataSource::DataSourceOutputInfo
DataSource3D::RequestCube(const Point& p1, const Point& p7, size_t offset) {
    DataSource::DataSourceOutputInfo output;

    Point p2 = Point{p1[0], p1[1], p7[2]};
    Point p3 = Point{p7[0], p1[1], p7[2]};
    Point p4 = Point{p7[0], p1[1], p1[2]};
    Point p5 = Point{p1[0], p7[1], p1[2]};
    Point p6 = Point{p1[0], p7[1], p7[2]};
    Point p8 = Point{p7[0], p7[1], p1[2]};

    Points points = {p1, p2, p3, p4, p5, p6, p7, p8};
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> pointIndices = {0, 1, 2, 3, 4, 5, 6, 7};
    std::for_each(pointIndices.begin(), pointIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), pointIndices.begin(),
                             pointIndices.end());

    std::vector<iguIndex> lineIndices = {0, 1, 1, 2, 2, 3, 3, 0, 0, 4, 1, 5,
                                         2, 6, 3, 7, 4, 5, 5, 6, 6, 7, 7, 4};
    std::for_each(lineIndices.begin(), lineIndices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[1].insert(output.indices[1].end(), lineIndices.begin(),
                             lineIndices.end());

    std::vector<iguIndex> indices = {0, 1, 2, 2, 3, 0, 0, 1, 5, 5, 4, 0,
                                     0, 3, 7, 7, 4, 0, 3, 7, 6, 6, 2, 3,
                                     1, 2, 6, 6, 5, 1, 4, 5, 6, 6, 7, 4};
    std::for_each(indices.begin(), indices.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[2].insert(output.indices[2].end(), indices.begin(),
                             indices.end());

    return output;
}

DataSource::DataSourceOutputInfo
DataSource3D::RequestCircle(const Point& center, const Point& normal,
                            float radius, int resolution, size_t offset) {
    DataSource::DataSourceOutputInfo output;

    Points points;
    std::vector<iguIndex> pointIndices;
    std::vector<iguIndex> lineIndices;
    std::vector<iguIndex> indices;

    normal.normalized();

    Vector3f up(0, 0, 1);
    if (std::abs(normal.dot(up)) > 0.999f) { up = Vector3f(0, 1, 0); }

    Vector3f u = (normal.cross(up)).normalized();
    Vector3f v = (normal.cross(u)).normalized();

    for (int i = 0; i <= resolution; ++i) {
        float angle = 2.0f * IGM_PI * i / resolution;
        float x_offset = radius * std::cos(angle);
        float y_offset = radius * std::sin(angle);

        Point p = center + u * x_offset + v * y_offset;
        points.push_back(p);
        pointIndices.push_back(points.size() - 1);
    }

    for (int i = 0; i < resolution; i++) {
        lineIndices.push_back(i);
        lineIndices.push_back(i + 1);
    }

    for (int i = 0; i < resolution; i++) {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
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
