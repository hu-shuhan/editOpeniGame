//
// Created by Sumzeek on 11/19/2024.
//

/**
 * @class    iGameDataSource2D
 * @brief    iGameDataSource2D's brief
 */
#include "iGameDataSource2D.h"

IGAME_NAMESPACE_BEGIN
DataSource2D::DataSource2D() {}

DataSource2D::~DataSource2D(){};

DataSource::DataSourceOutputInfo
DataSource2D::RequestPoint(const Point2D& point, size_t offset) {
    Point3D p_3d = Point3D{point[0], point[1], 0.0f};
    return DataSource3D::RequestPoint(p_3d, offset);
}

DataSource::DataSourceOutputInfo
DataSource2D::RequestLine(const Point2D& p1, const Point2D& p2, size_t offset) {
    Point3D p1_3d = Point3D{p1[0], p1[1], 0.0f};
    Point3D p2_3d = Point3D{p2[0], p2[1], 0.0f};
    return DataSource3D::RequestLine(p1_3d, p2_3d, offset);
}

DataSource::DataSourceOutputInfo
DataSource2D::RequestTriangle(const Point2D& p1, const Point2D& p2,
                              const Point2D& p3, size_t offset) {
    Point3D p1_3d = Point3D{p1[0], p1[1], 0.0f};
    Point3D p2_3d = Point3D{p2[0], p2[1], 0.0f};
    Point3D p3_3d = Point3D{p3[0], p3[1], 0.0f};
    return DataSource3D::RequestTriangle(p1_3d, p2_3d, p3_3d, offset);
}

DataSource::DataSourceOutputInfo
DataSource2D::RequestRect(const Point2D& p1, const Point2D& p3, size_t offset) {
    Point3D p1_3d = Point3D{p1[0], p1[1], 0.0f};
    Point3D p3_3d = Point3D{p3[0], p3[1], 0.0f};
    return DataSource3D::RequestRect(p1_3d, p3_3d, offset);
}

DataSource::DataSourceOutputInfo
DataSource2D::RequestCircle(const Point2D& center, double radius,
                            int resolution, size_t offset) {
    Point3D center_3d = Point3D{center[0], center[1], 0.0f};
    Point3D normal = Point3D{0.0f, 0.0f, -1.0f};
    return DataSource3D::RequestCircle(center_3d, normal, radius, resolution,
                                       offset);
}

IGAME_NAMESPACE_END
