cmake_minimum_required(VERSION 3.18)
project(GPSpline LANGUAGES CXX CUDA)

# -----------------------
# Basic configuration
# -----------------------
# Enable CUDA separable compilation globally (helps ensure -rdc=true behavior)
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
# Request a reasonably modern C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# If you have CUDA_ARCHS variable defined elsewhere, keep it. Otherwise you can
# set CUDA architectures like: set(CMAKE_CUDA_ARCHITECTURES 70 80 86)
# (or use your own CUDA_ARCHS variable)
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES AND DEFINED CUDA_ARCHS)
	set(CMAKE_CUDA_ARCHITECTURES ${CUDA_ARCHS})
endif()

# Find CUDA toolkit targets (modern CMake)
find_package(CUDAToolkit REQUIRED)     # provides CUDAToolkit::cudart etc.

# -----------------------
# Header-only interface for compile features/paths
# -----------------------
add_library(GPU_header_lib INTERFACE)
target_compile_features(GPU_header_lib INTERFACE cxx_std_17)

# Provide input/output macros as compile definitions (quoted path)
target_compile_definitions(GPU_header_lib
		INTERFACE
		INPUT_DIR=\"${PROJECT_SOURCE_DIR}/input/\"
		OUTPUT_DIR=\"${PROJECT_SOURCE_DIR}/output/\"
)

# Include current dir (headers) and CUDA toolkit include directories
target_include_directories(GPU_header_lib
		INTERFACE
		${CMAKE_CURRENT_LIST_DIR}
		$<$<BOOL:${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}>:${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}>
)

# -----------------------
# Developer compile flags (C++ and CUDA)
# -----------------------
set(cxx_flags
		# You can add C++ specific flags here; example:
		# -fopenmp -O3 -Wall
)

set(cuda_flags
		-Xcompiler=$<$<CXX_COMPILER_ID:GNU>:-Wall -O3>
		-Xcudafe "--display_error_number --diag_suppress=177"
		-rdc=true
		-lineinfo
		--expt-extended-lambda
		--expt-relaxed-constexpr
		-Xptxas -warn-spills -res-usage
		--ptxas-options=-v
		--threads=24
		--use_fast_math
)

# Add debug GPU flag in Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	list(APPEND cuda_flags -G)
endif()

add_library(developer_flags INTERFACE)
target_compile_options(developer_flags INTERFACE
		$<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>
		$<$<COMPILE_LANGUAGE:CUDA>:${cuda_flags}>
)

target_link_libraries(GPU_header_lib INTERFACE developer_flags)

# -----------------------
# Main library (OBJECT + STATIC) to ensure CUDA device registration is preserved
# -----------------------
set(LibraryName GPSplineLib)

# Gather sources
file(GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_LIST_DIR} "*.cpp" "*.cxx" "*.cc")
file(GLOB_RECURSE CUDA_SOURCE_FILES RELATIVE ${CMAKE_CURRENT_LIST_DIR} "*.cu")
file(GLOB_RECURSE INCLUDE_FILES RELATIVE ${CMAKE_CURRENT_LIST_DIR} "*.hpp" "*.h")

set(SOURCE_LIST
		${INCLUDE_FILES}
		${SOURCE_FILES}
		${CUDA_SOURCE_FILES}
)

# Create an OBJECT library first so .cu objects are available to the final link
# This avoids some of the static .a device-link problems.
add_library(${LibraryName}_obj OBJECT ${SOURCE_LIST})

# Apply properties to object target so CUDA separable compilation is active
set_target_properties(${LibraryName}_obj PROPERTIES
		CUDA_SEPARABLE_COMPILATION ON
		POSITION_INDEPENDENT_CODE ON
)

# Ensure developer flags and headers apply
target_link_libraries(${LibraryName}_obj
		PRIVATE
		GPU_header_lib
)

# If needed, expose include dirs for consumers of the final library:
target_include_directories(${LibraryName}_obj
		PRIVATE ${CMAKE_CURRENT_LIST_DIR}
)

# Now make the actual static library from objects
add_library(${LibraryName} STATIC $<TARGET_OBJECTS:${LibraryName}_obj>)

set_target_properties(${LibraryName} PROPERTIES
		FOLDER "apps"
		# When final linking, CMake may need to resolve CUDA device symbols:
		CUDA_RESOLVE_DEVICE_SYMBOLS ON
)

# Propagate compile/link settings to the final library
target_link_libraries(${LibraryName}
		PRIVATE
		GPU_header_lib
		PUBLIC
		Geom
		TBB::tbb
)

# If you want the library to also link the CUDA runtime (optional; final exe can link too)
# target_link_libraries(${LibraryName} PUBLIC CUDAToolkit::cudart)

# Grouping for IDEs
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX "${LibraryName}" FILES ${SOURCE_LIST})

# -----------------------
# Helpful messages
# -----------------------
message(STATUS "GPSpline: building '${LibraryName}'")
message(STATUS "CMAKE_BUILD_TYPE = '${CMAKE_BUILD_TYPE}'")
message(STATUS "CUDA separable compilation = ${CMAKE_CUDA_SEPARABLE_COMPILATION}")
message(STATUS "CMAKE_CUDA_ARCHITECTURES = ${CMAKE_CUDA_ARCHITECTURES}")

# -----------------------
# Notes:
# 1) If your top-level executable (iGameVis) is outside this subproject, ensure the final
#    executable target also has CUDA_SEPARABLE_COMPILATION ON (or was configured by top-level CMake).
#    Example in top-level CMake where you add_executable(iGameVis ...):
#      set_target_properties(iGameVis PROPERTIES CUDA_SEPARABLE_COMPILATION ON CUDA_RESOLVE_DEVICE_SYMBOLS ON)
#
# 2) If you prefer a shared library instead of static, replace:
#      add_library(${LibraryName} STATIC $<TARGET_OBJECTS:${LibraryName}_obj>)
#    with:
#      add_library(${LibraryName} SHARED $<TARGET_OBJECTS:${LibraryName}_obj>)
#
# 3) After changing CMake, do a clean build:
#      rm -rf build && mkdir build && cd build && cmake .. && ninja
#
# 4) If you still get `__cudaRegisterLinkedBinary` undefined errors at final link:
#      - Confirm the top-level final target is linking against this GPSplineLib target
#      - Confirm top-level target has CUDA symbols resolved (see note 1)
#
# -----------------------
