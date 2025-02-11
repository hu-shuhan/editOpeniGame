cmake_minimum_required(VERSION 3.8)

add_definitions(/D_USE_MATH_DEFINES)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
project ("test")
#set(iGameCore_module_DIR "C:/Program Files (x86)/OpeniGame/lib/cmake/iGameCore_module")
set(iGameCore_module_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../OpeniGame/lib/cmake/iGameCore_module")
find_package(iGameCore_module REQUIRED)


file(COPY "${CMAKE_BINARY_DIR}/Resources" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "Models" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
add_executable(testSetRenderWindow "${CMAKE_CURRENT_SOURCE_DIR}/Core/Rendering/RenderWindow/SetRenderWindow.cpp")
add_executable(testSetMultiRenderWindow "${CMAKE_CURRENT_SOURCE_DIR}/Core/Rendering/RenderWindow/SetMultiWindow.cpp")
add_executable(testResetCameraView "${CMAKE_CURRENT_SOURCE_DIR}/Core/Rendering/ResetCameraView.cpp")
add_executable(testSetCameraView "${CMAKE_CURRENT_SOURCE_DIR}/Core/Rendering/SetCameraView.cpp")
add_executable(testSetLineWidth "${CMAKE_CURRENT_SOURCE_DIR}/Core/Rendering/SetLineWidth.cpp")
add_executable(testSetOrthographicProjection "${CMAKE_CURRENT_SOURCE_DIR}/Core/Rendering/SetOrthographicProjection.cpp")
add_executable(testSetPointSize "${CMAKE_CURRENT_SOURCE_DIR}/Core/Rendering/SetPointSize.cpp")
add_executable(testSetTransparency "${CMAKE_CURRENT_SOURCE_DIR}/Core/Rendering/SetTransparency.cpp")
add_executable(testSetViewStyle "${CMAKE_CURRENT_SOURCE_DIR}/Core/Rendering/SetViewStyle.cpp")
add_executable(testSetScalarField "${CMAKE_CURRENT_SOURCE_DIR}/Core/Rendering/SetScalarField.cpp")
add_executable(testSetVolumeRendering "${CMAKE_CURRENT_SOURCE_DIR}/Core/Rendering/SetVolumeRendering.cpp")

add_executable(testChangeRange "${CMAKE_CURRENT_SOURCE_DIR}/Core/DataModel/TestSubDataRangeChangeAndRecollect.cpp")

add_executable(testVector "${CMAKE_CURRENT_SOURCE_DIR}/Core/Filter/Vector/TestVector.cpp")
add_executable(testVectorAllCell "${CMAKE_CURRENT_SOURCE_DIR}/Core/Filter/Vector/TestVectorAllCell.cpp")
add_executable(testVectorCellInRange "${CMAKE_CURRENT_SOURCE_DIR}/Core/Filter/Vector/TestVectorCellInRange.cpp")
add_executable(testVectorEveryNth "${CMAKE_CURRENT_SOURCE_DIR}/Core/Filter/Vector/TestVectorEveryNth.cpp")
add_executable(testVectorSubData "${CMAKE_CURRENT_SOURCE_DIR}/Core/Filter/Vector/TestVectorSubData.cpp")

add_executable(testDecoder "${CMAKE_CURRENT_SOURCE_DIR}/Core/Filter/Compression/TestDecoder.cpp")
add_executable(testEncoder "${CMAKE_CURRENT_SOURCE_DIR}/Core/Filter/Compression/TestEncoder.cpp")
add_executable(testSimplification "${CMAKE_CURRENT_SOURCE_DIR}/Core/Filter/Compression/TestSimplification.cpp")

add_executable(testCGNS "${CMAKE_CURRENT_SOURCE_DIR}/Core/TestIO/TestCGNSReader.cpp")
add_executable(testNurbsMesh "${CMAKE_CURRENT_SOURCE_DIR}/Core/TestIO/TestNurbsMesh.cpp")
add_executable(testODBReader "${CMAKE_CURRENT_SOURCE_DIR}/Core/TestIO/TestODBReader.cpp")

add_executable(testAnimation "${CMAKE_CURRENT_SOURCE_DIR}/Core/Animation/TestAnimation.cpp")
add_executable(testSaveAnimation "${CMAKE_CURRENT_SOURCE_DIR}/Core/Animation/SaveAnimation.cpp")

add_executable(testClip "${CMAKE_CURRENT_SOURCE_DIR}/Core/Filter/TestClip.cpp")
add_executable(testDeformation "${CMAKE_CURRENT_SOURCE_DIR}/Core/Filter/TestStressDeformationFilter.cpp")
add_executable(testContourLine "${CMAKE_CURRENT_SOURCE_DIR}/Core/Filter/TestContourLine.cpp")

add_executable(testChangeColorRange "${CMAKE_CURRENT_SOURCE_DIR}/Core/ChangeColorRange.cpp")
add_executable(testChangeColorBar "${CMAKE_CURRENT_SOURCE_DIR}/Core/ChangeColorBar.cpp")


# Link libraries for each target
target_link_libraries(testSetRenderWindow PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testSetMultiRenderWindow PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testResetCameraView PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testSetCameraView PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testSetLineWidth PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testSetOrthographicProjection PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testSetPointSize PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testSetTransparency PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testSetViewStyle PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testSetScalarField PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testSetVolumeRendering PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})


target_link_libraries(testChangeRange PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})

target_link_libraries(testVector PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testVectorAllCell PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testVectorCellInRange PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testVectorEveryNth PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testVectorSubData PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})

target_link_libraries(testDecoder PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testEncoder PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testSimplification PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})

target_link_libraries(testAnimation PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testClip PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testDeformation PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testContourLine PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})

target_link_libraries(testSaveAnimation PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testNurbsMesh PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testCGNS PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testODBReader PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})

target_link_libraries(testChangeColorRange PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})
target_link_libraries(testChangeColorBar PUBLIC ${OPENIGAME_CORE_LIB_TARGETS})

#Use this ThirdParty's Module need to Add Compile Definition
target_compile_definitions(testCGNS PUBLIC CGNS_ENABLE)
target_compile_definitions(testODBReader PUBLIC AbqSDK_ENABLE)
target_compile_definitions(testSaveAnimation PUBLIC FFMPEG_ENABLE)