//
// Created by Sumzeek on 11/19/2024.
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
    //using Normals = std::vector<Vector3f>;
    using Indices = std::array<std::vector<iguIndex>, 3>;
    struct DataSourceOutputInfo {
        Points points;
        //Normals normals;
        Indices indices;
    };

protected:
    DataSource();
    ~DataSource() override;
};

IGAME_NAMESPACE_END
