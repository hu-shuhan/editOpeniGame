#pragma once

#include <array>

#include <Geom/CBSplineSurface.h>

#include "iGameGPPatch.h"
#include "Util/iGameGP_Macros.h"
#include "Util/iGameGPSplineDefine.h"

#include "iGameGPSurfaceControlPoints.h"
#include <assert.h>
GPSTART

class GPSplinePatchSurface
{

public:

	void init_surface(CBSplineSurface& surface);

	void init_knot_vector(CBSplineSurface& surface);

	void init_cpoints(CBSplineSurface& surface);

	void init_scalar(CBSplineSurface& surface);


	void init_patches();

	void update_patch_data_arr();

	void release();

	void release_knot_vector();

	void release_cpoints();

	void build_tessellation(size_t  new_p, size_t new_q, bool bUsingTCEC = true);

	void set_tessellation_factor(size_t  new_p, size_t new_q);

	void run_evaluation();

	inline std::pair<size_t, size_t> get_p_q() const {

		assert(cur_p != INVALID64);
		assert(cur_q != INVALID64);

		return { cur_p, cur_q };
	}
	inline std::pair<size_t, size_t> get_max_p_q() const { return { MAX_P, MAX_Q }; }

	inline bool is_valid(size_t gp, size_t gq) const {
		return gp < cur_p && gq < cur_q;
	}

	inline size_t get_local_index(size_t lp, size_t lq)
	{
		assert(cur_p != INVALID64);
		assert(cur_q != INVALID64);

		assert(lp + lq * cur_p < cur_q * cur_p);
		assert(lp + lq * MAX_P < MAX_P * MAX_Q);

		return lp + lq * MAX_P;
	}

	inline GPPatch& get_local_patch(size_t lp, size_t lq)
	{

		size_t local_id = get_local_index(lp, lq);
		return get_local_patch(local_id);
	}

	inline GPPatch& get_local_patch(size_t local_id)
	{
		return patches[local_id];
	}

	inline GPPatch& get_global_patch(size_t gp, size_t gq)
	{
		assert(gp + gq * MAX_P < MAX_P * MAX_Q);

		return patches[gp + gq * MAX_P];
	}

	inline GPSurfaceControlPoint get_device_surface_control_points()
	{
		assert(m_device_cpoints_x);
		assert(m_device_cpoints_y);
		assert(m_device_cpoints_z);

		return { m_device_cpoints_x, m_device_cpoints_y, m_device_cpoints_z };
	}

	inline GPSurfaceControlPoint get_device_surface_scalar_points()
	{
		assert(m_device_scalar_cpoints_x);
		assert(m_device_scalar_cpoints_y);
		assert(m_device_scalar_cpoints_z);

		return { m_device_scalar_cpoints_x, m_device_scalar_cpoints_y, m_device_scalar_cpoints_z };
	}

	inline GPSurfaceControlPoint get_host_surface_control_points()
	{
		assert(m_host_cpoints_x);
		assert(m_host_cpoints_y);
		assert(m_host_cpoints_z);

		return { m_host_cpoints_x, m_host_cpoints_y, m_host_cpoints_z };
	}

	inline GPSurfaceControlPoint get_device_surface_control_points_extended()
	{
		assert(m_device_cpoints_x_extended);
		assert(m_device_cpoints_y_extended);
		assert(m_device_cpoints_z_extended);

		return { m_device_cpoints_x_extended, m_device_cpoints_y_extended, m_device_cpoints_z_extended };
	}

	inline GPSurfaceControlPoint get_device_surface_scalar_points_extended()
	{
		assert(m_device_scalar_cpoints_x_extended);
		assert(m_device_scalar_cpoints_y_extended);
		assert(m_device_scalar_cpoints_z_extended);

		return { m_device_scalar_cpoints_x_extended, m_device_scalar_cpoints_y_extended, m_device_scalar_cpoints_z_extended };
	}

	inline GPPatchData* get_device_patch_data_arr()
	{
		return m_device_patch_data_arr;
	}

	bool bCompactCUDAMapBuffer = true;

	bool m_bCCW = true;

	size_t m_offset;
	real_t* m_host_positions_ptr = nullptr;
	real_t* m_host_normal_ptr = nullptr;
	real_t* m_host_scalar_ptr = nullptr;
    void* m_device_patch_blob_pool = nullptr;
    size_t m_device_patch_blob_stride = 0;
    size_t m_device_patch_blob_total_bytes = 0;

private:

	static const size_t u_degree = 3;
	static const size_t v_degree = 3;

	static const size_t u_size = 8;
	static const size_t v_size = 8;

	static const size_t MAX_P = _M_MAX_P;
	static const size_t MAX_Q = _M_MAX_Q;

	size_t cur_p = MAX_P;
	size_t cur_q = MAX_Q;

	real_t* device_u_knots = nullptr;
	real_t* device_v_knots = nullptr;

	real_t* host_u_knots = nullptr;
	real_t* host_v_knots = nullptr;

	real_t* m_device_cpoints_x = nullptr;
	real_t* m_device_cpoints_y = nullptr;
	real_t* m_device_cpoints_z = nullptr;

	real_t* m_host_cpoints_x = nullptr;
	real_t* m_host_cpoints_y = nullptr;
	real_t* m_host_cpoints_z = nullptr;

	real_t* m_device_cpoints_x_extended = nullptr;
	real_t* m_device_cpoints_y_extended = nullptr;
	real_t* m_device_cpoints_z_extended = nullptr;

	real_t* m_host_cpoints_x_extended = nullptr;
	real_t* m_host_cpoints_y_extended = nullptr;
	real_t* m_host_cpoints_z_extended = nullptr;


	real_t* m_device_scalar_cpoints_x = nullptr;
	real_t* m_device_scalar_cpoints_y = nullptr;
	real_t* m_device_scalar_cpoints_z = nullptr;

	real_t* m_host_scalar_cpoints_x = nullptr;
	real_t* m_host_scalar_cpoints_y = nullptr;
	real_t* m_host_scalar_cpoints_z = nullptr;

	real_t* m_device_scalar_cpoints_x_extended = nullptr;
	real_t* m_device_scalar_cpoints_y_extended = nullptr;
	real_t* m_device_scalar_cpoints_z_extended = nullptr;

	real_t* m_host_scalar_cpoints_x_extended = nullptr;
	real_t* m_host_scalar_cpoints_y_extended = nullptr;
	real_t* m_host_scalar_cpoints_z_extended = nullptr;

	std::array<GPPatch, MAX_P* MAX_Q> patches;

	std::array<GPPatchData, MAX_P* MAX_Q>m_host_mapping_patch_data_arr;
	GPPatchData* m_device_patch_data_arr;
};

GPEND
