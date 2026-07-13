#pragma once
#include <algorithm>
#include <assert.h>
#include <cuda_runtime_api.h>

#define OFFSET(row, col, ld) ((row) * (ld) + (col))
#define FLOAT4(pointer) (reinterpret_cast<float4*>(&(pointer))[0])

template<typename real_t>
__global__ void evaluation_D01_GISMO(uint16_t p, real_t* H, real_t* H_prime) {
    uint16_t eval_num = 7 * p + 1;

    if (threadIdx.x >= eval_num) { return; }

    uint16_t id = threadIdx.x;

    real_t u = 1.0f / static_cast<real_t>(eval_num) * id;

    if (id == 0) {
    }

    const int m_p = 3;
    const int p1 = m_p + 1;

    real_t* left = new real_t[p1]{0};
    real_t* right = new real_t[p1]{0};

    real_t* ndu = new real_t[p1 * p1];
    real_t* a = new real_t[2 * p1];

    ndu[0] = static_cast<real_t>(1);

    for (int j = 1; j <= m_p; ++j) {
        left[j] = u - 0;
        right[j] = 1.0 - u;
        real_t saved = static_cast<real_t>(0);

        for (int r = 0; r < j; ++r) {
            ndu[j * p1 + r] = right[r + 1] + left[j - r];
            const real_t temp = ndu[r * p1 + j - 1] / ndu[j * p1 + r];
            ndu[r * p1 + j] =
                    saved +
                    right[r + 1] * temp;       
            saved = left[j - r] * temp;
        }

        ndu[j * p1 + j] = saved;
    }

    size_t start_id = 4 * id;
    for (int j = 0; j <= m_p; ++j) {
        assert(start_id + j < 4 * eval_num);
        H[start_id + j] = ndu[j * p1 + m_p];
    }

    const int n = 1;

    for (int r = 0; r <= m_p; ++r) {
        real_t* a1 = &a[0];
        real_t* a2 = &a[p1];

        a1[0] = static_cast<real_t>(1);

        for (int k = 1; k <= n; ++k) {
            int rk, pk, j1, j2;
            real_t d(0);
            rk = r - k;
            pk = m_p - k;

            if (r >= k) {
                a2[0] = a1[0] / ndu[(pk + 1) * p1 + rk];
                d = a2[0] * ndu[rk * p1 + pk];
            }

            j1 = (rk >= -1 ? 1 : -rk);
            j2 = (r - 1 <= pk ? k - 1 : m_p - r);

            for (int j = j1; j <= j2; j++) {
                a2[j] = (a1[j] - a1[j - 1]) / ndu[(pk + 1) * p1 + rk + j];
                d += a2[j] * ndu[(rk + j) * p1 + pk];
            }

            if (r <= pk) {
                a2[k] = -a1[k - 1] / ndu[(pk + 1) * p1 + r];
                d += a2[k] * ndu[r * p1 + pk];
            }

            assert(start_id + r < 4 * eval_num);
            H_prime[start_id + r] = d * m_p;
        }
    }

    delete[] left;
    delete[] right;
    delete[] ndu;
    delete[] a;
}


template<typename real_t>
__global__ void evaluation_D01_NURBS_BOOK(uint16_t p, real_t* H) {
    uint16_t eval_num = 7 * p + 1;

    if (threadIdx.x >= eval_num) { return; }

    size_t id = threadIdx.x;

    size_t start_id = 4 * id;

    real_t u = 1.0f / static_cast<real_t>(eval_num) * id;

    real_t* N = new real_t[4];

    int j, r;
    real_t saved, temp;
    real_t* lef = new real_t[4];
    real_t* rig = new real_t[4];

    N[0] = 1.0;

    for (j = 1; j <= 3; ++j) {
        lef[j] = u - 0;
        rig[j] = 1 - u;
        saved = 0.0;
        for (r = 0; r < j; ++r) {
            temp = N[r] / (rig[r + 1] + lef[j - r]);
            N[r] = saved + rig[r + 1] * temp;
            saved = lef[j - r] * temp;
        }

        N[j] = saved;
    }

    for (int i = 0; i < 4; ++i) { H[start_id + i] = N[i]; }

    delete[] N;
    delete[] lef;
    delete[] rig;
}

template<typename real_t>
__global__ void evaluation_D01_NURBS_BOOK(uint16_t p, real_t* H,
                                          real_t* H_prime) {
    uint16_t eval_num = 7 * p + 1;

    if (threadIdx.x >= eval_num) { return; }

    size_t id = threadIdx.x;

    size_t start_id = 4 * id;

    real_t u = 1.0f / static_cast<real_t>(eval_num) * id;

    real_t* N = new real_t[4];

    int j, r;
    real_t saved, temp;
    real_t* lef = new real_t[4];
    real_t* rig = new real_t[4];

    N[0] = 1.0;

    for (j = 1; j <= 3; ++j) {
        lef[j] = u - 0;
        rig[j] = 1 - u;
        saved = 0.0;
        for (r = 0; r < j; ++r) {
            temp = N[r] / (rig[r + 1] + lef[j - r]);
            N[r] = saved + rig[r + 1] * temp;
            saved = lef[j - r] * temp;
        }

        N[j] = saved;
    }

    for (int i = 0; i < 4; ++i) { H[start_id + i] = N[i]; }

    delete[] N;
    delete[] lef;
    delete[] rig;
}


template<typename real_t>
__global__ void evaluation_D01_GISMO(real_t start, real_t delta, const size_t N,
                                     real_t* H, real_t* H_prime) {

    if (threadIdx.x >= N) { return; }

    uint16_t id = threadIdx.x;

    real_t u = start + delta * id;

    if (id == 0) {
    }

    const int m_p = 3;
    const int p1 = m_p + 1;

    real_t left[p1] = {0};
    real_t right[p1] = {0};

    real_t ndu[p1 * p1] = {0};
    real_t a[2 * p1] = {0};

    ndu[0] = static_cast<real_t>(1);

    for (int j = 1; j <= m_p; ++j) {
        left[j] = u - 0;
        right[j] = 1.0 - u;
        real_t saved = static_cast<real_t>(0);

        for (int r = 0; r < j; ++r) {
            ndu[j * p1 + r] = right[r + 1] + left[j - r];
            const real_t temp = ndu[r * p1 + j - 1] / ndu[j * p1 + r];
            ndu[r * p1 + j] =
                    saved +
                    right[r + 1] * temp;       
            saved = left[j - r] * temp;
        }

        ndu[j * p1 + j] = saved;
    }

    size_t start_id = 4 * id;
    for (int j = 0; j <= m_p; ++j) {
        assert(start_id + j < 4 * N);
        H[start_id + j] = ndu[j * p1 + m_p];
    }

    const int n = 1;

    for (int r = 0; r <= m_p; ++r) {
        real_t* a1 = &a[0];
        real_t* a2 = &a[p1];

        a1[0] = static_cast<real_t>(1);

        for (int k = 1; k <= n; ++k) {
            int rk, pk, j1, j2;
            real_t d(0);
            rk = r - k;
            pk = m_p - k;

            if (r >= k) {
                a2[0] = a1[0] / ndu[(pk + 1) * p1 + rk];
                d = a2[0] * ndu[rk * p1 + pk];
            }

            j1 = (rk >= -1 ? 1 : -rk);
            j2 = (r - 1 <= pk ? k - 1 : m_p - r);

            for (int j = j1; j <= j2; j++) {
                a2[j] = (a1[j] - a1[j - 1]) / ndu[(pk + 1) * p1 + rk + j];
                d += a2[j] * ndu[(rk + j) * p1 + pk];
            }

            if (r <= pk) {
                a2[k] = -a1[k - 1] / ndu[(pk + 1) * p1 + r];
                d += a2[k] * ndu[r * p1 + pk];
            }

            assert(start_id + r < 4 * N);
            H_prime[start_id + r] = d * m_p;
        }
    }
}

template<typename real_t>
__global__ void evaluation_D01_GPU_HHprime_p(real_t start, real_t delta,
                                             const size_t N, real_t* HHprime) {
    if (threadIdx.x >= N) { return; }

    uint16_t id = threadIdx.x;

    real_t u = start + delta * id;

    if (id == 0) {
    }

    const int m_p = 3;
    const int p1 = m_p + 1;

    real_t left[p1] = {0};
    real_t right[p1] = {0};

    real_t ndu[p1 * p1] = {0};
    real_t a[2 * p1] = {0};

    real_t* H = HHprime;
    real_t* Hprime = &HHprime[8 * N];

    ndu[0] = static_cast<real_t>(1);

    for (int j = 1; j <= m_p; ++j) {
        left[j] = u - 0;
        right[j] = 1.0 - u;
        real_t saved = static_cast<real_t>(0);

        for (int r = 0; r < j; ++r) {
            ndu[j * p1 + r] = right[r + 1] + left[j - r];
            const real_t temp = ndu[r * p1 + j - 1] / ndu[j * p1 + r];
            ndu[r * p1 + j] =
                    saved +
                    right[r + 1] * temp;       
            saved = left[j - r] * temp;
        }

        ndu[j * p1 + j] = saved;
    }

    size_t start_id = 8 * id;
    for (int j = 0; j <= m_p; ++j) {
        assert((start_id + j) + 4 < 8 * N);
        H[start_id + j] = ndu[j * p1 + m_p];
    }

    const int n = 1;

    for (int r = 0; r <= m_p; ++r) {
        real_t* a1 = &a[0];
        real_t* a2 = &a[p1];

        a1[0] = static_cast<real_t>(1);

        for (int k = 1; k <= n; ++k) {
            int rk, pk, j1, j2;
            real_t d(0);
            rk = r - k;
            pk = m_p - k;

            if (r >= k) {
                a2[0] = a1[0] / ndu[(pk + 1) * p1 + rk];
                d = a2[0] * ndu[rk * p1 + pk];
            }

            j1 = (rk >= -1 ? 1 : -rk);
            j2 = (r - 1 <= pk ? k - 1 : m_p - r);

            for (int j = j1; j <= j2; j++) {
                a2[j] = (a1[j] - a1[j - 1]) / ndu[(pk + 1) * p1 + rk + j];
                d += a2[j] * ndu[(rk + j) * p1 + pk];
            }

            if (r <= pk) {
                a2[k] = -a1[k - 1] / ndu[(pk + 1) * p1 + r];
                d += a2[k] * ndu[r * p1 + pk];
            }

            assert(start_id + r + 4 < 8 * N);
            Hprime[start_id + r] = d * m_p;
        }
    }
}

template<typename real_t>
__global__ void evaluation_D01_GPU_H_q_extended_and_Hprime_q_extended(
        real_t start, real_t delta, const size_t N, real_t* H_extended,
        real_t* Hprime_extended) {
    if (threadIdx.x >= N) { return; }

    uint16_t id = threadIdx.x;

    real_t u = start + delta * id;

    if (id == 0) {
    }

    const int m_p = 3;
    const int p1 = m_p + 1;

    real_t left[p1] = {0};
    real_t right[p1] = {0};

    real_t ndu[p1 * p1] = {0};
    real_t a[2 * p1] = {0};

    real_t* H = H_extended;
    real_t* Hprime = Hprime_extended;

    ndu[0] = static_cast<real_t>(1);

    for (int j = 1; j <= m_p; ++j) {
        left[j] = u - 0;
        right[j] = 1.0 - u;
        real_t saved = static_cast<real_t>(0);

        for (int r = 0; r < j; ++r) {
            ndu[j * p1 + r] = right[r + 1] + left[j - r];
            const real_t temp = ndu[r * p1 + j - 1] / ndu[j * p1 + r];
            ndu[r * p1 + j] =
                    saved +
                    right[r + 1] * temp;       
            saved = left[j - r] * temp;
        }

        ndu[j * p1 + j] = saved;
    }

    size_t start_id = 8 * id;
    for (int j = 0; j <= m_p; ++j) {
        assert((start_id + j) + 4 < 8 * N);
        H[start_id + j] = ndu[j * p1 + m_p];
    }

    const int n = 1;

    for (int r = 0; r <= m_p; ++r) {
        real_t* a1 = &a[0];
        real_t* a2 = &a[p1];

        a1[0] = static_cast<real_t>(1);

        for (int k = 1; k <= n; ++k) {
            int rk, pk, j1, j2;
            real_t d(0);
            rk = r - k;
            pk = m_p - k;

            if (r >= k) {
                a2[0] = a1[0] / ndu[(pk + 1) * p1 + rk];
                d = a2[0] * ndu[rk * p1 + pk];
            }

            j1 = (rk >= -1 ? 1 : -rk);
            j2 = (r - 1 <= pk ? k - 1 : m_p - r);

            for (int j = j1; j <= j2; j++) {
                a2[j] = (a1[j] - a1[j - 1]) / ndu[(pk + 1) * p1 + rk + j];
                d += a2[j] * ndu[(rk + j) * p1 + pk];
            }

            if (r <= pk) {
                a2[k] = -a1[k - 1] / ndu[(pk + 1) * p1 + r];
                d += a2[k] * ndu[r * p1 + pk];
            }

            assert(start_id + r + 4 < 8 * N);
            Hprime[start_id + r] = d * m_p;
        }
    }
}


template<typename real_t>
__global__ void write_position(real_t* position_ptr, size_t offset, size_t size,
                               real_t* SSv_x, real_t* SSv_y, real_t* SSv_z) {
    assert(offset % sizeof(real_t) == 0);
    assert(size % sizeof(real_t) == 0);

    size_t start_index = offset;
    size_t length = size;

    size_t limit_index = start_index + length;

    uint32_t ids[2] = {threadIdx.x, threadIdx.x + 32};

    for (int i = 0; i < 2; ++i) {
        uint32_t id = ids[i];

        if (id >= length) continue;

        uint32_t id_x = id * 3 + 0;
        uint32_t id_y = id * 3 + 1;
        uint32_t id_z = id * 3 + 2;
        assert(start_index + id_x < limit_index && id < 64);
        position_ptr[start_index + id_x] = SSv_x[id];
        assert(start_index + id_y < limit_index);
        position_ptr[start_index + id_y] = SSv_y[id];
        assert(start_index + id_z < limit_index);
        position_ptr[start_index + id_z] = SSv_z[id];
    }
}


template<typename real_t>
__device__ void CrossProduct1D(real_t a[3], real_t b[3], real_t c[3]) {
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
}

template<typename real_t>
__device__ void NormalizeInPlace(real_t norm[3]) {
    real_t length =
            sqrt(norm[0] * norm[0] + norm[1] * norm[1] + norm[2] * norm[2]);

    norm[0] /= length;
    norm[1] /= length;
    norm[2] /= length;
}

template<typename real_t>
__global__ void write_normal(real_t* normal_ptr, size_t offset, size_t size,
                             real_t* SSv_x, real_t* SSv_y, real_t* SSv_z,
                             real_t* Su0_x, real_t* Su0_y, real_t* Su0_z,
                             bool bCCW = true) {
    assert(offset % sizeof(real_t) == 0);
    assert(size % sizeof(real_t) == 0);

    size_t start_index = offset;
    size_t length = size;

    size_t limit_index = start_index + length;

    uint32_t ids[2] = {threadIdx.x, threadIdx.x + 32};

    for (int i = 0; i < 2; ++i) {
        uint32_t id = ids[i];
        assert(id < 64);
        if (id >= length) continue;

        real_t dv[3] = {SSv_x[id + 64], SSv_y[id + 64], SSv_z[id + 64]};
        real_t du[3] = {Su0_x[id], Su0_y[id], Su0_z[id]};

        real_t norm[3];

        CrossProduct1D<real_t>(du, dv, norm);
        NormalizeInPlace<real_t>(norm);

        if (!bCCW) {
            norm[0] *= -1;
            norm[1] *= -1;
            norm[2] *= -1;
        }

        uint32_t id_x = id * 3 + 0;
        uint32_t id_y = id * 3 + 1;
        uint32_t id_z = id * 3 + 2;
        assert(start_index + id_z < limit_index);
        normal_ptr[start_index + id_x] = norm[0];
        normal_ptr[start_index + id_y] = norm[1];
        normal_ptr[start_index + id_z] = norm[2];
    }
}