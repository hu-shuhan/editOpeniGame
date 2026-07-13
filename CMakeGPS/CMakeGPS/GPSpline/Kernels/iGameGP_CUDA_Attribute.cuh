#pragma once
#include "Kernels/GP_CUDA_Util.cuh"
#include <cub/block/block_reduce.cuh>

#include "../Util/GP_Macros.h"


namespace gpmesh {

template <typename T>
class Attribute;

template <typename T>
class SplineAttributeBase;

template <typename T>
class SplineAttributeGlobal;

class SplineAttributeGlobalMappingContext;

namespace detail {

template <class T, uint32_t blockSize>
__device__ __forceinline__ void cub_block_sum(const T thread_val,
                                              T*      d_block_output)
{
    typedef cub::BlockReduce<T, blockSize>       BlockReduce;
    __shared__ typename BlockReduce::TempStorage temp_storage;
    T block_sum = BlockReduce(temp_storage).Sum(thread_val);

    if (threadIdx.x == 0) {
        d_block_output[blockIdx.x] = block_sum;
    }
}

template <class T, uint32_t blockSize>
__launch_bounds__(blockSize) __global__
    void norm2_kernel(const Attribute<T> X,
                      const uint16_t*    d_element_per_patch,
                      const uint32_t     num_patches,
                      const uint32_t     num_attributes,
                      T*                 d_block_output,
                      uint32_t           attribute_id)
{
    uint32_t p_id = blockIdx.x;
    if (p_id < num_patches) {
        const uint16_t element_per_patch = d_element_per_patch[p_id];
        T              thread_val        = 0;
        for (uint16_t i = threadIdx.x; i < element_per_patch; i += blockSize) {
            if (attribute_id != INVALID32) {
                const T val = X(p_id, i, attribute_id);
                thread_val += val * val;
            } else {
                for (uint32_t j = 0; j < num_attributes; ++j) {
                    const T val = X(p_id, i, j);
                    thread_val += val * val;
                }
            }
        }

        cub_block_sum<T, blockSize>(thread_val, d_block_output);
    }
}


template <typename T, uint32_t blockSize>
__launch_bounds__(blockSize) __global__
    void dot_kernel(const Attribute<T> X,
                    const Attribute<T> Y,
                    const uint16_t*    d_element_per_patch,
                    const uint32_t     num_patches,
                    const uint32_t     num_attributes,
                    T*                 d_block_output,
                    uint32_t           attribute_id)
{
    assert(X.get_num_attributes() == Y.get_num_attributes());

    uint32_t p_id = blockIdx.x;
    if (p_id < num_patches) {
        const uint16_t element_per_patch = d_element_per_patch[p_id];
        T              thread_val        = 0;
        for (uint16_t i = threadIdx.x; i < element_per_patch; i += blockSize) {
            if (attribute_id != INVALID32) {
                thread_val +=
                    X(p_id, i, attribute_id) * Y(p_id, i, attribute_id);
            } else {
                for (uint32_t j = 0; j < num_attributes; ++j) {
                    thread_val += X(p_id, i, j) * Y(p_id, i, j);
                }
            }
        }

        cub_block_sum<T, blockSize>(thread_val, d_block_output);
    }
}


template <class T, uint32_t blockSize, typename ReductionOp>
__launch_bounds__(blockSize) __global__
    void generic_reduce(const Attribute<T> X,
                        const uint16_t*    d_element_per_patch,
                        const uint32_t     num_patches,
                        const uint32_t     num_attributes,
                        T*                 d_block_output,
                        ReductionOp        reduction_op,
                        T                  init,
                        uint32_t           attribute_id)
{
    uint32_t p_id = blockIdx.x;
    if (p_id < num_patches) {
        const uint16_t element_per_patch = d_element_per_patch[p_id];
        T              thread_val        = init;
        for (uint16_t i = threadIdx.x; i < element_per_patch; i += blockSize) {
            if (attribute_id != INVALID32) {
                const T val = X(p_id, i, attribute_id);
                thread_val  = reduction_op(thread_val, val);
            } else {
                for (uint32_t j = 0; j < num_attributes; ++j) {
                    const T val = X(p_id, i, j);
                    thread_val  = reduction_op(thread_val, val);
                }
            }
        }
        typedef cub::BlockReduce<T, blockSize>       BlockReduce;
        __shared__ typename BlockReduce::TempStorage temp_storage;

        T block_aggregate =
            BlockReduce(temp_storage).Reduce(thread_val, reduction_op);
        if (threadIdx.x == 0) {
            d_block_output[blockIdx.x] = block_aggregate;
        }
    }
}


template <typename T>
__global__ void memset_attribute(const Attribute<T> attr,
                                 const T            value,
                                 const uint16_t*    d_element_per_patch,
                                 const uint32_t     num_patches,
                                 const uint32_t     num_attributes)
{
    uint32_t p_id = blockIdx.x;
    if (p_id < num_patches) {
        const uint16_t element_per_patch = d_element_per_patch[p_id];
        for (uint16_t i = threadIdx.x; i < element_per_patch; i += blockDim.x) {
            for (uint32_t j = 0; j < num_attributes; ++j) {
                attr(p_id, i, j) = value;
            }
        }
    }
}


template <typename T>
__global__ void memset_spline_attribute(const SplineAttributeBase<T> attr,
                                 const T                      value,
                                 const uint16_t cpoint_num_per_geometry,
                                 const uint32_t num_geometry,
                                 const uint32_t num_attributes)
{
    uint32_t geometry_id = blockIdx.x;

    if (geometry_id < num_geometry)
    {
        for (uint16_t i = threadIdx.x; i < cpoint_num_per_geometry; i += blockDim.x)
        {
            for (uint32_t j = 0; j < num_attributes; ++j)
            {
                attr(geometry_id, i, j) = value;
            }
        }
    }
}

    template <typename T>
    __global__ void memset_spline_attribute(const SplineAttributeGlobal<T> attr,
                                            const T                      value,
                                            const uint16_t cpoint_num_per_geometry,
                                            const uint32_t num_geometry,
                                            const uint32_t num_attributes)
    {
        uint32_t geometry_id = blockIdx.x;

        if (geometry_id < num_geometry)
        {
            for (uint16_t i = threadIdx.x; i < cpoint_num_per_geometry; i += blockDim.x)
            {
                for (uint32_t j = 0; j < num_attributes; ++j)
                {
                    attr(geometry_id, i, j) = value;
                }
            }
        }
    }

    template <typename T>
    __global__ void memset_spline_attribute_global_part(const SplineAttributeGlobal<T> attr,
                                                        const T                      value,
                                                        const uint32_t num_elements)
    {
        size_t tid = threadIdx.x + blockIdx.x * blockDim.x;

        if(tid < num_elements)
        {
            attr.m_d_global_attr[tid] = value;
        }
    }

    template <typename T>
    __global__ void spline_attr_local_to_global(T* global_attr,
                                                T** local_attr,
                                                const SplineAttributeGlobalMappingContext attr_mapping_context,
                                                uint32_t num_cpoints)
    {
        uint32_t geometry_id = blockIdx.x;
        uint16_t cpoint_local_id = threadIdx.x;
        SplineAttributeGlobalMappingInfo& mappingInfo = attr_mapping_context.get_attribute_global_mapping_info(geometry_id);
        assert(geometry_id == mappingInfo.geometry_id);
        uint32_t cpoint_global_id = mappingInfo.ltog_mapping[cpoint_local_id];
        assert(cpoint_global_id < num_cpoints);

        for (uint16_t d = 0; d < 3; ++d)
        {
            ::atomicAdd(&global_attr[cpoint_global_id + d * num_cpoints], local_attr[geometry_id][cpoint_local_id + 64 * d]);
        }
    }

    template <typename T>
    __global__ void spline_attr_local_to_global(const SplineAttributeGlobal<T> attr,
                                                const SplineAttributeGlobalMappingContext attr_mapping_context,
                                                uint32_t num_cpoints)
    {
        uint32_t geometry_id = blockIdx.x;
        uint16_t cpoint_local_id = threadIdx.x;
        SplineAttributeGlobalMappingInfo& mappingInfo = attr_mapping_context.get_attribute_global_mapping_info(geometry_id);
        assert(geometry_id == mappingInfo.geometry_id);
        uint32_t cpoint_global_id = mappingInfo.ltog_mapping[cpoint_local_id];
        assert(cpoint_global_id < num_cpoints);

        for (uint16_t d = 0; d < 3; ++d)
        {
            ::atomicAdd(&attr.m_d_global_attr[cpoint_global_id + d * num_cpoints], attr(geometry_id, cpoint_local_id, d));
        }
    }

    template <typename T>
    __global__ void spline_attr_global_to_local(T* global_attr,
                                                T** local_attr,
                                                const SplineAttributeGlobalMappingContext attr_mapping_context,
                                                uint32_t num_cpoints)
    {
        uint32_t geometry_id = blockIdx.x;
        uint16_t cpoint_local_id = threadIdx.x;
        SplineAttributeGlobalMappingInfo& mappingInfo = attr_mapping_context.get_attribute_global_mapping_info(geometry_id);
        assert(geometry_id == mappingInfo.geometry_id);
        uint32_t cpoint_global_id = mappingInfo.ltog_mapping[cpoint_local_id];
        assert(cpoint_global_id < num_cpoints);

        for (uint16_t d = 0; d < 3; ++d)
        {
            local_attr[geometry_id][cpoint_local_id + 64 * d] = global_attr[cpoint_global_id + d * num_cpoints];
        }
    }

    template <typename T>
    __global__ void spline_attr_add_global_to_local()
    {

    }

    template <typename T, uint32_t blockSize>
    __global__ void squaredNorm_kernel_spline_attribute_global(const SplineAttributeGlobal<T> X,
                          const uint32_t     num_elements,
                          T*                 d_block_output)
    {
        uint32_t p_id = blockIdx.x;
        size_t tid = threadIdx.x + blockIdx.x * blockDim.x;


        T  thread_val  = 0;

        while(tid < num_elements)
        {
            T val = X.m_d_global_attr[tid];
            thread_val += val * val;
            tid += blockDim.x * gridDim.x;
        }
        cub_block_sum<T, blockSize>(thread_val, d_block_output);
    }

    template <typename T, uint32_t blockSize>
    __global__
    void dot_kernel_spline_attribute_global(const SplineAttributeGlobal<T> X,
                                                const SplineAttributeGlobal<T> Y,
                                                const uint32_t     num_elements,
                                                T*                 d_block_output
                                                )
    {
        assert(X.m_num_cpoints == Y.m_num_cpoints);
        size_t tid = threadIdx.x + blockIdx.x * blockDim.x;

        T  thread_val  = 0;

        while(tid < num_elements)
        {
            thread_val += X.m_d_global_attr[tid] * Y.m_d_global_attr[tid];
            tid += blockDim.x * gridDim.x;
        }

        cub_block_sum<T, blockSize>(thread_val, d_block_output);

    }


    template <typename T, uint32_t blockSize>
    __global__ void dot_kernel_spline_attribute_global (const T* x,
                                                          const T* y,
                                                          const uint32_t num_elements,
                                                          T* d_block_output)
    {
        size_t tid = threadIdx.x + blockIdx.x * blockDim.x;


        T  thread_val  = 0;
        while(tid < num_elements)
        {
            thread_val += x[tid] * y[tid];
            tid += blockDim.x * gridDim.x;
        }
        cub_block_sum<T, blockSize>(thread_val, d_block_output);
    }

    template <typename T, uint32_t blockSize>
    __global__
        void dot_kernel_spline_attribute_global_v2(const SplineAttributeGlobal<T> X,
                                           const SplineAttributeGlobal<T> Y,
                                           const uint32_t     num_elements,
                                           T*                 d_block_output
        )
    {
        assert(X.m_num_cpoints == Y.m_num_cpoints);
        __shared__ T cache[blockSize];

        size_t tid = threadIdx.x + blockIdx.x * blockDim.x;
        int cacheIndex = threadIdx.x;

        T thread_val = 0;
        while(tid < num_elements)
        {
            thread_val += X.m_d_global_attr[tid] * Y.m_d_global_attr[tid];
            tid += blockDim.x * gridDim.x;
        }

        cache[cacheIndex] = thread_val;

        __syncthreads();

        int i = blockDim.x/2;
        while (i != 0){
            if (cacheIndex < i){
                cache[cacheIndex] += cache[cacheIndex + i];
            }
            __syncthreads();
            i /= 2;
        }

        if (cacheIndex == 0)
            d_block_output[blockIdx.x] = cache[0];
    }

}    
}    