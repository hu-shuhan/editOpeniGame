cmake_minimum_required(VERSION 3.19)

add_definitions(/D_USE_MATH_DEFINES)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
project("iGameExamples" LANGUAGES CXX)

if (TARGET iGameCore)
    set(_IGAME_EXAMPLE_CORE_LIBS iGameCore)
else ()
    if (NOT iGameCore_DIR)
        set(iGameCore_DIR "${CMAKE_BINARY_DIR}/install/lib/cmake/iGameCore")
    endif ()
    find_package(iGameCore REQUIRED)
    set(_IGAME_EXAMPLE_CORE_LIBS ${IGAMEVIS_CORE_LIB_TARGETS})
endif ()

find_package(HDF5 REQUIRED)
find_package(FFMPEG QUIET)
find_package(AbqSDK QUIET)
find_package(OpenMP QUIET)

# 必须与编译 iGameCore 时一致，否则 Scene 等类型布局不同（如 GL_SUPPORT_MSAA），
# Scene::New 会在运行期直接堆损坏崩溃。裸链 ${IGAMEVIS_CORE_LIB_TARGETS} 不会自动带上这些宏。
add_compile_definitions(
    IGAME_PLATFORM_WINDOWS
    IGAME_OPENGL_VERSION_460
)

if (MSVC)
    add_compile_options(/utf-8)
endif ()

file(COPY "${CMAKE_BINARY_DIR}/Resources" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Models" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

set(IGAME_EXAMPLE_TARGETS "")

function(igame_add_example EXAMPLE_TARGET SOURCE)
    add_executable(${EXAMPLE_TARGET} ${SOURCE})
    set_property(DIRECTORY APPEND PROPERTY IGAME_EXAMPLE_TARGETS ${EXAMPLE_TARGET})
endfunction()

function(igame_link_standard_example EXAMPLE_TARGET)
    target_link_libraries(${EXAMPLE_TARGET} PUBLIC ${_IGAME_EXAMPLE_CORE_LIBS} HDF5::HDF5)
endfunction()

function(igame_register_example_test EXAMPLE_TARGET)
    if (NOT TARGET ${EXAMPLE_TARGET})
        return()
    endif ()

    set(_TEST_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/ci/run_smoke_test.sh")
    set(_TEST_COMMAND bash "${_TEST_SCRIPT}" "$<TARGET_FILE:${EXAMPLE_TARGET}>")

    if ("${EXAMPLE_TARGET}" STREQUAL "testLosslessEncode")
        set(_TEST_COMMAND bash "${_TEST_SCRIPT}" "$<TARGET_FILE:${EXAMPLE_TARGET}>" 120 -- Models/Quad_Plane_Tensor.vtk)
    endif ()

    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_test(
            NAME ${EXAMPLE_TARGET}
            COMMAND xvfb-run -a ${_TEST_COMMAND}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    else ()
        add_test(
            NAME ${EXAMPLE_TARGET}
            COMMAND ${_TEST_COMMAND}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    endif ()

    set_tests_properties(${EXAMPLE_TARGET} PROPERTIES TIMEOUT 180)
endfunction()

# Rendering
igame_add_example(testSetRenderWindow Rendering/RenderWindow/SetRenderWindow.cpp)
igame_add_example(testSetMultiRenderWindow Rendering/RenderWindow/SetMultiWindow.cpp)
igame_add_example(testMeshletRendering Rendering/MeshletRendering.cpp)
igame_add_example(testResetCameraView Rendering/ResetCameraView.cpp)
igame_add_example(testSetCameraView Rendering/SetCameraView.cpp)
igame_add_example(testSetLineWidth Rendering/SetLineWidth.cpp)
igame_add_example(testSetOrthographicProjection Rendering/SetOrthographicProjection.cpp)
igame_add_example(testSetPointSize Rendering/SetPointSize.cpp)
igame_add_example(testSetTransparency Rendering/SetTransparency.cpp)
igame_add_example(testSetViewStyle Rendering/SetViewStyle.cpp)
igame_add_example(testSetScalarField Rendering/SetScalarField.cpp)
igame_add_example(testSetVolumeRendering Rendering/SetVolumeRendering.cpp)
igame_add_example(testSetRenderingPressure Rendering/SetRenderingPressure.cpp)

# Attribute
igame_add_example(testChangeRange Attribute/TestSubDataRangeChangeAndRecollect.cpp)

# Vector / Tensor
igame_add_example(testVector Filter/Vector/TestVector.cpp)
igame_add_example(testTimeVaryingVector Filter/Vector/TestTimeVaryingVector.cpp)
igame_add_example(testVectorAllCell Filter/Vector/TestVectorAllCell.cpp)
igame_add_example(testVectorCellInRange Filter/Vector/TestVectorCellInRange.cpp)
igame_add_example(testVectorEveryNth Filter/Vector/TestVectorEveryNth.cpp)
igame_add_example(testVectorSubData Filter/Vector/TestVectorSubData.cpp)
igame_add_example(testStreamline Filter/Vector/TestStreamline.cpp)
igame_add_example(testTensorView Filter/Tensor/TestTensorView.cpp)

# Visualization data
igame_add_example(testParallelCoordinatesData Filter/VisualizationData/TestParallelCoordinatesData.cpp)
igame_add_example(testVariableCorrelationData Filter/VisualizationData/TestVariableCorrelationData.cpp)
igame_add_example(testVariableDensityData Filter/VisualizationData/TestVariableDensityData.cpp)
igame_add_example(testPlotLineData Filter/VisualizationData/TestPlotLineData.cpp)

# Selection
igame_add_example(testSetSelectionCallBack Filter/Selection/TestSetSelectionCallBack.cpp)
igame_add_example(testSetClearSelectionCallBackFunc Filter/Selection/TestSetClearSelectionCallBackFunc.cpp)
igame_add_example(testSetPointsSelect Filter/Selection/TestSetPointsSelect.cpp)
igame_add_example(testSetCellsSelect Filter/Selection/TestSetCellsSelect.cpp)
igame_add_example(testGetClosestPointsInLine Filter/Selection/TestGetClosestPointsInLine.cpp)
igame_add_example(testGetClosestCellsInLine Filter/Selection/TestGetClosestCellsInLine.cpp)
igame_add_example(testGetPointsInFrustum Filter/Selection/TestGetPointsInFrustum.cpp)
igame_add_example(testGetCellsInFrustum Filter/Selection/TestGetCellsInFrustum.cpp)

# Compression
igame_add_example(testDecoder Filter/Compression/TestDecoder.cpp)
igame_add_example(testEncoder Filter/Compression/TestEncoder.cpp)
igame_add_example(testSimplification Filter/Compression/TestSimplification.cpp)
igame_add_example(testLosslessEncode Filter/Compression/TestLosslessEncode.cpp)

# Convert
igame_add_example(testConvertPolyhedralCells Filter/Convert/TestConvertPolyhedralCells.cpp)
igame_add_example(testConvertToPointCloud Filter/Convert/TestConvertToPointCloud.cpp)
igame_add_example(testConvertToVolumeMesh Filter/Convert/TestConvertToVolumeMesh.cpp)
igame_add_example(testConvertToSurfaceMesh Filter/Convert/TestConvertToSurfaceMesh.cpp)
igame_add_example(testConvertToLagrangeUnstructuredMesh Filter/Convert/TestConvertToLagrangeUnstructuredMesh.cpp)

# IO (optional modules)
igame_add_example(testSplineReaderCPU IO/SplineReaderCPU.cpp)

if (ENABLE_CGNS_MODULE)
    igame_add_example(testCGNS IO/CGNSReader.cpp)
    target_compile_definitions(testCGNS PUBLIC CGNS_ENABLE)
endif ()

if (AbqSDK_FOUND)
    igame_add_example(testODBReader IO/ODBReader.cpp)
    target_compile_definitions(testODBReader PUBLIC AbqSDK_ENABLE)
endif ()

if (ENABLE_GPSCUDA_MODULE)
    igame_add_example(testSplineReaderGPU IO/SplineReaderGPU.cpp)
    target_compile_definitions(testSplineReaderGPU PUBLIC GPSCUDA_ENABLE)
endif ()

if (ENABLE_NASTRAN_MODULE)
    igame_add_example(testNastranReader IO/NastranReader.cpp)
endif ()

igame_add_example(testCASReader IO/CASReader.cpp)

# Animation / Filters
igame_add_example(testAnimation Animation/TestAnimation.cpp)

if (FFMPEG_FOUND)
    igame_add_example(testSaveAnimation Animation/SaveAnimation.cpp)
    target_compile_definitions(testSaveAnimation PUBLIC FFMPEG_ENABLE)
endif ()

igame_add_example(testClip Filter/TestClip.cpp)
igame_add_example(testSlice Filter/TestSlice.cpp)
igame_add_example(testDeformation Filter/Deformation/TestStressDeformationFilter.cpp)
igame_add_example(testDeformationCode Filter/Deformation/TestStressDeformationFilterCode.cpp)
igame_add_example(testContourLine Filter/TestContourLine.cpp)

# Color bar / interaction
igame_add_example(testChangeColorRange ColorBar/ChangeColorRange.cpp)
igame_add_example(testChangeColorBar ColorBar/ChangeColorBar.cpp)
igame_add_example(testMultiscaleInteraction MultiscaleInteraction/TestMultiscaleInteraction.cpp)

# Feature extraction
igame_add_example(testVortexExtraction Filter/FeatureExtraction/VortexExtraction.cpp)
igame_add_example(testCurvatureExtraction Filter/FeatureExtraction/CurvatureExtraction.cpp)
igame_add_example(testGradientExtraction Filter/FeatureExtraction/GradientExtraction.cpp)
igame_add_example(testLaplacianExtraction Filter/FeatureExtraction/LaplacianExtraction.cpp)

if (ENABLE_LIBTORCH_MODULE)
    igame_add_example(testVortexDetection Filter/FeatureExtraction/VortexDetection.cpp)
    target_compile_definitions(testVortexDetection PUBLIC LibTorch_ENABLE)
    target_link_libraries(testVortexDetection PRIVATE ${TORCH_LIBRARIES})
    if (OpenMP_CXX_FOUND)
        target_link_libraries(testVortexDetection PRIVATE OpenMP::OpenMP_CXX)
    endif ()
endif ()

get_property(IGAME_EXAMPLE_TARGETS DIRECTORY PROPERTY IGAME_EXAMPLE_TARGETS)
if (NOT IGAME_EXAMPLE_TARGETS)
    set(IGAME_EXAMPLE_TARGETS "")
endif ()

foreach(EXAMPLE_TARGET IN LISTS IGAME_EXAMPLE_TARGETS)
    if (TARGET ${EXAMPLE_TARGET})
        igame_link_standard_example(${EXAMPLE_TARGET})
    endif ()
endforeach()

enable_testing()
foreach(EXAMPLE_TARGET IN LISTS IGAME_EXAMPLE_TARGETS)
    igame_register_example_test(${EXAMPLE_TARGET})
endforeach()

