//
// Created by Sumzeek on 11/18/2024.
//

/**
 * @class    iGameDataSource
 * @brief    iGameDataSource's brief
 */
#include "iGameDataSource.h"

IGAME_NAMESPACE_BEGIN
DataSource::DataSource() {}

DataSource::~DataSource(){};

DataSource::DataSourceOutputInfo DataSource::RequestPoint(const Point& point,
                                                          const size_t offset) {
    DataSource::DataSourceOutputInfo output;

    Points points = {point};
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> index0 = {0};
    std::for_each(index0.begin(), index0.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), index0.begin(),
                             index0.end());

    return output;
}

DataSource::DataSourceOutputInfo
DataSource::RequestLine(const Point& p1, const Point& p2, const size_t offset) {
    DataSource::DataSourceOutputInfo output;

    Points points = {p1, p2};
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> index0 = {0, 1};
    std::for_each(index0.begin(), index0.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), index0.begin(),
                             index0.end());

    std::vector<iguIndex> index1 = {0, 1};
    std::for_each(index1.begin(), index1.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[1].insert(output.indices[1].end(), index1.begin(),
                             index1.end());

    return output;
}

DataSource::DataSourceOutputInfo
DataSource::RequestTriangle(const Point& p1, const Point& p2, const Point& p3,
                            const size_t offset) {
    DataSource::DataSourceOutputInfo output;

    Points points = {p1, p2, p3};
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> index0 = {0, 1, 2};
    std::for_each(index0.begin(), index0.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), index0.begin(),
                             index0.end());

    std::vector<iguIndex> index1 = {0, 1, 1, 2, 2, 0};
    std::for_each(index1.begin(), index1.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[1].insert(output.indices[1].end(), index1.begin(),
                             index1.end());

    std::vector<iguIndex> index2 = {0, 1, 2};
    std::for_each(index2.begin(), index2.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[2].insert(output.indices[2].end(), index2.begin(),
                             index2.end());

    return output;
}

DataSource::DataSourceOutputInfo
DataSource::RequestRect(const Point& p1, const Point& p3, const size_t offset) {
    DataSource::DataSourceOutputInfo output;

    auto p2 = Point{p1[0], p3[1], p1[2]};
    auto p4 = Point{p3[0], p1[1], p3[2]};

    Points points = {p1, p2, p3, p4};
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> index0 = {0, 1, 2, 3};
    std::for_each(index0.begin(), index0.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), index0.begin(),
                             index0.end());

    std::vector<iguIndex> index1 = {0, 1, 1, 2, 2, 3, 3, 0};
    std::for_each(index1.begin(), index1.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[1].insert(output.indices[1].end(), index1.begin(),
                             index1.end());

    std::vector<iguIndex> index2 = {0, 1, 2, 2, 3, 0};
    std::for_each(index2.begin(), index2.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[2].insert(output.indices[2].end(), index2.begin(),
                             index2.end());

    return output;
}

DataSource::DataSourceOutputInfo
DataSource::RequestCube(const Point& p1, const Point& p7, const size_t offset) {
    DataSource::DataSourceOutputInfo output;

    auto p2 = Point{p1[0], p1[1], p7[2]};
    auto p3 = Point{p7[0], p1[1], p7[2]};
    auto p4 = Point{p7[0], p1[1], p1[2]};
    auto p5 = Point{p1[0], p7[1], p1[2]};
    auto p6 = Point{p1[0], p7[1], p7[2]};
    auto p8 = Point{p7[0], p7[1], p1[2]};

    Points points = {p1, p2, p3, p4, p5, p6, p7, p8};
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> index0 = {0, 1, 2, 3, 4, 5, 6, 7};
    std::for_each(index0.begin(), index0.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), index0.begin(),
                             index0.end());

    std::vector<iguIndex> index1 = {0, 1, 1, 2, 2, 3, 3, 0, 0, 4, 1, 5,
                                    2, 6, 3, 7, 4, 5, 5, 6, 6, 7, 7, 4};
    std::for_each(index1.begin(), index1.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[1].insert(output.indices[1].end(), index1.begin(),
                             index1.end());

    std::vector<iguIndex> index2 = {0, 1, 2, 2, 3, 0, 0, 1, 5, 5, 4, 0,
                                    0, 3, 7, 7, 4, 0, 3, 7, 6, 6, 2, 3,
                                    1, 2, 6, 6, 5, 1, 4, 5, 6, 6, 7, 4};
    std::for_each(index2.begin(), index2.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[2].insert(output.indices[2].end(), index2.begin(),
                             index2.end());

    return output;
}

DataSource::DataSourceOutputInfo
DataSource::RequestCircle(const Point& center, const Vector3f& normal,
                          double radius, int resolution, const size_t offset) {
    DataSource::DataSourceOutputInfo output;

    normal.normalized();

    Vector3f up(0, 0, 1);
    if (std::abs(normal.dot(up)) > 0.999f) { up = Vector3f(0, 1, 0); }

    Vector3f u = (normal.cross(up)).normalized();
    Vector3f v = (normal.cross(u)).normalized();

    Points points;
    for (int i = 0; i <= resolution; ++i) {
        double angle = 2.0 * IGM_PI * i / resolution;
        float x_offset = radius * std::cos(angle);
        float y_offset = radius * std::sin(angle);

        Point p = center + u * x_offset + v * y_offset;
        points.push_back(p);
    }
    output.points.insert(output.points.end(), points.begin(), points.end());

    std::vector<iguIndex> index0;
    for (int i = 0; i < output.points.size(); ++i) { index0.push_back(i); }
    std::for_each(index0.begin(), index0.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[0].insert(output.indices[0].end(), index0.begin(),
                             index0.end());

    std::vector<iguIndex> index1;
    for (int i = 0; i < resolution; i++) {
        index1.push_back(i);
        index1.push_back(i + 1);
    }
    std::for_each(index1.begin(), index1.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[1].insert(output.indices[1].end(), index1.begin(),
                             index1.end());

    std::vector<iguIndex> index2;
    for (int i = 0; i < resolution; i++) {
        index2.push_back(0);
        index2.push_back(i + 1);
        index2.push_back(i + 2);
    }
    std::for_each(index2.begin(), index2.end(),
                  [offset](iguIndex& value) { value += offset; });
    output.indices[2].insert(output.indices[2].end(), index2.begin(),
                             index2.end());

    return output;
}

IGAME_NAMESPACE_END
