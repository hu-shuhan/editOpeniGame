#ifndef iGameQuadric_h
#define iGameQuadric_h

#include "iGameVector.h"

#include <Eigen/Core>
#include <Eigen/Dense>

#include <cmath>

IGAME_NAMESPACE_BEGIN
template<class ValueT>
class Quadric {
public:
	ValueT a[6];
	ValueT b[3];
	ValueT c;

	void setZero()
	{
		a[0] = 0;
		a[1] = 0;
		a[2] = 0;
		a[3] = 0;
		a[4] = 0;
		a[5] = 0;
		b[0] = 0;
		b[1] = 0;
		b[2] = 0;
		c = 0;
	}

	void operator=(const Quadric<ValueT>& q)
	{
		a[0] = q.a[0];
		a[1] = q.a[1];
		a[2] = q.a[2];
		a[3] = q.a[3];
		a[4] = q.a[4];
		a[5] = q.a[5];
		b[0] = q.b[0];
		b[1] = q.b[1];
		b[2] = q.b[2];
		c = q.c;
	}

	void operator+=(const Quadric<ValueT>& q)
	{
		a[0] += q.a[0];
		a[1] += q.a[1];
		a[2] += q.a[2];
		a[3] += q.a[3];
		a[4] += q.a[4];
		a[5] += q.a[5];
		b[0] += q.b[0];
		b[1] += q.b[1];
		b[2] += q.b[2];
		c += q.c;
	}

	void operator*=(const double& w)
	{
		a[0] *= w;
		a[1] *= w;
		a[2] *= w;
		a[3] *= w;
		a[4] *= w;
		a[5] *= w;
		b[0] *= w;
		b[1] *= w;
		b[2] *= w;
		c *= w;
	}

	template<class _Ty>
	void byPlane(const _Ty& p)
	{
		a[0] = p[0] * p[0]; // a11
		a[1] = p[1] * p[0];	// a12 (=a21)
		a[2] = p[2] * p[0];	// a13 (=a31)
		a[3] = p[1] * p[1];	// a22
		a[4] = p[2] * p[1];	// a23 (=a32)
		a[5] = p[2] * p[2];	// a33
		b[0] = p[3] * p[0];
		b[1] = p[3] * p[1];
		b[2] = p[3] * p[2];
		c = p[3] * p[3];
	}

	template<class _Ty>
	void byPlane(const _Ty& p, double d)
	{
		a[0] = p[0] * p[0]; // a11
		a[1] = p[1] * p[0];	// a12 (=a21)
		a[2] = p[2] * p[0];	// a13 (=a31)
		a[3] = p[1] * p[1];	// a22
		a[4] = p[2] * p[1];	// a23 (=a32)
		a[5] = p[2] * p[2];	// a33
		b[0] = d * p[0];    // a14 (=a41)
		b[1] = d * p[1];    // a24 (=a42)
		b[2] = d * p[2];    // a34 (=a43)
		c = d * d;          // a44
	}

	template<class _Ty>
	double apply(const _Ty& p) const
	{
		return p[0] * p[0] * a[0] + 2 * p[0] * p[1] * a[1] + 2 * p[0] * p[2] * a[2] + p[0] * (2 * b[0])
			+ p[1] * p[1] * a[3] + 2 * p[1] * p[2] * a[4] + p[1] * (2 * b[1])
			+ p[2] * p[2] * a[5] + p[2] * (2 * b[2]) + c;
	}

	template<class _Ty>
	bool findMinimum(_Ty& x)
	{
		Eigen::Matrix3d A;
		Eigen::Vector3d be;
		A << a[0], a[1], a[2],
			a[1], a[3], a[4],
			a[2], a[4], a[5];
		be << -b[0], -b[1], -b[2];

		Eigen::FullPivLU<Eigen::Matrix3d> lu(A);
		// A nearly singular quadric may still be reported as invertible, but its
		// unconstrained minimum can be many orders of magnitude away from the
		// mesh. Reject poorly conditioned systems and let the caller fall back to
		// a position on the edge.
		constexpr double MinimumReciprocalCondition = 1e-12;
		if (lu.isInvertible() && lu.rcond() >= MinimumReciprocalCondition) {
			Eigen::Vector3d xe = lu.solve(be);
			if (!xe.allFinite()) { return false; }
			x[0] = xe[0];
			x[1] = xe[1];
			x[2] = xe[2];
			return true;
		}
		return false;
	}

	Eigen::Matrix4d mat() const {
		Eigen::Matrix4d A;
		A << a[0], a[1], a[2], b[0],
			a[1], a[3], a[4], b[1],
			a[2], a[4], a[5], b[2],
			b[0], b[1], b[2], c;
		return A;
	}
};

IGAME_NAMESPACE_END
#endif
