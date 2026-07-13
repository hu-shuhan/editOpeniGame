//#pragma once
//
//#include <Geom/CBSplineSurface.h>
//#include "Util/GP_Macros.h"
//#include "Util/GPSplineDefine.h"
//
//#include "cutlass/cutlass.h"
//#include "cutlass/gemm/device/gemm.h"
//#include "cutlass/util/host_tensor.h"
//#include "cutlass/util/reference/device/gemm.h"
//
//GPSTART
//
//
//    class GPSplineSurfaceCUTLASS {
//
//        using real_t = float;
//
//        using ElementAccumulator = real_t;                   // <- data type of accumulator
//        using ElementComputeEpilogue = ElementAccumulator;  // <- data type of epilogue operations
//        using ElementInputA = real_t;                        // <- data type of elements in input matrix A
//        using ElementInputB = real_t;                        // <- data type of elements in input matrix B
//        using ElementOutput = real_t;                        // <- data type of elements in output matrix D
//
//        // The code section below describes matrix layout of input and output matrices.
//        using LayoutInputA = cutlass::layout::RowMajor;
//        using LayoutInputB = cutlass::layout::ColumnMajor;
//        using LayoutOutput = cutlass::layout::RowMajor;
//
//        // This code section describes whether you want to use tensor cores or regular SIMT cores on GPU SM
//        using MMAOp = cutlass::arch::OpClassTensorOp;
//
//        // This code section describes CUDA SM architecture number
//        using SmArch = cutlass::arch::Sm80;
//
//        // This code section describes the tile size a thread block will compute
//        using ShapeMMAThreadBlock =
//                cutlass::gemm::GemmShape<128, 64, 32>;  // <- threadblock tile M = 128, N = 128, K = 16
//        // This code section describes tile size a warp will compute
//        using ShapeMMAWarp = cutlass::gemm::GemmShape<64, 32, 16>;  // <- warp tile M = 64, N = 64, K = 16
//        // This code section describes the size of MMA op
//        using ShapeMMAOp = cutlass::gemm::GemmShape<16, 8, 8>;  // <- MMA Op tile M = 16, N = 8, K = 8
//
//        // This code section describes how threadblocks are scheduled on GPU
//        using SwizzleThreadBlock = cutlass::gemm::threadblock::GemmIdentityThreadblockSwizzle<>;  // <- ??
//
//        // This code section describes the epilogue part of the kernel
//        using EpilogueOp = cutlass::epilogue::thread::LinearCombination<
//                ElementOutput,                                     // <- data type of output matrix
//                128 / cutlass::sizeof_bits<ElementOutput>::value,  // <- the number of elements per vectorized
//                // memory access. For a byte, it's 16
//                // elements. This becomes the vector width of
//                // math instructions in the epilogue too
//                ElementAccumulator,                                // <- data type of accumulator
//                ElementComputeEpilogue>;  // <- data type for alpha/beta in linear combination function
//
//        // Number of pipelines you want to use
//        constexpr static int NumStages = 3;
//
//        using Gemm = cutlass::gemm::device::Gemm<ElementInputA,
//                LayoutInputA,
//                ElementInputB,
//                LayoutInputB,
//                ElementOutput,
//                LayoutOutput,
//                ElementAccumulator,
//                MMAOp,
//                SmArch,
//                ShapeMMAThreadBlock,
//                ShapeMMAWarp,
//                ShapeMMAOp,
//                EpilogueOp,
//                SwizzleThreadBlock,
//                NumStages>;
//
//    public:
//
//        void init(CBSplineSurface& surface);
//
//        virtual void init_knot_vector(CBSplineSurface& surface);
//
//        virtual void init_cpoints(CBSplineSurface& surface);
//
//        virtual void init_memory_pools(CBSplineSurface& surface);
//
//        void release();
//
//        virtual void release_knot_vector();
//
//        virtual void release_cpoints();
//
//        virtual void release_memory_pools();
//
//        virtual void build_tessellation(uint16_t  new_p, uint16_t new_q);
//
//        void build_tessellation_cpu(uint16_t new_p, uint16_t new_q);
//
//        void compare_cpu_and_gpu_data();
//
//    public:
//
//        //1. Knot Vector
//        // u knot vector
//        real_t *device_uKnots = nullptr;
//        // v knot vector
//        real_t *device_vKnots = nullptr;
//
//        // CPU mappings
//        real_t *host_uKnots = nullptr;
//        real_t *host_vKnots = nullptr;
//
//        //2. Matrix H
//        // matrix Hn : 4x(7p+1)
//        real_t *device_memPool_H_p = nullptr;
//        // matrix Hm : 4x(7q+1)
//        real_t *device_memPool_H_q = nullptr;
//
//        //matrix H'n : 4(7p+1)
//        real_t *device_memPool_H_prime_p = nullptr;
//        real_t *device_memPool_H_dv_p = nullptr;
//
//        //matrix H'm : 4(7p+1)
//        real_t *device_memPool_H_du_q = nullptr;
//        real_t *device_memPool_H_prime_q = nullptr;
//
//        //CPU mappings
//        real_t *host_memPool_H_p = nullptr;
//        real_t *host_memPool_H_q = nullptr;
//
//        real_t *host_memPool_H_prime_p = nullptr;
//        real_t *host_memPool_H_prime_q = nullptr;
//
//        //Matrix P
//        //Control Points
//        // matrix Px : 4x4
//        real_t *device_cpoints_x = nullptr;
//        // matrix Py : 4x4
//        real_t *device_cpoints_y = nullptr;
//        // matrix Pz : 4x4
//        real_t *device_cpoints_z = nullptr;
//
//        //CPU mappings
//        real_t *host_cpoints_x = nullptr;
//        real_t *host_cpoints_y = nullptr;
//        real_t *host_cpoints_z = nullptr;
//
//        //Matrix S
//        // Result Surface Data
//        // matrix Sx : (7p+1)x(7q+1)
//        real_t *device_memPool_result_x = nullptr;
//        // matrix Sy : (7p+1)x(7q+1)
//        real_t *device_memPool_result_y = nullptr;
//        // matrix Sz : (7p+1)x(7q+1)
//        real_t *device_memPool_result_z = nullptr;
//
//        //CPU mappings
//        real_t *host_memPool_result_x = nullptr;
//        real_t *host_memPool_result_y = nullptr;
//        real_t *host_memPool_result_z = nullptr;
//
//        //P Q
//        //p, q data
//        const uint16_t MAX_P = _M_MAX_P;
//        const uint16_t MAX_Q = _M_MAX_Q;
//
//        uint16_t old_p = INVALID16;
//        uint16_t old_q = INVALID16;
//
//        uint16_t cur_p = INVALID16;
//        uint16_t cur_q = INVALID16;
//
//
//        // U V
//        const uint16_t uDegree = 3;
//        const uint16_t vDegree = 3;
//
//        const uint16_t uSize = 8;
//        const uint16_t vSize = 8;
//
//    };
//
//GPEND