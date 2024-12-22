/**
* @class    DataSource
* @brief    DataSource类是一个渲染数据生成的工具类，可以用其来生成指定图元的渲染数据。
* @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
*/

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
