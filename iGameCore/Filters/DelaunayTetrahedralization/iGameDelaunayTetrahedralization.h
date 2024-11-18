#ifndef IGAMEDELAUNAYTETRAHEDRALIZATION_H
#define IGAMEDELAUNAYTETRAHEDRALIZATION_H

#include "iGamePoints.h"
#include "iGameTetra.h"
#include "iGameDelaunayTetrahedralization.h"
#include <vector>

IGAME_NAMESPACE_BEGIN

class iGameDelaunayTetrahedralization : public Object {
public:
    I_OBJECT(iGameDelaunayTetrahedralization);


    static iGameDelaunayTetrahedralization::Pointer New() {
        return new iGameDelaunayTetrahedralization;
    };

    void ExecuteTest(Points::Pointer points);

    // 计算四面体外接球的中心和半径
    static void ComputeCircumcenter(const Point& p0, const Point& p1, const Point& p2, const Point& p3, Point& center, double& radius);

    // 判断点是否在四面体外接球内
    static bool IsPointInCircumball(const Point& p, const Point& center, double radius);
    
    // 创建初始四面体
    static std::vector<Tetra::Pointer> InitTetras();

    // 插入新点并进行局部修复
    void AddPointAndUpdate(std::vector<Point>& points, std::vector<Tetra::Pointer>& tetrahedra, const Point& new_point);

    std::vector<Tetra::Pointer> GenerateTetrahedraFromPoints(Points::Pointer points);
protected:
    // 构造函数和析构函数
    iGameDelaunayTetrahedralization();
    ~iGameDelaunayTetrahedralization() override = default;
private:
    

};

IGAME_NAMESPACE_END

#endif // IGAMEDELAUNAYTETRAHEDRALIZATION_H
