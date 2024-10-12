#ifndef iGameVolumeMeshClipper_h
#define iGameVolumeMeshClipper_h

#include "iGameDataObject.h"
#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"

IGAME_NAMESPACE_BEGIN


class iGameVolumeMeshClipper : public Filter {
public:
	I_OBJECT(iGameVolumeMeshClipper);

	static iGameVolumeMeshClipper* New() {
		return new iGameVolumeMeshClipper;
	};

	void SetExtentClipping(bool _in) { this->m_BoxClipper.m_Use = _in; }
	void SetPlaneClipping(bool _in) { this->m_PlaneClipper.m_Use = _in; }

	void SetExtent(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax, bool flip = false);
	void SetExtent(double boxMin[3], double boxMax[3], bool flip = false);

	void SetPlane(double ox, double oy, double oz, double nx, double ny, double nz, bool flip = false);
	void SetPlane(double orgin[3], double normal[3], bool flip = false);


	bool Execute() override;

protected:
	iGameVolumeMeshClipper();

	void InitializeResultMesh(Points::Pointer points);

	bool ExecuteWithVolumeMesh();
	bool ExecuteWithUnstructedMesh();

	bool PointVisibility(double* position) {
		if (m_BoxClipper.m_Use) {
			const auto& bmin = m_BoxClipper.m_BoxMin;
			const auto& bmax = m_BoxClipper.m_BoxMax;
			if (position[0] >= bmin[0] && position[0] <= bmax[0]
				&& position[1] >= bmin[1] && position[1] <= bmax[1]
				&& position[2] >= bmin[2] && position[2] <= bmax[2]) {
				return !m_BoxClipper.m_Flip;
			}
			else {
				return m_BoxClipper.m_Flip;
			}
		}
		else if (m_PlaneClipper.m_Use) {
            const auto& o = m_PlaneClipper.m_Origin;
            const auto& n = m_PlaneClipper.m_Normal;
			if (((position[0] - o[0]) * n[0] + (position[1] - o[1]) * n[1] + (position[2] - o[2]) * n[2])
				>= 0.) {
				return !m_PlaneClipper.m_Flip;
			}
			else {
				return m_PlaneClipper.m_Flip;
			}
		}
		return true;
	}

	UnsignedCharArray::Pointer ComputeCellsVisibility(Points::Pointer points, CellArray::Pointer cells);
private:
	SurfaceMesh::Pointer m_OuterMeshVisible, m_InterMeshVisible, m_OuterMeshInvisible;

	struct BoxClipper {
		double m_BoxMin[3], m_BoxMax[3];
		bool m_Use{ false };
		bool m_Flip{ false };
	} m_BoxClipper;
	struct PlaneClipper {
		double m_Origin[3], m_Normal[3];
		bool m_Use{ false };
		bool m_Flip{ false };
	} m_PlaneClipper;

};
IGAME_NAMESPACE_END

#endif // !iGameVolumeMeshClipper_h
