#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <vector>
#include <iGameUnstructuredMesh.h>
#include <iGamePoints.h>
IGAME_NAMESPACE_BEGIN
class iGameGetPointsInFrustum : public Filter {
public:
    I_OBJECT(iGameGetPointsInFrustum);
    static Pointer New(const Point& startPoint, const Point& direction, const Point& upDirection,
                       double nearFaceDistance, double farFaceDistance, double nearFaceHalfWidth,
                       double nearFaceHalfHigh, double farFaceHalfWidth, double farFaceHalfHigh) {
        return new iGameGetPointsInFrustum(startPoint, direction, upDirection, nearFaceDistance, farFaceDistance,
                                           nearFaceHalfWidth, nearFaceHalfHigh, farFaceHalfWidth, farFaceHalfHigh);
    }
    bool Execute() override;
    const std::vector<int>& GetResult();

private:
    void Run();

protected:
    iGameGetPointsInFrustum(const Point& startPoint, const Point& direction, const Point& upDirection,
                            double nearFaceDistance, double farFaceDistance, double nearFaceHalfWidth,
                            double nearFaceHalfHigh, double farFaceHalfWidth, double farFaceHalfHigh);
    ~iGameGetPointsInFrustum() override = default;

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