#include "iGameGPPatch.h"
#include "iGameGPSplinePatchSurface.h"
#include "Util/iGameGP_CUDA_Macros.cuh"
#include <chrono>
#include <fstream>
#include <iomanip>
GPSTART

void GPSplinePatchSurface::init_surface(CBSplineSurface& surface) {
    init_knot_vector(surface);
    init_cpoints(surface);
    init_scalar(surface);

    this->m_bCCW = surface.m_bCCW;
}

void GPSplinePatchSurface::init_knot_vector(CBSplineSurface& surface) {
    host_u_knots = new real_t[u_size]{0};

    for (int i = 4; i < 8; ++i) { host_u_knots[i] = 1.0; }

    host_v_knots = new real_t[v_size]{0};

    for (int i = 4; i < 8; ++i) { host_v_knots[i] = 1.0; }

    size_t uDataSize = u_size * sizeof(real_t);
    CUDA_ERROR(cudaMalloc(&device_u_knots, uDataSize));
    CUDA_ERROR(cudaMemcpyAsync(device_u_knots, host_u_knots, uDataSize,
                               cudaMemcpyHostToDevice));

    size_t vDataSize = v_size * sizeof(real_t);
    CUDA_ERROR(cudaMalloc(&device_v_knots, vDataSize));
    CUDA_ERROR(cudaMemcpyAsync(device_v_knots, host_v_knots, vDataSize,
                               cudaMemcpyHostToDevice));
}

void GPSplinePatchSurface::init_cpoints(CBSplineSurface& surface) {

    const int ROW_NUM = 4;
    const int COL_NUM = 4;
    const size_t CPOINT_SIZE = ROW_NUM * COL_NUM;
    m_host_cpoints_x = new real_t[CPOINT_SIZE]{0};
    m_host_cpoints_y = new real_t[CPOINT_SIZE]{0};
    m_host_cpoints_z = new real_t[CPOINT_SIZE]{0};

    const int ROW_EXTEND_NUM = 8;
    const int COL_EXTEND_NUM = 8;
    const size_t CPOINT_SIZE_EXTEND = ROW_EXTEND_NUM * COL_EXTEND_NUM;

    m_host_cpoints_x_extended = new real_t[CPOINT_SIZE_EXTEND]{0};
    m_host_cpoints_y_extended = new real_t[CPOINT_SIZE_EXTEND]{0};
    m_host_cpoints_z_extended = new real_t[CPOINT_SIZE_EXTEND]{0};

    auto get_cpoint_index_RowMajor = [COL_NUM](int i, int j) -> int {
        int res = i * COL_NUM + j;

        assert(res < 16 && res >= 0);

        return res;
    };

    auto get_cpoint_index_ColMajor = [ROW_NUM](int i, int j) -> int {
        int res = i + j * ROW_NUM;

        assert(res < 16 && res >= 0);

        return res;
    };

    auto get_cpoint_index_ColMajor_tensor_core =
            [ROW_EXTEND_NUM](int i, int j) -> int {
        int res = i + j * ROW_EXTEND_NUM;
        assert(res < 64 && res >= 0);

        return res;
    };

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m_host_cpoints_x[get_cpoint_index_ColMajor(i, j)] =
                    static_cast<float>(surface.getControlPoints()[i][j].getX());
            m_host_cpoints_y[get_cpoint_index_ColMajor(i, j)] =
                    static_cast<float>(surface.getControlPoints()[i][j].getY());
            m_host_cpoints_z[get_cpoint_index_ColMajor(i, j)] =
                    static_cast<float>(surface.getControlPoints()[i][j].getZ());
        }
    }

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m_host_cpoints_x_extended[get_cpoint_index_ColMajor_tensor_core(
                    i, j)] =
                    static_cast<float>(surface.getControlPoints()[i][j].getX());
            m_host_cpoints_y_extended[get_cpoint_index_ColMajor_tensor_core(
                    i, j)] =
                    static_cast<float>(surface.getControlPoints()[i][j].getY());
            m_host_cpoints_z_extended[get_cpoint_index_ColMajor_tensor_core(
                    i, j)] =
                    static_cast<float>(surface.getControlPoints()[i][j].getZ());
        }
    }

    CUDA_ERROR(cudaMalloc(&m_device_cpoints_x, CPOINT_SIZE * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(m_device_cpoints_x, m_host_cpoints_x,
                               CPOINT_SIZE * sizeof(real_t),
                               cudaMemcpyHostToDevice));

    CUDA_ERROR(cudaMalloc(&m_device_cpoints_y, CPOINT_SIZE * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(m_device_cpoints_y, m_host_cpoints_y,
                               CPOINT_SIZE * sizeof(real_t),
                               cudaMemcpyHostToDevice));

    CUDA_ERROR(cudaMalloc(&m_device_cpoints_z, CPOINT_SIZE * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(m_device_cpoints_z, m_host_cpoints_z,
                               CPOINT_SIZE * sizeof(real_t),
                               cudaMemcpyHostToDevice));

    CUDA_ERROR(cudaMalloc(&m_device_cpoints_x_extended,
                          CPOINT_SIZE_EXTEND * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(
            m_device_cpoints_x_extended, m_host_cpoints_x_extended,
            CPOINT_SIZE_EXTEND * sizeof(real_t), cudaMemcpyHostToDevice));

    CUDA_ERROR(cudaMalloc(&m_device_cpoints_y_extended,
                          CPOINT_SIZE_EXTEND * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(
            m_device_cpoints_y_extended, m_host_cpoints_y_extended,
            CPOINT_SIZE_EXTEND * sizeof(real_t), cudaMemcpyHostToDevice));

    CUDA_ERROR(cudaMalloc(&m_device_cpoints_z_extended,
                          CPOINT_SIZE_EXTEND * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(
            m_device_cpoints_z_extended, m_host_cpoints_z_extended,
            CPOINT_SIZE_EXTEND * sizeof(real_t), cudaMemcpyHostToDevice));
}



void GPSplinePatchSurface::init_scalar(CBSplineSurface& surface) {

    const int ROW_NUM = 4;
    const int COL_NUM = 4;
    const size_t CPOINT_SIZE = ROW_NUM * COL_NUM;
    m_host_scalar_cpoints_x = new real_t[CPOINT_SIZE]{ 0 };
    m_host_scalar_cpoints_y = new real_t[CPOINT_SIZE]{ 0 };
    m_host_scalar_cpoints_z = new real_t[CPOINT_SIZE]{ 0 };

    const int ROW_EXTEND_NUM = 8;
    const int COL_EXTEND_NUM = 8;
    const size_t CPOINT_SIZE_EXTEND = ROW_EXTEND_NUM * COL_EXTEND_NUM;

    m_host_scalar_cpoints_x_extended = new real_t[CPOINT_SIZE_EXTEND]{ 0 };
    m_host_scalar_cpoints_y_extended = new real_t[CPOINT_SIZE_EXTEND]{ 0 };
    m_host_scalar_cpoints_z_extended = new real_t[CPOINT_SIZE_EXTEND]{ 0 };

    auto get_cpoint_index_RowMajor = [COL_NUM](int i, int j) -> int {
        int res = i * COL_NUM + j;

        assert(res < 16 && res >= 0);

        return res;
        };

    auto get_cpoint_index_ColMajor = [ROW_NUM](int i, int j) -> int {
        int res = i + j * ROW_NUM;

        assert(res < 16 && res >= 0);

        return res;
        };

    auto get_cpoint_index_ColMajor_tensor_core =
        [ROW_EXTEND_NUM](int i, int j) -> int {
        int res = i + j * ROW_EXTEND_NUM;
        assert(res < 64 && res >= 0);

        return res;
        };

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            auto xx = surface.getScalarPoints();
            auto x2 = surface.getControlPoints();
            m_host_scalar_cpoints_x[get_cpoint_index_ColMajor(i, j)] =
                static_cast<float>(surface.getScalarPoints()[i][j].getX());
            m_host_scalar_cpoints_y[get_cpoint_index_ColMajor(i, j)] =
                static_cast<float>(surface.getScalarPoints()[i][j].getY());
            m_host_scalar_cpoints_z[get_cpoint_index_ColMajor(i, j)] =
                static_cast<float>(surface.getScalarPoints()[i][j].getZ());
        }
    }

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m_host_scalar_cpoints_x_extended[get_cpoint_index_ColMajor_tensor_core(
                i, j)] =
                static_cast<float>(surface.getScalarPoints()[i][j].getX());
            m_host_scalar_cpoints_y_extended[get_cpoint_index_ColMajor_tensor_core(
                i, j)] =
                static_cast<float>(surface.getScalarPoints()[i][j].getY());
            m_host_scalar_cpoints_z_extended[get_cpoint_index_ColMajor_tensor_core(
                i, j)] =
                static_cast<float>(surface.getScalarPoints()[i][j].getZ());
        }
    }

    CUDA_ERROR(cudaMalloc(&m_device_scalar_cpoints_x, CPOINT_SIZE * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(m_device_scalar_cpoints_x, m_host_scalar_cpoints_x,
        CPOINT_SIZE * sizeof(real_t),
        cudaMemcpyHostToDevice));

    CUDA_ERROR(cudaMalloc(&m_device_scalar_cpoints_y, CPOINT_SIZE * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(m_device_scalar_cpoints_y, m_host_scalar_cpoints_y,
        CPOINT_SIZE * sizeof(real_t),
        cudaMemcpyHostToDevice));

    CUDA_ERROR(cudaMalloc(&m_device_scalar_cpoints_z, CPOINT_SIZE * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(m_device_scalar_cpoints_z, m_host_scalar_cpoints_z,
        CPOINT_SIZE * sizeof(real_t),
        cudaMemcpyHostToDevice));

    CUDA_ERROR(cudaMalloc(&m_device_scalar_cpoints_x_extended,
        CPOINT_SIZE_EXTEND * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(
        m_device_scalar_cpoints_x_extended, m_host_scalar_cpoints_x_extended,
        CPOINT_SIZE_EXTEND * sizeof(real_t), cudaMemcpyHostToDevice));

    CUDA_ERROR(cudaMalloc(&m_device_scalar_cpoints_y_extended,
        CPOINT_SIZE_EXTEND * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(
        m_device_scalar_cpoints_y_extended, m_host_scalar_cpoints_y_extended,
        CPOINT_SIZE_EXTEND * sizeof(real_t), cudaMemcpyHostToDevice));

    CUDA_ERROR(cudaMalloc(&m_device_scalar_cpoints_z_extended,
        CPOINT_SIZE_EXTEND * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(
        m_device_scalar_cpoints_z_extended, m_host_scalar_cpoints_z_extended,
        CPOINT_SIZE_EXTEND * sizeof(real_t), cudaMemcpyHostToDevice));
}


//void GPSplinePatchSurface::init_patches() {
//    assert(m_device_cpoints_x != nullptr);
//    assert(m_device_cpoints_y != nullptr);
//    assert(m_device_cpoints_z != nullptr);
//
//    for (size_t i = 0; i < patches.size(); ++i) {
//        auto& patch = patches[i];
//
//        patch.init_device_cpoints(m_device_cpoints_x, m_device_cpoints_y, m_device_cpoints_z);
//        patch.init_host_cpoints(m_host_cpoints_x, m_host_cpoints_y, m_host_cpoints_z);
//
//        patch.init_device_cpoints_extended(m_device_cpoints_x_extended, m_device_cpoints_y_extended,
//                                           m_device_cpoints_z_extended);
//        patch.init_host_cpoints_extended(m_host_cpoints_x_extended, m_host_cpoints_y_extended,
//                                         m_host_cpoints_z_extended);
//
//        patch.init_device_scalar_cpoints(m_device_scalar_cpoints_x, m_device_scalar_cpoints_y,
//                                         m_device_scalar_cpoints_z);
//        patch.init_host_scalar_cpoints(m_host_scalar_cpoints_x, m_host_scalar_cpoints_y, m_host_scalar_cpoints_z);
//
//        patch.init_device_scalar_cpoints_extended(m_device_scalar_cpoints_x_extended,
//                                                  m_device_scalar_cpoints_y_extended,
//                                                  m_device_scalar_cpoints_z_extended);
//        patch.init_host_scalar_cpoints_extended(m_host_scalar_cpoints_x_extended, m_host_scalar_cpoints_y_extended,
//                                                m_host_scalar_cpoints_z_extended);
//
//        patch.bCompactCUDAMapBuffer = this->bCompactCUDAMapBuffer;
//        patch.i_bCCW = this->m_bCCW;
//
//        patch.init_device();
//
//        patch.init_stream();
//    }
//
//    for (size_t p = 0; p < MAX_P; ++p) {
//        for (size_t q = 0; q < MAX_Q; ++q) {
//            size_t local_id = get_local_index(p, q);
//            m_host_mapping_patch_data_arr[local_id] = get_local_patch(local_id).get_gpu_patch_data();
//        }
//    }
//
//    const size_t bytes = MAX_P * MAX_Q * sizeof(GPPatchData);
//    if (m_device_patch_data_arr == nullptr) { CUDA_ERROR(cudaMalloc(&m_device_patch_data_arr, bytes)); }
//
//    CUDA_ERROR(
//            cudaMemcpy(m_device_patch_data_arr, m_host_mapping_patch_data_arr.data(), bytes, cudaMemcpyHostToDevice));
//}


void GPSplinePatchSurface::init_patches() {
    assert(m_device_cpoints_x != nullptr);
    assert(m_device_cpoints_y != nullptr);
    assert(m_device_cpoints_z != nullptr);

    const size_t patch_count = patches.size();
    const size_t stride = GPPatch::device_blob_stride_bytes();
    const size_t total_bytes = stride * patch_count;

    if (m_device_patch_blob_pool == nullptr || m_device_patch_blob_total_bytes < total_bytes) {
        if (m_device_patch_blob_pool) {
            CUDA_ERROR(cudaFree(m_device_patch_blob_pool));
            m_device_patch_blob_pool = nullptr;
        }
        CUDA_ERROR(cudaMalloc(&m_device_patch_blob_pool, total_bytes));
        m_device_patch_blob_stride = stride;
        m_device_patch_blob_total_bytes = total_bytes;
    }

    CUDA_ERROR(cudaMemset(m_device_patch_blob_pool, 0, total_bytes));

    for (size_t i = 0; i < patch_count; ++i) {
        auto& patch = patches[i];

        patch.init_device_cpoints(m_device_cpoints_x, m_device_cpoints_y, m_device_cpoints_z);
        patch.init_host_cpoints(m_host_cpoints_x, m_host_cpoints_y, m_host_cpoints_z);

        patch.init_device_cpoints_extended(m_device_cpoints_x_extended, m_device_cpoints_y_extended,
                                           m_device_cpoints_z_extended);
        patch.init_host_cpoints_extended(m_host_cpoints_x_extended, m_host_cpoints_y_extended,
                                         m_host_cpoints_z_extended);

        patch.init_device_scalar_cpoints(m_device_scalar_cpoints_x, m_device_scalar_cpoints_y,
                                         m_device_scalar_cpoints_z);
        patch.init_host_scalar_cpoints(m_host_scalar_cpoints_x, m_host_scalar_cpoints_y, m_host_scalar_cpoints_z);

        patch.init_device_scalar_cpoints_extended(m_device_scalar_cpoints_x_extended,
                                                  m_device_scalar_cpoints_y_extended,
                                                  m_device_scalar_cpoints_z_extended);
        patch.init_host_scalar_cpoints_extended(m_host_scalar_cpoints_x_extended, m_host_scalar_cpoints_y_extended,
                                                m_host_scalar_cpoints_z_extended);

        patch.bCompactCUDAMapBuffer = this->bCompactCUDAMapBuffer;
        patch.i_bCCW = this->m_bCCW;

        unsigned char* base = reinterpret_cast<unsigned char*>(m_device_patch_blob_pool) + i * stride;
        patch.bind_device_blob(base, stride, /*zero_init=*/false);

        patch.init_stream();
    }

    for (size_t p = 0; p < MAX_P; ++p) {
        for (size_t q = 0; q < MAX_Q; ++q) {
            size_t local_id = get_local_index(p, q);
            m_host_mapping_patch_data_arr[local_id] = get_local_patch(local_id).get_gpu_patch_data();
        }
    }

    const size_t bytes = MAX_P * MAX_Q * sizeof(GPPatchData);
    if (m_device_patch_data_arr == nullptr) { CUDA_ERROR(cudaMalloc(&m_device_patch_data_arr, bytes)); }

    CUDA_ERROR(
            cudaMemcpy(m_device_patch_data_arr, m_host_mapping_patch_data_arr.data(), bytes, cudaMemcpyHostToDevice));
}



void GPSplinePatchSurface::update_patch_data_arr() {
    for (size_t p = 0; p < MAX_P; ++p) {
        for (size_t q = 0; q < MAX_Q; ++q) {
            size_t local_id = get_local_index(p, q);
            auto& global_path = get_global_patch(p, q);

            m_host_mapping_patch_data_arr[local_id] =
                    global_path.get_gpu_patch_data();
            m_host_mapping_patch_data_arr[local_id].m_position_ptr =
                    global_path.m_cuda_position_ptr;
            m_host_mapping_patch_data_arr[local_id].m_normal_ptr =
                    global_path.m_cuda_normal_ptr;
        }
    }

    CUDA_ERROR(cudaMemcpy(
            m_device_patch_data_arr, m_host_mapping_patch_data_arr.data(),
            MAX_P * MAX_Q * sizeof(GPPatchData), cudaMemcpyHostToDevice));
}

void GPSplinePatchSurface::release() {
    release_knot_vector();
    release_cpoints();

    for (auto& patch: patches) { patch.release(); }

    GPU_FREE(m_device_patch_data_arr);
}

void GPSplinePatchSurface::release_knot_vector() {
    if (host_u_knots != nullptr) {
        delete[] host_u_knots;
        host_u_knots = nullptr;
    }

    if (host_v_knots != nullptr) {
        delete[] host_v_knots;
        host_v_knots = nullptr;
    }

    GPU_FREE(device_u_knots);
    GPU_FREE(device_v_knots);
}


void GPSplinePatchSurface::release_cpoints() {
    if (m_host_cpoints_x != nullptr) {
        delete[] m_host_cpoints_x;
        m_host_cpoints_x = nullptr;
    }

    if (m_host_cpoints_y != nullptr) {
        delete[] m_host_cpoints_y;
        m_host_cpoints_y = nullptr;
    }

    if (m_host_cpoints_z != nullptr) {
        delete[] m_host_cpoints_z;
        m_host_cpoints_z = nullptr;
    }

    GPU_FREE(m_device_cpoints_x);
    GPU_FREE(m_device_cpoints_y);
    GPU_FREE(m_device_cpoints_z);


    if (m_host_cpoints_x_extended != nullptr) {
        delete[] m_host_cpoints_x_extended;
        m_host_cpoints_x_extended = nullptr;
    }

    if (m_host_cpoints_y_extended != nullptr) {
        delete[] m_host_cpoints_y_extended;
        m_host_cpoints_y_extended = nullptr;
    }

    if (m_host_cpoints_z_extended != nullptr) {
        delete[] m_host_cpoints_z_extended;
        m_host_cpoints_z_extended = nullptr;
    }


    GPU_FREE(m_device_cpoints_x_extended);
    GPU_FREE(m_device_cpoints_y_extended);
    GPU_FREE(m_device_cpoints_z_extended);
}

void GPSplinePatchSurface::build_tessellation(size_t new_p, size_t new_q,
                                              bool bUsingTCEC) {

    new_p = std::min(new_p, MAX_P);
    new_p = std::max(new_p, static_cast<size_t>(1));

    new_q = std::min(new_q, MAX_Q);
    new_q = std::max(new_q, static_cast<size_t>(1));

    cur_p = new_p;
    cur_q = new_q;

    real_t du = 1.0f / new_p;
    real_t dv = 1.0f / new_q;

    size_t i = 0;
    for (size_t p = 0; p < new_p; ++p) {
        for (size_t q = 0; q < new_q; ++q) {
            auto& patch = get_local_patch(p, q);
            patch.init_u(du * p, du / (GPPatch::n - 1));
            patch.init_v(dv * q, dv / (GPPatch::m - 1));
            patch.m_cuda_position_ptr = m_host_positions_ptr;
            patch.m_cuda_normal_ptr = m_host_normal_ptr;
            patch.m_cuda_scalar_ptr = m_host_scalar_ptr;

            size_t tt = m_offset + i * 192;
            patch.evaluation_tensor_core(bUsingTCEC, tt);
            i = i + 1;
        }
    }

}

void GPSplinePatchSurface::set_tessellation_factor(size_t new_p, size_t new_q) {
    new_p = std::min(new_p, MAX_P);
    new_p = std::max(new_p, static_cast<size_t>(1));

    new_q = std::min(new_q, MAX_Q);
    new_q = std::max(new_q, static_cast<size_t>(1));

    cur_p = new_p;
    cur_q = new_q;

    real_t du = 1.0 / new_p;
    real_t dv = 1.0 / new_q;

    for (size_t p = 0; p < new_p; ++p) {
        for (size_t q = 0; q < new_q; ++q) {
            auto& patch = get_local_patch(p, q);
            patch.init_u(du * p, du / (GPPatch::n - 1));
            patch.init_v(dv * q, dv / (GPPatch::m - 1));
        }
    }
}

void GPSplinePatchSurface::run_evaluation() {
    for (size_t p = 0; p < cur_p; ++p) {
        for (size_t q = 0; q < cur_q; ++q) {
            auto& patch = get_local_patch(p, q);
            patch.stream_driven_evaluation();
        }
    }
}

GPEND