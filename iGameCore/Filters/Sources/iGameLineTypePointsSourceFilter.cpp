//
// Created by m_ky on 2024/8/7.
//

/**
 * @class   iGameLineTypePointsSource
 * @brief   iGameLineTypePointsSource's brief
 */

#include "iGameLineTypePointsSourceFilter.h"
IGAME_NAMESPACE_BEGIN

bool LineTypePointsSourceFilter::Execute() {
    if (this->GetInput(0) != nullptr) {
        UnstructuredMesh::Pointer ps = DynamicCast<UnstructuredMesh>(this->GetInput(0));
        if (ps == nullptr) return false;
        SetPoint_0(ps->GetPoint(0));
        SetPoint_1(ps->GetPoint(1));
    }
    uint32_t totalNum = m_OutPut_PointSet->GetNumberOfPoints();
    int cellId = totalNum;
    int cell[1] = {};
    for (size_t i = 0; i <= m_Resolution; i++) {
        float resolution = (float) (i) / (float) m_Resolution;
        Point newPoint = resolution * m_Point_0 + m_Point_1 * (1.f - resolution);
        if (i >= totalNum) {
            cell[0] = cellId++;
            m_OutPut_PointSet->AddPoint(newPoint);
            m_OutPut_PointSet->AddCell(cell, 1, IG_VERTEX);
        } else {
            m_OutPut_PointSet->SetPoint(i, newPoint);
        }
    }
    m_OutPut_PointSet->Modified();
    return true;
}

void LineTypePointsSourceFilter::SetPoint_0(const Point& point0) { m_Point_0 = point0; }

void LineTypePointsSourceFilter::SetPoint_1(const Point& point1) { m_Point_1 = point1; }

void LineTypePointsSourceFilter::SetResolution(unsigned int Resolution) {
    if (Resolution == 0) m_Resolution = 1;
    m_Resolution = Resolution;
}


IGAME_NAMESPACE_END
