cmake_minimum_required(VERSION 3.8)

add_definitions(/D_USE_MATH_DEFINES)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
project ("test")
set(iGameCore_module_DIR "C:/Program Files (x86)/OpeniGame/lib/cmake/iGameCore_module")
#set(iGameCore_module_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../OpeniGame/lib/cmake/iGameCore_module")
find_package(iGameCore_module REQUIRED)

file(COPY "${CMAKE_BINARY_DIR}/Resources" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "Models" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
add_executable(testSetRenderWindow "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/RenderWindow/SetRenderWindow.cpp")
add_executable(testSetMultiRenderWindow "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/RenderWindow/SetMultiWindow.cpp")
add_executable(testResetCameraView "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/ResetCameraView.cpp")
add_executable(testSetCameraView "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/SetCameraView.cpp")
add_executable(testSetLineWidth "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/SetLineWidth.cpp")
add_executable(testSetOrthographicProjection "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/SetOrthographicProjection.cpp")
add_executable(testSetPointSize "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/SetPointSize.cpp")
add_executable(testSetTransparency "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/SetTransparency.cpp")
add_executable(testSetViewStyle "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/SetViewStyle.cpp")
add_executable(testSetScalarField "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/SetScalarField.cpp")
add_executable(testSetVolumeRendering "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/SetVolumeRendering.cpp")

add_executable(testChangeRange "${CMAKE_CURRENT_SOURCE_DIR}/Attribute/TestSubDataRangeChangeAndRecollect.cpp")

add_executable(testVector "${CMAKE_CURRENT_SOURCE_DIR}/Filter/Vector/TestVector.cpp")
add_executable(testVectorAllCell "${CMAKE_CURRENT_SOURCE_DIR}/Filter/Vector/TestVectorAllCell.cpp")
add_executable(testVectorCellInRange "${CMAKE_CURRENT_SOURCE_DIR}/Filter/Vector/TestVectorCellInRange.cpp")
add_executable(testVectorEveryNth "${CMAKE_CURRENT_SOURCE_DIR}/Filter/Vector/TestVectorEveryNth.cpp")
add_executable(testVectorSubData "${CMAKE_CURRENT_SOURCE_DIR}/Filter/Vector/TestVectorSubData.cpp")

add_executable(testDecoder "${CMAKE_CURRENT_SOURCE_DIR}/Filter/Compression/TestDecoder.cpp")
add_executable(testEncoder "${CMAKE_CURRENT_SOURCE_DIR}/Filter/Compression/TestEncoder.cpp")
add_executable(testSimplification "${CMAKE_CURRENT_SOURCE_DIR}/Filter/Compression/TestSimplification.cpp")

add_executable(testCGNS "${CMAKE_CURRENT_SOURCE_DIR}/IO/CGNSReader.cpp")
add_executable(testNurbsMesh "${CMAKE_CURRENT_SOURCE_DIR}/IO/NurbsMesh.cpp")
add_executable(testODBReader "${CMAKE_CURRENT_SOURCE_DIR}/IO/ODBReader.cpp")

add_executable(testAnimation "${CMAKE_CURRENT_SOURCE_DIR}/Animation/TestAnimation.cpp")
add_executable(testSaveAnimation "${CMAKE_CURRENT_SOURCE_DIR}/Animation/SaveAnimation.cpp")

add_executable(testClip "${CMAKE_CURRENT_SOURCE_DIR}/Filter/TestClip.cpp")
add_executable(testDeformation "${CMAKE_CURRENT_SOURCE_DIR}/Filter/TestStressDeformationFilter.cpp")
add_executable(testContourLine "${CMAKE_CURRENT_SOURCE_DIR}/Filter/TestContourLine.cpp")

add_executable(testChangeColorRange "${CMAKE_CURRENT_SOURCE_DIR}/ColorBar/ChangeColorRange.cpp")
add_executable(testChangeColorBar "${CMAKE_CURRENT_SOURCE_DIR}/ColorBar/ChangeColorBar.cpp")


# Link libraries for each target
target_link_libraries(testSetRenderWindow PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testSetMultiRenderWindow PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testResetCameraView PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testSetCameraView PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testSetLineWidth PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testSetOrthographicProjection PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testSetPointSize PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testSetTransparency PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testSetViewStyle PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testSetScalarField PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testSetVolumeRendering PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})


target_link_libraries(testChangeRange PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})

target_link_libraries(testVector PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testVectorAllCell PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testVectorCellInRange PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testVectorEveryNth PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testVectorSubData PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})

target_link_libraries(testDecoder PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testEncoder PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testSimplification PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})

target_link_libraries(testAnimation PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testClip PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testDeformation PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testContourLine PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})

target_link_libraries(testSaveAnimation PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testNurbsMesh PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testCGNS PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testODBReader PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})

target_link_libraries(testChangeColorRange PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})
target_link_libraries(testChangeColorBar PUBLIC ${IGAMEVIS_CORE_LIB_TARGETS})

#Use this ThirdParty's Module need to Add Compile Definition
target_compile_definitions(testCGNS PUBLIC CGNS_ENABLE)
target_compile_definitions(testODBReader PUBLIC AbqSDK_ENABLE)
target_compile_definitions(testSaveAnimation PUBLIC FFMPEG_ENABLE)