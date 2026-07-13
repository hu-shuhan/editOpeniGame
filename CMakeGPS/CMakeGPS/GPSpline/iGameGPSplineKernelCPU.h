#pragma once

#include <thread>
#include <tbb/parallel_for.h>
#include <vector>

template<typename real_t>
void evaluation_D01_CPU(uint16_t p, real_t *H, real_t *H_prime) {

    uint16_t eval_num = 7 * p + 1;
    real_t du = 1.0f / static_cast<real_t>(eval_num);
    std::vector<real_t> u(eval_num);

    for (int i = 0; i < eval_num; ++i) {
        u[i] = du * i;
    }

    const int m_p = 3;
    const int p1 = m_p + 1;

    real_t *left = new real_t[p1]{0};
    real_t *right = new real_t[p1]{0};

    real_t *ndu = new real_t[p1 * p1];
    real_t *a = new real_t[2 * p1];


    for (int i = 0; i < eval_num; ++i) {
        ndu[0] = static_cast<real_t>(1);

        for (int j = 1; j <= m_p; ++j) {
            left[j] = u[i] - 0;
            right[j] = 1.0 - u[i];

            real_t saved = static_cast<real_t>(0);

            for (int r = 0; r < j; r++)               
            {
                ndu[j * p1 + r] = right[r + 1] + left[j - r];
                const real_t temp = ndu[r * p1 + j - 1] / ndu[j * p1 + r];
                ndu[r * p1 + j] = saved + right[r + 1] * temp;      
                saved = left[j - r] * temp;
            }
            ndu[j * p1 + j] = saved;
        }

        size_t start_id = 4 * i;
        for (int j = 0; j <= m_p; ++j) {
            H[start_id + j] = ndu[j * p1 + m_p];
        }

        const int n = 1;

        for (int r = 0; r <= m_p; r++) {
            real_t *a1 = &a[0];
            real_t *a2 = &a[p1];

            a1[0] = static_cast<real_t>(1);

            for (int k = 1; k <= n; k++) {
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

                H_prime[start_id + r] = d * m_p;
            }
        }
    }

    delete[] left;
    delete[] right;
    delete[] ndu;
    delete[] a;
}

template<typename real_t>
void evaluation_D01_CPU(real_t start, real_t delta, const size_t N, real_t *H, real_t *H_prime) {

    size_t eval_num = N;
    real_t du = delta;
    std::vector<real_t> u(eval_num);

    for (int i = 0; i < eval_num; ++i) {
        u[i] = start + du * i;
    }

    const int m_p = 3;
    const int p1 = m_p + 1;

    real_t left[p1] = {0};
    real_t right[p1] = {0};

    real_t ndu[p1 * p1] = {0};
    real_t a[2 * p1] = {0};

    for (int i = 0; i < eval_num; ++i) {

        ndu[0] = static_cast<real_t>(1);

        for (int j = 1; j <= m_p; ++j) {
            left[j] = u[i] - 0;
            right[j] = 1.0 - u[i];

            real_t saved = static_cast<real_t>(0);

            for (int r = 0; r < j; r++)               
            {
                ndu[j * p1 + r] = right[r + 1] + left[j - r];
                const real_t temp = ndu[r * p1 + j - 1] / ndu[j * p1 + r];
                ndu[r * p1 + j] = saved + right[r + 1] * temp;      
                saved = left[j - r] * temp;
            }
            ndu[j * p1 + j] = saved;
        }

        size_t start_id = 4 * i;
        for (int j = 0; j <= m_p; ++j) {
            assert(start_id + j < 4 * N);
            H[start_id + j] = ndu[j * p1 + m_p];
        }

        const int n = 1;

        for (int r = 0; r <= m_p; r++) {
            real_t *a1 = &a[0];
            real_t *a2 = &a[p1];

            a1[0] = static_cast<real_t>(1);

            for (int k = 1; k <= n; k++) {
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
}

template<typename real_t>
void evaluation_D01_CPU_HHprime_p(real_t start, real_t delta, const size_t N, real_t * HHprime)
{
    assert(N == 8);

    size_t eval_num = N;
    real_t du = delta;
    std::vector<real_t> u(eval_num);

    for (int i = 0; i < eval_num; ++i) {
        u[i] = start + du * i;
    }

    const int m_p = 3;
    const int p1 = m_p + 1;

    real_t left[p1] = {0};
    real_t right[p1] = {0};

    real_t ndu[p1 * p1] = {0};
    real_t a[2 * p1] = {0};

    real_t* H = HHprime;
    real_t* Hprime = &HHprime[8 * N];

    for (int i = 0; i < eval_num; ++i) {

        ndu[0] = static_cast<real_t>(1);

        for (int j = 1; j <= m_p; ++j) {
            left[j] = u[i] - 0;
            right[j] = 1.0 - u[i];

            real_t saved = static_cast<real_t>(0);

            for (int r = 0; r < j; r++)               
            {
                ndu[j * p1 + r] = right[r + 1] + left[j - r];
                const real_t temp = ndu[r * p1 + j - 1] / ndu[j * p1 + r];
                ndu[r * p1 + j] = saved + right[r + 1] * temp;      
                saved = left[j - r] * temp;
            }
            ndu[j * p1 + j] = saved;
        }

        size_t start_id = 8 * i;
        for (int j = 0; j <= m_p; ++j) {
            assert(start_id + j < 8 * N);
            H[start_id + j] = ndu[j * p1 + m_p];
        }

        const int n = 1;

        for (int r = 0; r <= m_p; r++) {
            real_t *a1 = &a[0];
            real_t *a2 = &a[p1];

            a1[0] = static_cast<real_t>(1);

            for (int k = 1; k <= n; k++) {
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

                assert((start_id + r) + r < 8 * N);
                Hprime[start_id + r] = d * m_p;
            }
        }
    }
}

template<typename real_t>
void evaluation_D01_CPU_H_q_extended_and_Hprime_q_extended(real_t start, real_t delta, const size_t N, real_t * H_extended, real_t * Hprime_extended)
{
    assert(N == 8);

    size_t eval_num = N;
    real_t du = delta;
    std::vector<real_t> u(eval_num);

    for (int i = 0; i < eval_num; ++i) {
        u[i] = start + du * i;
    }

    const int m_p = 3;
    const int p1 = m_p + 1;

    real_t left[p1] = {0};
    real_t right[p1] = {0};

    real_t ndu[p1 * p1] = {0};
    real_t a[2 * p1] = {0};

    real_t* H = H_extended;
    real_t* Hprime = Hprime_extended;

    for (int i = 0; i < eval_num; ++i) {

        ndu[0] = static_cast<real_t>(1);

        for (int j = 1; j <= m_p; ++j) {
            left[j] = u[i] - 0;
            right[j] = 1.0 - u[i];

            real_t saved = static_cast<real_t>(0);

            for (int r = 0; r < j; r++)               
            {
                ndu[j * p1 + r] = right[r + 1] + left[j - r];
                const real_t temp = ndu[r * p1 + j - 1] / ndu[j * p1 + r];
                ndu[r * p1 + j] = saved + right[r + 1] * temp;      
                saved = left[j - r] * temp;
            }
            ndu[j * p1 + j] = saved;
        }

        size_t start_id = 8 * i;
        for (int j = 0; j <= m_p; ++j) {
            assert(start_id + j < 8 * N);
            H[start_id + j] = ndu[j * p1 + m_p];
        }

        const int n = 1;

        for (int r = 0; r <= m_p; r++) {
            real_t *a1 = &a[0];
            real_t *a2 = &a[p1];

            a1[0] = static_cast<real_t>(1);

            for (int k = 1; k <= n; k++) {
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

                assert((start_id + r) + r < 8 * N);
                Hprime[start_id + r] = d * m_p;
            }
        }
    }
}