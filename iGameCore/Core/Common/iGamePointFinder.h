#ifndef iGamePointFinder_h
#define iGamePointFinder_h

#include "iGameBoundingBox.h"
#include "iGameElementArray.h"
#include "iGameFlexArray.h"
#include "iGameIdArray.h"
#include "iGameObject.h"
#include "iGamePoints.h"

IGAME_NAMESPACE_BEGIN
class PointFinder : public Object {
public:
    I_OBJECT(PointFinder);
    static Pointer New() { return new PointFinder; }

    void SetPoints(Points::Pointer p) { m_Points = p; }

    void Initialize() {
        if (!m_Points) { return; }
        // m_NumberOfPointsPerBox = m_Threshold * m_Points->GetNumberOfPoints();
        m_NumberOfPointsPerBox = std::sqrt(m_Points->GetNumberOfPoints());
        int numBoxes = m_Points->GetNumberOfPoints() / m_NumberOfPointsPerBox;
        m_Depth = std::ceil(std::log10(numBoxes) / std::log10(8));
        m_Depth = std::max(1, std::min(4, m_Depth));
        m_Size = std::pow(2, m_Depth);
        m_SizeSquared = m_Size * m_Size;
        m_NumberOfBoxes = std::pow(8, m_Depth);

        m_Bounding.reset();
        for (int i = 0; i < m_Points->GetNumberOfPoints(); i++) {
            m_Bounding.add(m_Points->GetPoint(i));
        }

        double max = m_Bounding.diagVector().maxCoeff();
        m_Bounding.min -= 0.005 * max;
        m_Bounding.max += 0.005 * max;

        Vector3d diag = m_Bounding.diagVector();
        m_BoxSize = diag / m_Size;
        m_ReciprocalBoxSize[0] = 1.0 / m_BoxSize[0];
        m_ReciprocalBoxSize[1] = 1.0 / m_BoxSize[1];
        m_ReciprocalBoxSize[2] = 1.0 / m_BoxSize[2];

        m_Buffer = ElementArray<IdArray::Pointer>::New();
        m_Buffer->Resize(m_NumberOfBoxes, nullptr);

        for (IGsize i = 0; i < m_Points->GetNumberOfPoints(); i++) {
            auto& x = m_Points->GetPoint(i);
            IGsize index = this->GetBoxIndex(x);
            auto& box = m_Buffer->GetElement(index);
            if (box == nullptr) {
                box = IdArray::New();
                box->Reserve(m_NumberOfPointsPerBox);
                m_Buffer->SetElement(index, box);
            }
            box->AddId(i);
        }
    }

    igIndex FindClosestPoint(const Vector3d& x) {
        int i, j;
        double minDist2 = std::numeric_limits<double>::max();
        double dist2;
        Vector3d p;
        int level = 0;
        igIndex ptId, closest, boxIndex, nids;
        IdArray::Pointer ptIds;
        Vector3i ijk, nei;
        FlexArray<Vector3i> boxes;

        ijk = this->Interize(x);

        for (closest = -1; (closest == -1) && (level < m_Size); level++) {
            this->GetBoxNeighbors(boxes, ijk, level);

            for (i = 0; i < boxes.size(); i++) {
                nei = boxes[i];
                boxIndex = nei[0] + nei[1] * m_Size + nei[2] * m_SizeSquared;

                if ((ptIds = m_Buffer->GetElement(boxIndex)) != nullptr) {
                    nids = ptIds->GetNumberOfIds();
                    for (j = 0; j < nids; j++) {
                        ptId = ptIds->GetId(j);
                        m_Points->GetPoint(ptId, p);
                        if ((dist2 = (x - p).squaredLength()) < minDist2) {
                            closest = ptId;
                            minDist2 = dist2;
                        }
                    }
                }
            }
        }

        // Since the nearest point of this box is not necessarily the global
        // nearest, We need to search for all boxes within the minDist2 distance
        // range.
        if (minDist2 > 0.0) {
            this->GetOverlappingBoxes(boxes, x, ijk, sqrt(minDist2));
            for (i = 0; i < boxes.size(); i++) {
                nei = boxes[i];
                boxIndex = nei[0] + nei[1] * m_Size + nei[2] * m_SizeSquared;

                if ((ptIds = m_Buffer->GetElement(boxIndex)) != nullptr) {
                    nids = ptIds->GetNumberOfIds();
                    for (j = 0; j < nids; j++) {
                        ptId = ptIds->GetId(j);
                        m_Points->GetPoint(ptId, p);
                        if ((dist2 = (x - p).squaredLength()) < minDist2) {
                            closest = ptId;
                            minDist2 = dist2;
                        }
                    }
                }
            }
        }

        return closest;
    }

protected:
    PointFinder() {}
    ~PointFinder() override = default;

    void GetBoxNeighbors(FlexArray<Vector3i>& boxes, const Vector3i ijk,
                         int level) {
        int i, j, k, min, max;
        Vector3i minLevel, maxLevel, nei;

        boxes.reset();

        // If at this box
        if (level == 0) {
            boxes.push_back(ijk);
            return;
        }

        // Find all boxes in the level neighborhood
        for (i = 0; i < 3; i++) {
            min = ijk[i] - level;
            max = ijk[i] + level;
            minLevel[i] = (min > 0 ? min : 0);
            maxLevel[i] = (max < (m_Size - 1) ? max : (m_Size - 1));
        }

        // Only find boxes at the level layer
        for (i = minLevel[0]; i <= maxLevel[0]; i++) {
            for (j = minLevel[1]; j <= maxLevel[1]; j++) {
                for (k = minLevel[2]; k <= maxLevel[2]; k++) {
                    if (i == (ijk[0] + level) || i == (ijk[0] - level) ||
                        j == (ijk[1] + level) || j == (ijk[1] - level) ||
                        k == (ijk[2] + level) || k == (ijk[2] - level)) {
                        nei[0] = i;
                        nei[1] = j;
                        nei[2] = k;
                        boxes.push_back(nei);
                    }
                }
            }
        }
    }

    void GetOverlappingBoxes(FlexArray<Vector3i>& boxes, const Vector3d x,
                             const Vector3i ijk, double dist) {
        int i, j, k;
        Vector3i nei, minLevel, maxLevel;
        Vector3d xMin, xMax;
        xMin = xMax = x;

        boxes.reset();

        xMin -= dist;
        xMax += dist;

        minLevel = this->Interize(xMin);
        maxLevel = this->Interize(xMax);

        for (i = minLevel[0]; i <= maxLevel[0]; i++) {
            for (j = minLevel[1]; j <= maxLevel[1]; j++) {
                for (k = minLevel[2]; k <= maxLevel[2]; k++) {
                    if (i < ijk[0] || i > ijk[0] || j < ijk[1] || j > ijk[1] ||
                        k < ijk[2] || k > ijk[2]) {
                        nei[0] = i;
                        nei[1] = j;
                        nei[2] = k;
                        boxes.push_back(nei);
                    }
                }
            }
        }
    }

    IGsize GetBoxIndex(const Vector3d& x) const {
        Vector3i ijk = Interize(x);
        return static_cast<IGsize>(ijk[0]) +
               static_cast<IGsize>(ijk[1]) * m_Size +
               static_cast<IGsize>(ijk[2]) * m_SizeSquared;
    }

    Vector3i Interize(const Vector3d& x) const {
        // assert(m_Bounding.isIn(x));
        Vector3i pi;

        pi[0] = int((x[0] - m_Bounding.min[0]) * m_ReciprocalBoxSize[0]);
        pi[1] = int((x[1] - m_Bounding.min[1]) * m_ReciprocalBoxSize[1]);
        pi[2] = int((x[2] - m_Bounding.min[2]) * m_ReciprocalBoxSize[2]);

        pi[0] = std::max(0, std::min(m_Size - 1, pi[0]));
        pi[1] = std::max(0, std::min(m_Size - 1, pi[1]));
        pi[2] = std::max(0, std::min(m_Size - 1, pi[2]));

        return pi;
    }

    Vector3d DeInterize(const Vector3i& pi) const {
        Vector3d pf;

        assert(pi[0] >= 0 && pi[0] < m_Size);
        assert(pi[1] >= 0 && pi[1] < m_Size);
        assert(pi[2] >= 0 && pi[2] < m_Size);

        pf[0] = pi[0] * m_BoxSize[0] + m_Bounding.min[0];
        pf[1] = pi[1] * m_BoxSize[1] + m_Bounding.min[1];
        pf[2] = pi[2] * m_BoxSize[2] + m_Bounding.min[2];

        return pf;
    }

    Points::Pointer m_Points{};
    ElementArray<IdArray::Pointer>::Pointer m_Buffer{};
    Vector3d m_BoxSize, m_ReciprocalBoxSize;
    BoundingBox m_Bounding;

    int m_NumberOfBoxes{1};
    int m_NumberOfPointsPerBox{1};
    double m_Threshold{0.01};

    int m_Size{}, m_SizeSquared{}; // m_Size = 2 ^ m_Depth
    int m_Depth{};                 // Tree's depth
};
IGAME_NAMESPACE_END
#endif