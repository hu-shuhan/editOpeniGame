//
// Created by Sumzeek on 11/20/2024.
//

//
// Created by Sumzeek on 11/20/2024.
//

#pragma once

#include "iGameDataSource.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

class CylinderSource : public DataSource {
public:
    I_OBJECT(CylinderSource);

    static DataSourceOutputInfo
    RequestCylinder(const Point& center, const Vector3f& normal, float height,
                    float radius, unsigned int resolution, size_t offset = 0);

protected:
    CylinderSource();
    ~CylinderSource();
};

IGAME_NAMESPACE_END