#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGamePoints.h>
#include <iGameUnstructuredMesh.h>
#include <vector>
IGAME_NAMESPACE_BEGIN
class iGameGetClosestCellsInLine : public Filter {
public:
    I_OBJECT(iGameGetClosestCellsInLine);
    static Pointer New(const Point& startPoint, const endPoint, double radius) {
        return new iGameGetClosestCellsInLine(startPoint, endPoint, radius);
    }
    bool Execute() override;
    const std::vector<int>& GetResult();

private:
    void RUN();

protected:
    iGameGetClosestCellsInLine(const Point& startPoint, const endPoint, double radius);
    ~iGameGetClosestCellsInLine() override = default;

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