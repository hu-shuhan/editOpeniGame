#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGamePoints.h>
#include <iGameUnstructuredMesh.h>
#include <vector>
IGAME_NAMESPACE_BEGIN
class iGameGetCellsInFrustum : public Filter {
public:
    I_OBJECT(iGameGetCellsInFrustum);
    static Pointer New(const Point& startPoint, const Point& direction, const Point& upDirection,
                       double nearFaceDistance, double farFaceDistance, double nearFaceHalfWidth,
                       double nearFaceHalfHigh, double farFaceHalfWidth, double farFaceHalfHigh) {
        return new iGameGetCellsInFrustum(startPoint, direction, upDirection, nearFaceDistance, farFaceDistance,
                                           nearFaceHalfWidth, nearFaceHalfHigh, farFaceHalfWidth, farFaceHalfHigh);
    }
    bool Execute() override;
    const std::vector<int>& GetResult();

private:
    void Run();

protected:
    iGameGetCellsInFrustum(const Point& startPoint, const Point& direction, const Point& upDirection,
                            double nearFaceDistance, double farFaceDistance, double nearFaceHalfWidth,
                            double nearFaceHalfHigh, double farFaceHalfWidth, double farFaceHalfHigh);
    ~iGameGetCellsInFrustum() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    Point m_StartPoint;
    Point m_Direction;
    Point m_UpDirection;
    double m_NearFaceDistance{};
    double m_FarFaceDistance{};
    double m_NearFaceHalfWidth{};
    double m_NearFaceHalfHigh{};
    double m_FarFaceHalfWidth{};
    double m_FarFaceHalfHigh{};

private:
    /* Output */
    std::vector<int> m_Ids;
};
IGAME_NAMESPACE_END