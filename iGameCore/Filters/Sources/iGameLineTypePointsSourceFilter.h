/**
 * @class   iGameLineTypePointsSource
 * @brief   iGameLineTypePointsSource's brief
 */

#pragma once

#include "iGamePoints.h"
#include "iGamePointsSourceFilter.h"

IGAME_NAMESPACE_BEGIN
class LineTypePointsSourceFilter : public PointSourceFilter {
public:
    I_OBJECT(LineTypePointsSourceFilter)
    static Pointer New() { return new LineTypePointsSourceFilter(); }

    bool Execute() override;

    void SetPoint_0(const Point& point0);

    void SetPoint_1(const Point& point1);

    void SetResolution(unsigned int Resolution);

protected:
    LineTypePointsSourceFilter() {};

    Point m_Point_0;
    Point m_Point_1;

    uint32_t m_Resolution{1};
};


IGAME_NAMESPACE_END
