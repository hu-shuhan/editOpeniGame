#include "iGameGetCellsInFrustum.h"
#include <iGameCell.h>
IGAME_NAMESPACE_BEGIN
static bool IsPointInFrustum(const Point& point, const Point& startPoint, const Point& direction,
                             const Point& upDirection, double nearFaceDistance, double farFaceDistance,
                             double nearFaceHalfWidth, double nearFaceHalfHigh, double farFaceHalfWidth,
                             double farFaceHalfHigh) {
    // Normalize the direction vector to get the forward axis
    Point forward = direction.normalized();

    // Calculate the right axis: cross product of direction and upDirection, then normalize
    Point right = direction.cross(upDirection).normalized();

    // Recalculate the up axis to ensure orthogonality: cross product of right and forward
    Point up = right.cross(forward).normalized();

    // Vector from startPoint to the point
    Point vec = point - startPoint;

    // Project the vector onto the forward axis to get the distance along the ray
    double dist = vec.dot(forward);

    // Check if the point is between the near and far planes
    double minDist = std::min(nearFaceDistance, farFaceDistance);
    double maxDist = std::max(nearFaceDistance, farFaceDistance);
    if (dist < minDist || dist > maxDist) { return false; }

    // Calculate the current half width and half height at the point's distance using linear interpolation
    double currentHalfWidth, currentHalfHigh;
    if (farFaceDistance == nearFaceDistance) {
        currentHalfWidth = nearFaceHalfWidth;
        currentHalfHigh = nearFaceHalfHigh;
    } else {
        double t = (dist - nearFaceDistance) / (farFaceDistance - nearFaceDistance);
        currentHalfWidth = nearFaceHalfWidth + t * (farFaceHalfWidth - nearFaceHalfWidth);
        currentHalfHigh = nearFaceHalfHigh + t * (farFaceHalfHigh - nearFaceHalfHigh);
    }

    // Project the vector onto the right and up axes
    double rightCoord = vec.dot(right);
    double upCoord = vec.dot(up);

    // Check if the point is within the current rectangle
    if (std::abs(rightCoord) <= currentHalfWidth && std::abs(upCoord) <= currentHalfHigh) { return true; }

    return false;
}

static bool IsCellInFrustum(Cell* cell, const Point& startPoint, const Point& direction, const Point& upDirection,
                            double nearFaceDistance, double farFaceDistance, double nearFaceHalfWidth,
                            double nearFaceHalfHigh, double farFaceHalfWidth, double farFaceHalfHigh) {
    auto faceNum = cell->GetNumberOfFaces();
    if (faceNum == 0) {
        int pointSize = cell->GetNumberOfPoints();
        for (int i = 0; i < pointSize; i++) {
            auto& point = cell->GetPoint(i);
            bool pointInFrustumResult =
                    IsPointInFrustum(point, startPoint, direction, upDirection, nearFaceDistance, farFaceDistance,
                                     nearFaceHalfWidth, nearFaceHalfHigh, farFaceHalfWidth, farFaceHalfHigh);
            if (!pointInFrustumResult) return false;
        }
    } else {
        for (int faceIndex = 0; faceIndex < faceNum; faceIndex++) {
            auto face = cell->GetFace(faceIndex);
            bool cellInFrustumResult =
                    IsCellInFrustum(face, startPoint, direction, upDirection, nearFaceDistance, farFaceDistance,
                                    nearFaceHalfWidth, nearFaceHalfHigh, farFaceHalfWidth, farFaceHalfHigh);
            if (!cellInFrustumResult) return false;
        }
    }
    return true;
}

bool GetCellsInFrustumFilter::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    if (m_NearFaceHalfWidth <= 0 || m_NearFaceHalfHigh <= 0 || m_FarFaceHalfWidth <= 0 || m_FarFaceHalfHigh <= 0) {
        return false;
    }
    Run();
    return true;
}

const std::vector<int>& GetCellsInFrustumFilter::GetResult() { return m_Ids; }

void GetCellsInFrustumFilter::Run() {
    int objNum = m_Mesh->GetNumberOfCells();
    for (int cellId = 0; cellId < objNum; cellId++) {
        auto cell = m_Mesh->GetCell(cellId);
        if (IsCellInFrustum(cell, m_StartPoint, m_Direction, m_UpDirection, m_NearFaceDistance, m_FarFaceDistance,
                            m_NearFaceHalfWidth, m_NearFaceHalfHigh, m_FarFaceHalfWidth, m_FarFaceHalfHigh)) {
            m_Ids.push_back(cellId);
        }
    }
}

GetCellsInFrustumFilter::GetCellsInFrustumFilter(const Point& startPoint, const Point& direction,
                                               const Point& upDirection, double nearFaceDistance,
                                               double farFaceDistance, double nearFaceHalfWidth,
                                               double nearFaceHalfHigh, double farFaceHalfWidth,
                                               double farFaceHalfHigh) {
    m_StartPoint = startPoint;
    m_Direction = direction;
    m_UpDirection = upDirection;
    m_NearFaceDistance = nearFaceDistance;
    m_FarFaceDistance = farFaceDistance;
    m_NearFaceHalfWidth = nearFaceHalfWidth;
    m_NearFaceHalfHigh = nearFaceHalfHigh;
    m_FarFaceHalfWidth = farFaceHalfWidth;
    m_FarFaceHalfHigh = farFaceHalfHigh;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(0);
}
IGAME_NAMESPACE_END