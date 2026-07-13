cmake_minimum_required(VERSION 3.5)

get_filename_component(PROJNAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
Project(${PROJNAME}
        VERSION 0.0.1
        LANGUAGES C CXX CUDA)

Message(STATUS "-------------------------------")
Message(STATUS "Processing Project ${PROJNAME}:")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)

set(CMAKE_CUDA_ARCHITECTURES "86")

set(CMAKE_CUDA_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set( GS_EXENAME "gpspline" )

#####################################################################################

include_directories(extern)
include_directories(./)
include_directories(extern/wmma_extension/include)
include_directories(extern/cutlass/include)
include_directories(extern/cutlass/tools/util/include)
include_directories(extern/oneTBB-2021.10.0/include)
include_directories(extern/glm)
add_subdirectory(extern)
add_subdirectory(GPHelperIO)
add_subdirectory(GPSpline)

#####################################################################################

#file(GLOB ALL_SOURCE_FILES *.cpp *.hpp *.h *.c)
#add_executable(${GS_EXENAME}  ${ALL_SOURCE_FILES} )
#target_compile_definitions(${GS_EXENAME} PRIVATE VALUE=1)

#target_link_libraries(${GS_EXENAME} CommonLib helper_opencascade Geom GPHelperIO GPSplineLib)
#file(GLOB_RECURSE EXDLL ${PROJECT_SOURCE_DIR}/ReferenceDLL/*.dll)

#add_custom_command(TARGET GPSplineLib POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different
#        ${EXDLL}
#        $<TARGET_FILE_DIR:${PROJECT_NAME}>
#)