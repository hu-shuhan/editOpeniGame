#pragma once

#include <cuda_runtime_api.h>
#include <algorithm>
#include <cmath>

#include "iGameGPSurfaceControlPoints.h"
#include "iGameGPTessellationFactor.h"
#include <thrust/reduce.h>
#include <thrust/extrema.h>
#include <thrust/execution_policy.h>


__global__ void TessellationFactorEstimationKernel(gpmesh::GPSurfaceControlPoint* device_surface_control_points_arr, uint32_t num_surface,
                                             gpmesh::GPTessellationFactor tessellation_factor,
                                             float* delta_u_arr,
                                             float* delta_v_arr
)
{
    uint32_t surfaceId = blockIdx.x;
    uint32_t cpointId = threadIdx.x;

    if(surfaceId >= num_surface)
    {
        return;
    }

    if(cpointId >= 16)
    {
        return;
    }

    gpmesh::GPSurfaceControlPoint surface_control_point = device_surface_control_points_arr[surfaceId];

    uint32_t row = cpointId % 4;
    uint32_t col = cpointId / 4;

    auto get_index = [ROW=4,COL=4](uint32_t i, uint32_t j) -> uint32_t
    {
        uint32_t index = i * COL + j;
        assert(index < 16 && index >= 0);
        return index;
    };

    __shared__ float cpoints_x[16];
    __shared__ float cpoints_y[16];
    __shared__ float cpoints_z[16];

    float x = surface_control_point.m_cpoints_x[cpointId];
    float y = surface_control_point.m_cpoints_y[cpointId];
    float z = surface_control_point.m_cpoints_z[cpointId];

    float* view_matrix = tessellation_factor.ViewMatrix;

    cpoints_x[cpointId] = x * view_matrix[0] + y * view_matrix[1] + z * view_matrix[2] + view_matrix[3];
    cpoints_y[cpointId] = x * view_matrix[4] + y * view_matrix[5] + z * view_matrix[6] + view_matrix[7];
    cpoints_z[cpointId] = x * view_matrix[8] + y * view_matrix[9] + z * view_matrix[10] + view_matrix[11];

    __syncthreads();

    float* min_x = thrust::min_element(thrust::device, cpoints_x, cpoints_x + 16);
    float* min_y = thrust::min_element(thrust::device, cpoints_y, cpoints_y + 16);
    float* min_z = thrust::min_element(thrust::device, cpoints_z, cpoints_z + 16);

    __shared__ float epsilon;

    if(threadIdx.x == 0)
    {
        epsilon = 9999;

        float z_abs = std::max<float>(0.01f, std::abs(*min_z));
        float x_over_z_abs = std::abs(*min_x / z_abs);
        float y_over_z_abs = std::abs(*min_y / z_abs);

        bool bCheck = false;
        if(surfaceId == 0 && bCheck)
        {
            printf("GPU Surface-%u : z_abs=%f \n", surfaceId, z_abs);
            printf("GPU Surface-%u : x_over_z_abs=%f \n", surfaceId, x_over_z_abs);
            printf("GPU Surface-%u : y_over_z_abs=%f \n", surfaceId, y_over_z_abs);
        }


        epsilon = std::min<float>(epsilon,  z_abs * tessellation_factor.tauX / (tessellation_factor.A*(1 + x_over_z_abs) + tessellation_factor.tauX));
        epsilon = std::min<float>(epsilon, z_abs * tessellation_factor.tauY / (tessellation_factor.B*(1 + y_over_z_abs) + tessellation_factor.tauY));

        if(surfaceId == 0 && bCheck)
        {
            printf("GPU Surface-%u : epsilon=%f \n", surfaceId, epsilon);
        }
    }

    __shared__ float r[16];

    __shared__ float Duu, Duv, Dvv;

    int n = 3, m = 3;

    if(cpointId < 8)
    {
        uint32_t i = cpointId / 4;
        uint32_t j = cpointId % 4;

        x = cpoints_x[get_index(i+2,j)] - 2 * cpoints_x[get_index(i+1,j)] + cpoints_x[get_index(i,j)];
        y = cpoints_y[get_index(i+2,j)] - 2 * cpoints_y[get_index(i+1,j)] + cpoints_y[get_index(i,j)];
        z = cpoints_z[get_index(i+2,j)] - 2 * cpoints_z[get_index(i+1,j)] + cpoints_z[get_index(i,j)];

        r[cpointId] = std::sqrt(x * x + y * y + z * z);
    }

    __syncthreads();

    float* data = thrust::max_element(thrust::device, r, r + 8);

    if(threadIdx.x == 0){
        Duu = (*data);
    }

    if(cpointId < 9)
    {
        uint32_t i = cpointId / 3;
        uint32_t j = cpointId % 3;

        x = cpoints_x[get_index(i+1,j+1)] - cpoints_x[get_index(i+1,j)] - cpoints_x[get_index(i,j+1)] + cpoints_x[get_index(i,j)];
        y = cpoints_y[get_index(i+1,j+1)] - cpoints_y[get_index(i+1,j)] - cpoints_y[get_index(i,j+1)] + cpoints_y[get_index(i,j)];
        z = cpoints_z[get_index(i+1,j+1)] - cpoints_z[get_index(i+1,j)] - cpoints_z[get_index(i,j+1)] + cpoints_z[get_index(i,j)];

        r[cpointId] = std::sqrt(x * x + y * y + z * z);
    }

    __syncthreads();

    data = thrust::max_element(thrust::device, r, r + 9);

    if(threadIdx.x == 0) {
        Duv = (*data);
    }


    if(cpointId < 8)
    {
        uint32_t i = cpointId % 4;
        uint32_t j = cpointId / 4;

        x = cpoints_x[get_index(i,j+2)] - 2 * cpoints_x[get_index(i,j+1)] + cpoints_x[get_index(i,j)];
        y = cpoints_y[get_index(i,j+2)] - 2 * cpoints_y[get_index(i,j+1)] + cpoints_y[get_index(i,j)];
        z = cpoints_z[get_index(i,j+2)] - 2 * cpoints_z[get_index(i,j+1)] + cpoints_z[get_index(i,j)];

        r[cpointId] = std::sqrt(x * x + y * y + z * z);
    }

    __syncthreads();

    data = thrust::max_element(thrust::device, r, r + 8);

    if(threadIdx.x == 0) {
        Dvv = (*data);
    }

    if(threadIdx.x == 0)
    {
        float DuuDvv_sqrt = std::sqrt(Duu * Dvv);

        float deltav = std::sqrt(4 * Dvv * epsilon / (Duu * Dvv + Duv * DuuDvv_sqrt));
        float deltau = std::sqrt(4 * Duu * epsilon / (Duu * Dvv + Duv * DuuDvv_sqrt));

        bool bCheck = false;
        if(surfaceId == 0 && bCheck)
        {
            printf("GPU Surface-%u : Duu=%f \n", surfaceId, Duu);
            printf("GPU Surface-%u : Duv=%f \n", surfaceId, Duv);
            printf("GPU Surface-%u : Dvv=%f \n", surfaceId, Dvv);
            printf("GPU Surface-%u : DuuDvv_sqrt=%f \n", surfaceId, DuuDvv_sqrt);
            printf("GPU Surface-%u : epsilon=%f \n", surfaceId, epsilon);
            printf("GPU Surface-%u : deltau=%f \n", surfaceId, deltau);
            printf("GPU Surface-%u : deltav=%f \n", surfaceId, deltav);
        }

        delta_u_arr[surfaceId] = deltau;
        delta_v_arr[surfaceId] = deltav;
    }
}


template<size_t MAX_P, size_t MAX_Q>
__global__ void calculate_p_and_q(float* delta_u_arr,
                                  float* delta_v_arr,
                                  size_t* p_arr,
                                  size_t* q_arr,
                                  uint32_t num_surface)
{
    size_t id = blockIdx.x * blockDim.x + threadIdx.x;

    if(id >= num_surface)
    {
        return;
    }

    gpmesh::real_t delta_u = delta_u_arr[id];
    gpmesh::real_t delta_v = delta_v_arr[id];

    uint32_t tu = std::llround(std::ceil(static_cast<gpmesh::real_t>(1.0) / delta_u));
    uint32_t tv = std::llround(std::ceil(static_cast<gpmesh::real_t>(1.0) / delta_v));

    uint32_t cur_p = 1;
    uint32_t cur_q = 1;

    if(tu > 1)
    {
        cur_p = (tu - 1) / 7;

        if((tu - 1) % 7 != 0)
        {
            cur_p += 1;
        }
    }

    if(tv > 1)
    {
        cur_q = (tv - 1) / 7;

        if((tv - 1) % 7 != 0)
        {
            cur_q += 1;
        }
    }

    cur_p = std::min<uint32_t>(cur_p, MAX_P);
    cur_p = std::max<uint32_t>(cur_p, 1);

    cur_q = std::min<uint32_t>(cur_q, MAX_Q);
    cur_q = std::max<uint32_t>(cur_q, 1);

    p_arr[id] = cur_p;
    q_arr[id] = cur_q;
}