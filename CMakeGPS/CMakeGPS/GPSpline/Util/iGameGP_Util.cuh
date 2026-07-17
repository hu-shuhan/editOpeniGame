#pragma once
#include <cuda_runtime.h>
#include <algorithm>
#include <numeric>
#include <random>
#include "iGameGP_Macros.h"
#include "iGameGP_CUDA_Macros.cuh"

namespace gpmesh {

inline char* get_cmd_option(char** begin, char** end, const std::string& option)
{
    char** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) {
        return *itr;
    }
    return 0;
}

inline bool cmd_option_exists(char**             begin,
                              char**             end,
                              const std::string& option)
{
    return std::find(begin, end, option) != end;
}

inline void print_device_memory_usage()
{
    size_t free_t, total_t;
    CUDA_ERROR(cudaMemGetInfo(&free_t, &total_t));
    double free_m  = (double)free_t / (double)1048576.0;
    double total_m = (double)total_t / (double)1048576.0;
    double used_m  = total_m - free_m;
}


template <typename T>
inline uint32_t find_index(const T entry, const std::vector<T>& vect)
{
    typename std::vector<T>::const_iterator it =
        std::find(vect.begin(), vect.end(), entry);
    if (it == vect.end()) {
        return std::numeric_limits<uint32_t>::max();
    }
    return uint32_t(it - vect.begin());
}

template <typename T>
inline T find_index(const T* arr, const T arr_size, const T entry)
{
    const T* begin = arr;
    const T* end   = arr + arr_size;
    const T* it    = std::find(begin, end, entry);
    if (it == end) {
        return std::numeric_limits<T>::max();
    }
    return it - begin;
}

template <typename T>
inline void random_shuffle(T*             d_in,
                           const uint32_t end,
                           const uint32_t start = 0)
{
    std::random_device rd;
    std::mt19937       g(rd());
    std::shuffle(d_in + start, d_in + end, g);
}

template <typename T>
inline void fill_with_sequential_numbers(T*             arr,
                                         const uint32_t size,
                                         const T        start = 0)
{
    std::iota(arr, arr + size, start);
}


template <typename T, typename dataT>
bool compare(const dataT* gold,
             const dataT* arr,
             const T      size,
             const bool   verbose = false,
             const dataT  tol     = 10E-5)
{

    bool result = true;
    for (T i = 0; i < size; i++) {
        if (std::abs(double(gold[i]) - double(arr[i])) > tol) {
            if (verbose) {
                gpmesh_WARN("compare() mismatch at {} gold = {} arr = {} ",
                            i,
                            gold[i],
                            arr[i]);
                result = false;
            } else {
                return false;
            }
        }
    }
    return result;
}

template <typename T>
void copy(const std::vector<T>& src, std::vector<T>& tar, int tar_start = 0)
{
    std::copy(src.begin(), src.end(), tar.data() + tar_start);
}


template <typename T>
inline void compute_avg_stddev(const T* arr,
                               uint32_t size,
                               double&  avg,
                               double&  stddev)
{
    if (size == 1) {
        avg    = arr[0];
        stddev = 0;
        return;
    }
    avg = 0;
    for (uint32_t i = 0; i < size; i++) {
        avg += arr[i];
    }
    avg /= size;

    double sum = 0;
    for (uint32_t i = 0; i < size; i++) {
        double diff = double(arr[i]) - avg;
        sum += diff * diff;
    }
    stddev = std::sqrt(double(sum) / double(size - 1));
    return;
}
template <typename T>
inline void compute_avg_stddev_max_min_rs(const T* arr_rs,
                                          uint32_t size,
                                          double&  avg,
                                          double&  stddev,
                                          T&       max,
                                          T&       min)
{
    uint32_t* arr = (uint32_t*)malloc(size * sizeof(uint32_t));
    max           = std::numeric_limits<T>::min();
    min           = std::numeric_limits<T>::max();
    for (uint32_t i = 0; i < size; i++) {
        uint32_t start = (i == 0) ? 0 : arr_rs[i - 1];
        uint32_t end   = arr_rs[i];
        arr[i]         = end - start;
        max            = std::max(max, arr[i]);
        min            = std::min(min, arr[i]);
    }

    compute_avg_stddev(arr, size, avg, stddev);

    free(arr);
}

template <typename T>
inline size_t binary_search(const std::vector<T>& list,
                            const T               target,
                            const size_t          start,
                            const size_t          end)
{
    assert(list.size() >= end);
    assert(end >= start);


    if (end - start < 20) {
        for (size_t i = start; i < end; ++i) {
            if (list[i] == target) {
                return i;
            }
        }
    } else {
        auto loc =
            std::lower_bound(list.begin() + start, list.begin() + end, target);
        if (loc != (list.begin() + end) && (target == *loc)) {
            return loc - list.begin();
        }
    }

    return std::numeric_limits<size_t>::max();
}


template <typename T>
inline void inplace_remove_duplicates_sorted(std::vector<T>& sort_vec)
{
    if (sort_vec.size() == 0) {
        return;
    }

    uint32_t next_unique_id = 1;
    T        prev_value     = sort_vec.front();
    for (uint32_t i = 1; i < sort_vec.size(); ++i) {
        T curr_val = sort_vec[i];
        if (curr_val != prev_value) {
            sort_vec[next_unique_id++] = curr_val;
            prev_value                 = curr_val;
        }
    }

    sort_vec.resize(next_unique_id);
}

template <typename T>
inline void shuffle_obj(std::vector<std::vector<uint32_t>>& Faces,
                        std::vector<std::vector<T>>&        Verts)
{
    {
        std::vector<uint32_t> rand(Verts.size());
        fill_with_sequential_numbers(rand.data(), rand.size());
        random_shuffle(rand.data(), rand.size());

        for (auto& f : Faces) {
            for (uint32_t i = 0; i < f.size(); ++i) {
                f[i] = rand[f[i]];
            }
        }

        std::vector<std::vector<T>> verts_old(Verts);
        for (uint32_t v = 0; v < Verts.size(); ++v) {
            for (uint32_t i = 0; i < Verts[v].size(); ++i) {
                Verts[rand[v]][i] = verts_old[v][i];
            }
        }
    }

    {
        std::vector<uint32_t> rand(Faces.size());
        fill_with_sequential_numbers(rand.data(), rand.size());
        random_shuffle(rand.data(), rand.size());

        std::vector<std::vector<uint32_t>> faces_old(Faces);
        for (uint32_t f = 0; f < Faces.size(); ++f) {
            for (uint32_t i = 0; i < Faces[f].size(); ++i) {
                Faces[rand[f]][i] = faces_old[f][i];
            }
        }
    }
}


inline std::string remove_extension(const std::string& filename)
{   
    size_t lastdot = filename.find_last_of(".");
    if (lastdot == std::string::npos)
        return filename;
    return filename.substr(0, lastdot);
}

inline std::string extract_file_name(const std::string& full_path)
{
    std::string filename  = remove_extension(full_path);
    size_t      lastslash = filename.find_last_of("/\\");

    return filename.substr(lastslash + 1);
}

__device__ __host__ __inline__ uint32_t expand_to_align(
    uint32_t init_bytes,
    uint32_t alignment = 128)
{
    uint32_t remainder = init_bytes % alignment;
    if (remainder == 0) {
        return init_bytes;
    }
    return init_bytes + alignment - remainder;
};

namespace detail {

struct edge_key_hash
{
    inline std::size_t operator()(const std::pair<uint32_t, uint32_t>& e_key) const
    {
        return std::hash<uint32_t>()(e_key.first * 8100 + e_key.second * 11003);
    }
};

struct edge_key_equal
{
    inline bool operator()(const std::pair<uint32_t, uint32_t>& a, const std::pair<uint32_t, uint32_t>& b) const
    {
        return a.first == b.first && a.second == b.second;
    }
};

inline std::pair<uint32_t, uint32_t> edge_key(const uint32_t v0,
                                              const uint32_t v1)
{
    uint32_t i = std::max(v0, v1);
    uint32_t j = std::min(v0, v1);
    return std::make_pair(i, j);
}

template <typename T>
__device__ __host__ __inline__ void align(const std::size_t byte_alignment,
                                          T*&               ptr) noexcept
{
    const uint64_t intptr    = reinterpret_cast<uint64_t>(ptr);
    const uint64_t remainder = intptr % byte_alignment;
    if (remainder == 0) {
        return;
    }
    const uint64_t aligned = intptr + byte_alignment - remainder;
    ptr                    = reinterpret_cast<T*>(aligned);
}

}    
}    