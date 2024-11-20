//
// Created by Sumzeek on 11/20/2024.
//

#pragma once

#include "iGameDataSource.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

class ConeSource : public DataSource {
public:
    I_OBJECT(ConeSource);

    static DataSourceOutputInfo
    RequestCone(const Point& center, const Vector3f& normal, float height,
                float radius, unsigned int resolution, size_t offset = 0);

    static DataSourceOutputInfo
    RequestPyramid(const Point& center, const Vector3f& normal, float height,
                   float radius, unsigned int stackCount,
                   unsigned int sectorCount, size_t offset = 0);

    static DataSourceOutputInfo
    RequestFrustum(const Point& center, const Vector3f& normal, float height,
                   float baseRadius, float topRadius, unsigned int resolution,
                   size_t offset = 0 /*, bool capped = true*/);

protected:
    ConeSource();
    ~ConeSource();
};

IGAME_NAMESPACE_END