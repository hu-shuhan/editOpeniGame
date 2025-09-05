#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <vector>
#include <iGameUnstructuredMesh.h>
#include <iGamePoints.h>
IGAME_NAMESPACE_BEGIN
class iGameGetClosestPointsInLine : public Filter {
public:
    I_OBJECT(iGameGetClosestPointsInLine);
    static Pointer New(const Point& startPoint, const endPoint, double radius) {
        return new iGameGetClosestPointsInLine(startPoint, endPoint, radius);
    }
    bool Execute() override;
    const std::vector<int>& GetResult();

private:
    void RUN();

protected:
    iGameGetClosestPointsInLine(const Point& startPoint, const endPoint, double radius);
    ~iGameGetClosestPointsInLine() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    Point m_StartPoint, m_EndPoint;
    double m_Radius{};

private:
    /* Output */
    std::vector<int> m_Ids;
};
IGAME_NAMESPACE_END