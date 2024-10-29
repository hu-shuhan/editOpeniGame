#ifndef iGamePointPicker_h
#define iGamePointPicker_h

#include "iGamePicker.h"
#include "iGamePoints.h"
#include "iGameLine.h"
#include "iGamePointSet.h"

IGAME_NAMESPACE_BEGIN
class PointPicker : public Filter {
public:
	I_OBJECT(PointPicker);
	static Pointer New() { return new PointPicker; }

	void SetDataObject(DataObject::Pointer dataObject) {
        m_DataObject = dataObject;
		if (m_DataObject->HasSubDataObject()) {
			is_MultiBlock = true;
            for (int i = 0; i < dataObject->GetNumberOfSubDataObjects(); i++) {
                auto block = dataObject->GetSubDataObject(i);
				if (DynamicCast<PointSet>(block)) {
                    m_Points.push_back(
                            DynamicCast<PointSet>(block)->GetPoints());
				}
            }
			if (DynamicCast<PointSet>(dataObject)) {
                m_Points.push_back(
                        DynamicCast<PointSet>(dataObject)->GetPoints());
			}
        } else {
            is_MultiBlock = false;
            if (DynamicCast<PointSet>(dataObject)) {
                m_Points.push_back(
                        DynamicCast<PointSet>(dataObject)->GetPoints());
            }
		}

		auto& bbox = m_DataObject->GetBoundingBox();
		float len = (bbox.max - bbox.min).length();
        m_PickRadius = len * 0.005;
	}

	void SetPoints(Points::Pointer points)
	{
        m_Points.push_back(points);
		BoundingBox bbox;
        for (int i = 0; i < points->GetNumberOfPoints(); i++) {
            auto& point = points->GetPoint(i);
			bbox.add(point);
		}
		float len = (bbox.max - bbox.min).length();
		m_PickRadius = len * 0.005;
	}

	void SetPickRadius(double radius) {
		m_PickRadius = radius;
	}

	igIndex PickClosetPointOnLine(const Vector3d& startPoint, const Vector3d& lineDir, Point& p) {
		if (m_Points.size() == 0)
		{
			return (-1);
		}

		igIndex closeId = -1;
        double minDist = std::numeric_limits<double>::max();
        double zDist = std::numeric_limits<double>::max();

		for (int i = 0; i < m_Points.size(); i++) {
            auto& points = m_Points[i];
            for (int j = 0; j < points->GetNumberOfPoints(); j++) {
                auto& point = points->GetPoint(j);
                double dist =
                        Line::ComputePointToLineDis(startPoint, lineDir, point);
                double dist2 = (point - startPoint).length();

                if (dist < minDist && dist < m_PickRadius && dist2 < zDist) {
                    closeId = j;
                    minDist = dist;
                    zDist = dist2;
                    p = point;
                }
            }
		}
		

		return closeId;
	}

protected:
	PointPicker() {}
	~PointPicker() override {}

	bool is_MultiBlock{false};
	std::vector<Points::Pointer> m_Points{};
    DataObject::Pointer m_DataObject{};
	double m_PickRadius{ 0.01 };
};
IGAME_NAMESPACE_END
#endif