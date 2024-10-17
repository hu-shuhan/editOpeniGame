#pragma once

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolume.h"
#include "iGameSurfaceMesh.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
IGAME_NAMESPACE_BEGIN

namespace IsolineExtraction
{

    // 三维中的点
    struct Point_3D {
        float x;
        float y;
        float z;
        float value;
        Point_3D() : x(0), y(0), z(0), value(0) {}
        Point_3D(float x, float y, float z, float value = 0)
            : x(x), y(y), z(z), value(value) {}
    };
    // 三维的三角形
    struct Triangle_3D {
        Point_3D p[3];
        Triangle_3D() {
            for (int i = 0; i < 3; i++) { p[i] = Point_3D(); }
        }
    };

    Point_3D VertexInterp(Point_3D p1, Point_3D p2, float iso_val);


    float distance(const Point_3D& p1, const Point_3D& p2);

}// namespace IsolineExtraction


class iGameIsolineExtraction : public Filter {

public:
    I_OBJECT(iGameIsolineExtraction);

    static Pointer New() { return new iGameIsolineExtraction; }
    ~iGameIsolineExtraction() {};
    bool Execute() override;
    void SetAttribute(AttributeSet::Attribute& attr, int dimension = -1) {
        this->m_Attribute = attr;
        this->m_Scalar = attr.pointer;
        this->m_Dimension = dimension;
    }
    void SetValue(float v) { this->m_Value = v; }

protected:
    iGameIsolineExtraction() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    };
    DataObject::Pointer m_Mesh{ nullptr };
    SurfaceMesh::Pointer m_SurfaceMesh{ nullptr };
    UnstructuredMesh::Pointer m_UnstructuredMesh{ nullptr };
    AttributeSet::Attribute m_Attribute{ nullptr };
    ArrayObject::Pointer m_Scalar{ nullptr };
    int m_Dimension = -1;
    float m_Value = 0.0;
};
IGAME_NAMESPACE_END
