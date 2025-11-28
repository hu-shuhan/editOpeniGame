#ifndef iGameLagrangeVolume_h
#define iGameLagrangeVolume_h

#include "iGameCell.h" // 直接继承自 Cell

IGAME_NAMESPACE_BEGIN

// 所有3D拉格朗日单元的抽象基类
class LagrangeVolume : public Cell {
public:
    // 设置/获取单元的阶数
    void SetOrder(int order) { m_Order = order; }
    int GetOrder() const { return m_Order; }

    // 声明3D单元必须具备的纯虚函数接口，由子类(如LagrangeHexahedron)实现
    virtual int GetEdgePointIds(const int edgeId, const igIndex*& ptIds) = 0;
    virtual int GetFacePointIds(const int faceId, const igIndex*& ptIds) = 0;

protected:
    LagrangeVolume() : m_Order(0) {}
    ~LagrangeVolume() override = default;

    int m_Order;
};

IGAME_NAMESPACE_END
#endif