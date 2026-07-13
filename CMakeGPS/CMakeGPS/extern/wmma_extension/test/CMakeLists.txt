set(cxx_flags
        $<$<CXX_COMPILER_ID:MSVC>:-D_SCL_SECURE_NO_WARNINGS /openmp /std:c++17>           #Add MSVC-specific compiler flags here
        #	$<$<CXX_COMPILER_ID:GNU>: -fopenmp -std=c++17> #Add GCC/Clang-specific compiler flags here
        $<$<CXX_COMPILER_ID:GNU>:-Wall -m64 -fopenmp -O3 -std=c++17 -Wno-unused-function> #Add GCC/Clang-specific compiler flags here
)
set(cuda_flags
        -Xcompiler=$<$<CXX_COMPILER_ID:GNU>:-Wall -O3>
        -Xcompiler=$<$<CXX_COMPILER_ID:MSVC>:${OpenMP_CXX_FLAGS}>
        #Disables warning
        #177-D "function XXX was declared but never referenced"
        -Xcudafe "--display_error_number --diag_suppress=177"
        ${CUDA_ARCHS}
        -rdc=true
        -lineinfo
        --expt-extended-lambda
        --expt-relaxed-constexpr
        -Xptxas -warn-spills -res-usage
        --ptxas-options=-v
        --threads=24
        -G
)

add_library(wmma_language_flags INTERFACE)
target_compile_options(wmma_language_flags INTERFACE
        $<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>
        $<$<COMPILE_LANGUAGE:CUDA>:${cuda_flags}>
)

add_subdirectory(primitive)
add_subdirectory(tcec)