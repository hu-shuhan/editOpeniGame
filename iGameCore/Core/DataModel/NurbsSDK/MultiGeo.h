#pragma once

#include "Curve.h"
#include "Surface.h"
#include "Volume.h"
#include "string"
#include <memory>

IGAME_NAMESPACE_BEGIN
IGAME_NURBSSDK_NAMESPACE_BEGIN
using Geometry = std::shared_ptr<Geo>;

enum class Type { CURVE, SURFACE, VOLUME };

class MultiGeo : public Object {
public:
    I_OBJECT(MultiGeo)
    static Pointer New() { return new MultiGeo; }

public:
    void AddPatch(Geometry& geo) {
        m_Geometry.push_back(geo);
        this->Modified();
    }

    Geo& Patch(unsigned int i) { return *m_Geometry[i]; }

    Geometry PatchPointer(unsigned int i) { return m_Geometry[i]; }

    void SetBoundaryInfo(const std::vector<std::array<int, 2>>& boundary) {
        m_Boundary = boundary;
        this->Modified();
    }

    std::vector<std::array<int, 2>>& GetBoundaryInfo() { return m_Boundary; }

    int GetPatchSize() { return m_Geometry.size(); }

    void SetType(Type type) { m_NurbsType = type; }
    Type GetType() { return m_NurbsType; }

protected:
    MultiGeo() {}
    MultiGeo(std::vector<Geometry>& geometry) {
        m_Geometry = geometry;
        this->Modified();
    }
    ~MultiGeo();

    Type m_NurbsType;
    std::vector<Geometry> m_Geometry;

    // 边界信息  {片号，边界号}
    // 曲线   0：参数点u = 0的边界   1：参数点u = 1的边界
    // 曲面   0：参数点u = 0的边界   1：参数点u = 1的边界   2：参数点v = 0的边界    3：参数点v = 1的边界
    // 曲体   0：参数点u = 0的边界   1：参数点u = 1的边界   2：参数点v = 0的边界    3：参数点v = 1的边界    4：参数点w = 0的边界    5：参数点w = 1的边界
    std::vector<std::array<int, 2>> m_Boundary;
};
IGAME_NURBSSDK_NAMESPACE_END
IGAME_NAMESPACE_END