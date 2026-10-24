cmake_minimum_required(VERSION 3.19)

add_definitions(/D_USE_MATH_DEFINES)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
project("iGameExamples" LANGUAGES CXX)

if (TARGET iGameCore)
    set(_IGAME_EXAMPLE_CORE_LIBS iGameCore)
else ()
    if (NOT iGameCore_DIR)
        set(_IGAME_DEFAULT_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cmake-build-release/install/lib/cmake/iGameCore")
        if (EXISTS "${_IGAME_DEFAULT_CORE_DIR}/iGameCoreConfig.cmake")
            set(iGameCore_DIR "${_IGAME_DEFAULT_CORE_DIR}" CACHE PATH "Path to iGameCore package config" FORCE)
        else ()
            set(iGameCore_DIR "${CMAKE_BINARY_DIR}/install/lib/cmake/iGameCore" CACHE PATH "Path to iGameCore package config" FORCE)
        endif ()
    endif ()
    find_package(iGameCore REQUIRED)
    set(_IGAME_EXAMPLE_CORE_LIBS ${IGAMEVIS_CORE_LIB_TARGETS})
endif ()

# HDF5：与 iGameCore 的 IGV 分支用**同一套跨平台查找**（config 模式优先，
# Module 模式回退），不额外设任何选型变量 —— 让 CMake/HDF5 的包文件自行决定。
# 若在此处强设变体（如 HDF5_USE_STATIC_LIBRARIES），可能选出与 iGameCore 不同的
# 静态/共享组合，造成混链。
find_package(HDF5 REQUIRED)
find_package(FFMPEG QUIET)
find_package(AbqSDK QUIET)
find_package(OpenMP QUIET)

if (WIN32)
    set(_IGAME_EXAMPLE_ABQ_RUNTIME_NAMES "")
    if (AbqSDK_FOUND AND ABQ_RUNTIME_LIB_LIST)
        string(JOIN "," _IGAME_EXAMPLE_ABQ_RUNTIME_NAMES
                ${ABQ_RUNTIME_LIB_LIST})
    elseif (AbqSDK_FOUND AND ABQ_LIB_LIST)
        string(JOIN "," _IGAME_EXAMPLE_ABQ_RUNTIME_NAMES
                ${ABQ_LIB_LIST})
    endif ()

    # HDF5 运行库（Windows 需要把 DLL 放到 exe 旁；其它平台为空即可）。
    #
    # 用 `$<TARGET_FILE:...>` 而不是拼路径 —— 这样不假定任何安装布局，
    # 换 Linux/macOS 或换 HDF5 发行版都不用改这里。
    # 目标名按"不同来源暴露的名字不同"逐个探测；静态库**不需要**部署（因此跳过）。
    #
    # 关于 TYPE：config 模式下目标名是 `hdf5::hdf5-shared`（SHARED_LIBRARY）；
    # Module 模式下是 `HDF5::HDF5`，其 TYPE 为 UNKNOWN_LIBRARY（导入库无法判定
    # 静态/动态）。两者都接受 —— 真正的判据是"这个目标有没有可拷贝的
    # runtime 文件"，由 DeployExampleRuntime.cmake 用 EXISTS 兜住，比这里
    # 猜目标类型更可靠。
    set(_IGAME_EXAMPLE_HDF5_RUNTIME "")
    set(_IGAME_HDF5_CANDIDATES hdf5::hdf5-shared HDF5::HDF5)
    foreach (_cand IN LISTS _IGAME_HDF5_CANDIDATES)
        if (TARGET ${_cand})
            get_target_property(_candType ${_cand} TYPE)
            if (_candType MATCHES "SHARED_LIBRARY|UNKNOWN_LIBRARY")
                set(_IGAME_EXAMPLE_HDF5_RUNTIME "$<TARGET_FILE:${_cand}>")
                break()
            endif ()
        endif ()
    endforeach ()
    unset(_cand)
    unset(_candType)

    # IGAME_CONFIG 用 $<CONFIG> 传入：多配置生成器（VS/Xcode）下 exe 在
    # <binary>/Examples/<Config>/，DLL 必须落到同一个目录，否则 exe 启动即
    # 0xC0000135（P1 实测过，见 DeployExampleRuntime.cmake 的说明）。
    add_custom_target(iGameDeployExampleRuntime
        COMMAND "${CMAKE_COMMAND}"
            "-DIGAME_RUNTIME_DESTINATION=${CMAKE_CURRENT_BINARY_DIR}"
            "-DIGAME_CONFIG=$<CONFIG>"
            "-DIGAME_RESOURCES_SOURCE=${CMAKE_CURRENT_BINARY_DIR}/Resources"
            "-DIGAME_TORCH_RUNTIME_DIR=${LIBTORCH_DIR}/lib"
            "-DIGAME_TORCH_BIN_DIR=${LIBTORCH_DIR}/bin"
            "-DIGAME_FFMPEG_RUNTIME_DIR=${FFMPEG_DLL_DIR}"
            "-DIGAME_ABQ_RUNTIME_DIR=${AbqSDK_DLL_DIR}"
            "-DIGAME_ABQ_RUNTIME_LIBRARY_NAMES=${_IGAME_EXAMPLE_ABQ_RUNTIME_NAMES}"
            "-DIGAME_HDF5_RUNTIME_FILE=${_IGAME_EXAMPLE_HDF5_RUNTIME}"
            "-DIGAME_GPS_RUNTIME_DIR=${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/ReferenceDLL"
            -P "${CMAKE_SOURCE_DIR}/Cmake/DeployExampleRuntime.cmake"
        COMMENT "Deploying runtime DLLs for Examples"
        VERBATIM)
else ()
    add_custom_target(iGameDeployExampleRuntime)
endif ()

# 必须与编译 iGameCore 时一致，否则 Scene 等类型布局不同（如 GL_SUPPORT_MSAA），
# Scene::New 会在运行期直接堆损坏崩溃。裸链 ${IGAMEVIS_CORE_LIB_TARGETS} 不会自动带上这些宏。
add_compile_definitions(
    IGAME_PLATFORM_WINDOWS
    $<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS>
    IGAME_OPENGL_VERSION_460
)

if (MSVC)
    add_compile_options(/utf-8)
endif ()

set(_IGAME_EXAMPLE_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Resources")
set(_IGAME_EXAMPLE_ASSET_COPY_COMMANDS "")
if (EXISTS "${_IGAME_EXAMPLE_RESOURCES_DIR}")
    list(APPEND _IGAME_EXAMPLE_ASSET_COPY_COMMANDS
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${_IGAME_EXAMPLE_RESOURCES_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/Resources"
    )
endif ()

set(_IGAME_EXAMPLE_RENDERING_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../iGameCore/Rendering")
if (EXISTS "${_IGAME_EXAMPLE_RENDERING_DIR}/Shaders/GLSL")
    list(APPEND _IGAME_EXAMPLE_ASSET_COPY_COMMANDS
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${_IGAME_EXAMPLE_RENDERING_DIR}/Shaders/GLSL"
            "${CMAKE_CURRENT_BINARY_DIR}/Resources/Shaders"
    )
endif ()
if (EXISTS "${_IGAME_EXAMPLE_RENDERING_DIR}/Assests")
    list(APPEND _IGAME_EXAMPLE_ASSET_COPY_COMMANDS
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${_IGAME_EXAMPLE_RENDERING_DIR}/Assests"
            "${CMAKE_CURRENT_BINARY_DIR}/Resources/Assests"
    )
endif ()

set(_IGAME_EXAMPLE_PYTHON_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Python")
if (EXISTS "${_IGAME_EXAMPLE_PYTHON_DIR}/pyFluentLib/cas_converter.exe")
    list(APPEND _IGAME_EXAMPLE_ASSET_COPY_COMMANDS
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/Resources/pyFluentLib"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${_IGAME_EXAMPLE_PYTHON_DIR}/pyFluentLib/cas_converter.exe"
            "${CMAKE_CURRENT_BINARY_DIR}/Resources/pyFluentLib/cas_converter.exe"
    )
endif ()
if (EXISTS "${_IGAME_EXAMPLE_PYTHON_DIR}/pyNastranLib/nastran_to_vtk_cli.exe")
    list(APPEND _IGAME_EXAMPLE_ASSET_COPY_COMMANDS
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/Resources/pyNastranLib"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${_IGAME_EXAMPLE_PYTHON_DIR}/pyNastranLib/nastran_to_vtk_cli.exe"
            "${CMAKE_CURRENT_BINARY_DIR}/Resources/pyNastranLib/nastran_to_vtk_cli.exe"
    )
endif ()
if (EXISTS "${_IGAME_EXAMPLE_PYTHON_DIR}/pyCCMLib/ccm_to_vtu_converter.exe")
    list(APPEND _IGAME_EXAMPLE_ASSET_COPY_COMMANDS
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/Resources/pyCCMLib"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${_IGAME_EXAMPLE_PYTHON_DIR}/pyCCMLib/ccm_to_vtu_converter.exe"
            "${CMAKE_CURRENT_BINARY_DIR}/Resources/pyCCMLib/ccm_to_vtu_converter.exe"
    )
endif ()
if (EXISTS "${_IGAME_EXAMPLE_PYTHON_DIR}/pyAnsysLib/ansys_to_pvd_converter.exe")
    list(APPEND _IGAME_EXAMPLE_ASSET_COPY_COMMANDS
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/Resources/pyAnsysLib"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${_IGAME_EXAMPLE_PYTHON_DIR}/pyAnsysLib/ansys_to_pvd_converter.exe"
            "${CMAKE_CURRENT_BINARY_DIR}/Resources/pyAnsysLib/ansys_to_pvd_converter.exe"
    )
endif ()
if (EXISTS "${_IGAME_EXAMPLE_PYTHON_DIR}/pyLsDynaLib/lsdyna_to_pvd_converter.exe")
    list(APPEND _IGAME_EXAMPLE_ASSET_COPY_COMMANDS
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/Resources/pyLsDynaLib"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${_IGAME_EXAMPLE_PYTHON_DIR}/pyLsDynaLib/lsdyna_to_pvd_converter.exe"
            "${CMAKE_CURRENT_BINARY_DIR}/Resources/pyLsDynaLib/lsdyna_to_pvd_converter.exe"
    )
endif ()
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Models")
    list(APPEND _IGAME_EXAMPLE_ASSET_COPY_COMMANDS
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Models"
            "${CMAKE_CURRENT_BINARY_DIR}/Models"
    )
endif ()
add_custom_target(iGameCopyExampleAssets ${_IGAME_EXAMPLE_ASSET_COPY_COMMANDS})

set(IGAME_EXAMPLE_TARGETS "")

function(igame_add_example EXAMPLE_TARGET SOURCE)
    add_executable(${EXAMPLE_TARGET} ${SOURCE})
    add_dependencies(${EXAMPLE_TARGET}
        iGameCopyExampleAssets
        iGameDeployExampleRuntime)
    set_property(DIRECTORY APPEND PROPERTY IGAME_EXAMPLE_TARGETS ${EXAMPLE_TARGET})
endfunction()

function(igame_link_standard_example EXAMPLE_TARGET)
    target_link_libraries(${EXAMPLE_TARGET} PUBLIC ${_IGAME_EXAMPLE_CORE_LIBS} HDF5::HDF5)
endfunction()

set(IGAME_BATCH3_EXAMPLES
    testThirdBatchMenu
    testThirdBatchData
    testAngularPeriodic
    testAngularPeriodicSelfCheck
    testAppendLocationAttributeFilter
    testBoundaryMeshQuality
    testCleanToGridFilter
    testCountCellFaces
    testCountCellFacesOutput
    testFeatureEdgeRegion
    testThresholdAndGenerateIds
    testMeshQuality
    testPointLineInterpolator
    testPointSetToOctree
    testResampleOctreeChecks
    testPointVolumeInterpolator
    testPointVolumeInterpolatorSelfCheck
    testShrink
    testShrinkModel
    testSurfaceNormalsFilter
    testVolumeMeshSimplification
    testMeshTetrahedralize
)

set(IGAME_BATCH2_EXAMPLES
    testCellSizeExtraction
    testCountCellVertices
    testExtractEdges
    testGenerateGlobalIds
    testPointAndCellIds
    testGenerateProcessIds
    testAxisAlignedReflection
    testExtractComponent
    testMergeVectorComponents
    testResampleToImage
    testResampleToLine
    testTriangleStrip
    testResampleImageChecks
    testTriangleStripWidget
    testBatch2GeometryValidation
    testStandardFiltersMenu
)
set(IGAME_BATCH2_VISUAL_EXAMPLES
    testCountCellVertices
    testExtractEdges
    testAxisAlignedReflection
    testResampleToImage
    testResampleToLine
)

# Examples for the filters integrated in e7ec6571.
set(IGAME_BATCH1_EXAMPLES
    testPointCoordinates testCellCenterFilter testRandomVectors
    testRemoveGhostInformation testElevation testMaskPoints
    testFeatureEdges testFeatureEdgesVisualization testExtractSubset
    testOutlineCorners testProbe testConvertToVertex
)

function(igame_register_example_test EXAMPLE_TARGET)
    if (NOT TARGET ${EXAMPLE_TARGET})
        return()
    endif ()

    if (EXAMPLE_TARGET IN_LIST IGAME_BATCH1_EXAMPLES)
        set(_BATCH1_ARGS)
        if ("${EXAMPLE_TARGET}" STREQUAL "testRandomVectors")
            list(APPEND _BATCH1_ARGS Models/CellCenter_hexa_grid.vtk)
        elseif ("${EXAMPLE_TARGET}" STREQUAL "testFeatureEdges" OR
                "${EXAMPLE_TARGET}" STREQUAL "testFeatureEdgesVisualization")
            list(APPEND _BATCH1_ARGS Models/mazewheel.obj)
        endif ()
        if ("${EXAMPLE_TARGET}" STREQUAL "testCellCenterFilter" OR
            "${EXAMPLE_TARGET}" STREQUAL "testFeatureEdgesVisualization" OR
            "${EXAMPLE_TARGET}" STREQUAL "testExtractSubset" OR
            "${EXAMPLE_TARGET}" STREQUAL "testConvertToVertex")
            list(APPEND _BATCH1_ARGS --no-render)
        endif ()
        add_test(NAME ${EXAMPLE_TARGET}
            COMMAND "$<TARGET_FILE:${EXAMPLE_TARGET}>" ${_BATCH1_ARGS}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
        set_tests_properties(${EXAMPLE_TARGET} PROPERTIES TIMEOUT 180 LABELS "batch1-filters")
        if ("${EXAMPLE_TARGET}" STREQUAL "testRandomVectors")
            add_test(NAME testRandomVectorsMixedCells
                COMMAND "$<TARGET_FILE:testRandomVectors>" Models/CellCenter_surface_mixed.vtk
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
            set_tests_properties(testRandomVectorsMixedCells PROPERTIES TIMEOUT 180 LABELS "batch1-filters")
        endif ()
        return()
    endif ()

    if (EXAMPLE_TARGET IN_LIST IGAME_BATCH3_EXAMPLES)
        set(_BATCH3_ENV "IGAME_EXAMPLE_NO_RENDER=1")
        if ("${EXAMPLE_TARGET}" STREQUAL "testThirdBatchMenu")
            get_target_property(_BATCH3_QMAKE Qt5::qmake IMPORTED_LOCATION)
            get_filename_component(_BATCH3_QT_BIN "${_BATCH3_QMAKE}" DIRECTORY)
            list(APPEND _BATCH3_ENV "QT_QPA_PLATFORM=offscreen"
                "QT_QPA_PLATFORM_PLUGIN_PATH=${_BATCH3_QT_BIN}/../plugins/platforms")
            if (WIN32)
                string(REPLACE ";" "\;" _BATCH3_PATH "${CMAKE_BINARY_DIR};${_BATCH3_QT_BIN};$ENV{PATH}")
                list(APPEND _BATCH3_ENV "PATH=${_BATCH3_PATH}")
            endif ()
        endif ()
        add_test(NAME ${EXAMPLE_TARGET}
            COMMAND "${CMAKE_COMMAND}" -E env ${_BATCH3_ENV}
                "$<TARGET_FILE:${EXAMPLE_TARGET}>"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
        set_tests_properties(${EXAMPLE_TARGET} PROPERTIES TIMEOUT 180 LABELS "batch3-filters")
        return()
    endif ()

    if (EXAMPLE_TARGET IN_LIST IGAME_BATCH2_EXAMPLES)
        set(_BATCH2_ARGS)
        set(_BATCH2_ENV)
        if (EXAMPLE_TARGET IN_LIST IGAME_BATCH2_VISUAL_EXAMPLES)
            list(APPEND _BATCH2_ARGS --no-render)
        endif ()
        if ("${EXAMPLE_TARGET}" STREQUAL "testTriangleStripWidget" OR
            "${EXAMPLE_TARGET}" STREQUAL "testStandardFiltersMenu")
            get_target_property(_BATCH2_QMAKE Qt5::qmake IMPORTED_LOCATION)
            get_filename_component(_BATCH2_QT_BIN "${_BATCH2_QMAKE}" DIRECTORY)
            list(APPEND _BATCH2_ENV "QT_QPA_PLATFORM=offscreen"
                "QT_QPA_PLATFORM_PLUGIN_PATH=${_BATCH2_QT_BIN}/../plugins/platforms")
            if (WIN32)
                string(REPLACE ";" "\;" _BATCH2_PATH "${CMAKE_BINARY_DIR};${_BATCH2_QT_BIN};$ENV{PATH}")
                list(APPEND _BATCH2_ENV "PATH=${_BATCH2_PATH}")
            endif ()
        endif ()
        add_test(NAME ${EXAMPLE_TARGET}
            COMMAND "${CMAKE_COMMAND}" -E env ${_BATCH2_ENV}
                "$<TARGET_FILE:${EXAMPLE_TARGET}>" ${_BATCH2_ARGS}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
        set_tests_properties(${EXAMPLE_TARGET} PROPERTIES TIMEOUT 180 LABELS "batch2-filters")
        return()
    endif ()

    if ("${EXAMPLE_TARGET}" STREQUAL "testModelTreeViewStyleValidation" OR
        "${EXAMPLE_TARGET}" STREQUAL "testAnimationVertexValidation")
        get_target_property(_VIEW_QMAKE Qt5::qmake IMPORTED_LOCATION)
        get_filename_component(_VIEW_QT_BIN "${_VIEW_QMAKE}" DIRECTORY)
        set(_VIEW_ENV)
        if (WIN32)
            # The animation test has its own directory to isolate recent-file settings.
            string(REPLACE ";" "\\;" _VIEW_PATH "$<TARGET_FILE_DIR:iGameVis>;${_VIEW_QT_BIN};$ENV{PATH}")
            list(APPEND _VIEW_ENV "PATH=${_VIEW_PATH}")
        endif ()
        add_test(NAME ${EXAMPLE_TARGET}
            COMMAND "${CMAKE_COMMAND}" -E env
                "QT_QPA_PLATFORM=offscreen"
                "QT_QPA_PLATFORM_PLUGIN_PATH=${_VIEW_QT_BIN}/../plugins/platforms"
                ${_VIEW_ENV}
                "$<TARGET_FILE:${EXAMPLE_TARGET}>"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
        set_tests_properties(${EXAMPLE_TARGET} PROPERTIES TIMEOUT 120)
        return()
    endif ()

    if (WIN32)
        set(_TEST_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/ci/run_smoke_test.ps1")
        set(_TEST_COMMAND powershell -NoProfile -NonInteractive
            -ExecutionPolicy Bypass -File "${_TEST_SCRIPT}"
            -Executable "$<TARGET_FILE:${EXAMPLE_TARGET}>")
        set(_TEST_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

        if ("${EXAMPLE_TARGET}" STREQUAL "testModelGeometryParallelValidation" OR
            "${EXAMPLE_TARGET}" STREQUAL "testVTKXMLDataArrayDecoderValidation" OR
            "${EXAMPLE_TARGET}" STREQUAL "testSpectralReaderCPU" OR
            "${EXAMPLE_TARGET}" STREQUAL "testVertexReaderValidation" OR
            "${EXAMPLE_TARGET}" STREQUAL "testZeroCellPointCloudValidation" OR
            "${EXAMPLE_TARGET}" STREQUAL "testPolylineWireframeValidation" OR
            "${EXAMPLE_TARGET}" STREQUAL "testIGVRoundTrip" OR
            "${EXAMPLE_TARGET}" STREQUAL "testIGVFileIO" OR
            "${EXAMPLE_TARGET}" STREQUAL "testIGVIGCConvert" OR
            "${EXAMPLE_TARGET}" STREQUAL "testIGVLargeFile" OR
            "${EXAMPLE_TARGET}" STREQUAL "testAnimationExportRangeValidation")
            list(APPEND _TEST_COMMAND -TimeoutSeconds 180 -RequireExit)
            set(_TEST_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${EXAMPLE_TARGET}>")
        elseif ("${EXAMPLE_TARGET}" STREQUAL "igvDump")
            # igvDump 是工具：不带参数时打印用法并返回 2，所以 CTest 走它自己的自检模式。
            list(APPEND _TEST_COMMAND -TimeoutSeconds 120 -RequireExit -Arguments "--self-check")
            set(_TEST_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${EXAMPLE_TARGET}>")
        elseif ("${EXAMPLE_TARGET}" STREQUAL "testIGVRobustness")
            # 这个目标要跑 1000 次变异 fuzz（每次一遍 HDF5 打开 + 解析）。
            # 实测：直接运行约 22 s；经 run_smoke_test.ps1（Start-Process，输出到隐藏控制台）
            # 在磁盘忙时可到约 104 s —— 控制台写入与磁盘 I/O 都是同步的。CI 机器更慢，
            # 所以进程级超时给到 600 s（下面的 TIMEOUT 也同步放大）。
            # 注意：它**不能**放进上面那个 OR 列表 —— 那里统一是 180 s。
            list(APPEND _TEST_COMMAND -TimeoutSeconds 600 -RequireExit)
            set(_TEST_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${EXAMPLE_TARGET}>")
        elseif ("${EXAMPLE_TARGET}" STREQUAL "testLosslessEncode")
            list(APPEND _TEST_COMMAND -TimeoutSeconds 120
                -Arguments "Models/Quad_Plane_Tensor.vtk")
        endif ()
        if ("${EXAMPLE_TARGET}" STREQUAL "testAnimationExportRangeValidation")
            # Shaders/fonts are deployed in the Examples build directory.
            set(_TEST_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
        endif ()

        add_test(
            NAME ${EXAMPLE_TARGET}
            COMMAND ${_TEST_COMMAND}
            WORKING_DIRECTORY "${_TEST_WORKING_DIRECTORY}"
        )
        set_tests_properties(${EXAMPLE_TARGET} PROPERTIES TIMEOUT 200)
        if ("${EXAMPLE_TARGET}" STREQUAL "testIGVRobustness")
            # 1000 次变异 fuzz：见上面 Windows 分支里对 600 s 的说明。
            set_tests_properties(${EXAMPLE_TARGET} PROPERTIES TIMEOUT 600)
        endif ()
        if ("${EXAMPLE_TARGET}" STREQUAL "testAnimationExportRangeValidation")
            set_tests_properties(${EXAMPLE_TARGET} PROPERTIES SKIP_RETURN_CODE 77)
        endif ()
        return()
    endif ()

    if ("${EXAMPLE_TARGET}" STREQUAL "testModelGeometryParallelValidation" OR
        "${EXAMPLE_TARGET}" STREQUAL "testVTKXMLDataArrayDecoderValidation" OR
        "${EXAMPLE_TARGET}" STREQUAL "testSpectralReaderCPU" OR
        "${EXAMPLE_TARGET}" STREQUAL "testVertexReaderValidation" OR
        "${EXAMPLE_TARGET}" STREQUAL "testZeroCellPointCloudValidation" OR
        "${EXAMPLE_TARGET}" STREQUAL "testPolylineWireframeValidation" OR
        "${EXAMPLE_TARGET}" STREQUAL "testIGVRoundTrip" OR
        "${EXAMPLE_TARGET}" STREQUAL "testIGVFileIO" OR
        "${EXAMPLE_TARGET}" STREQUAL "testIGVIGCConvert" OR
        "${EXAMPLE_TARGET}" STREQUAL "testIGVRobustness" OR
        "${EXAMPLE_TARGET}" STREQUAL "testIGVLargeFile" OR
        "${EXAMPLE_TARGET}" STREQUAL "testAnimationExportRangeValidation")
        set(_VALIDATION_COMMAND "$<TARGET_FILE:${EXAMPLE_TARGET}>")
        if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND
            "${EXAMPLE_TARGET}" STREQUAL "testAnimationExportRangeValidation")
            list(PREPEND _VALIDATION_COMMAND xvfb-run -a)
        endif ()
        add_test(
            NAME ${EXAMPLE_TARGET}
            COMMAND ${_VALIDATION_COMMAND}
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:${EXAMPLE_TARGET}>"
        )
        set_tests_properties(${EXAMPLE_TARGET} PROPERTIES TIMEOUT 180)
        if ("${EXAMPLE_TARGET}" STREQUAL "testIGVRobustness")
            # 1000 次变异 fuzz：见上面 Windows 分支里对 600 s 的说明。
            set_tests_properties(${EXAMPLE_TARGET} PROPERTIES TIMEOUT 600)
        endif ()
        if ("${EXAMPLE_TARGET}" STREQUAL "testAnimationExportRangeValidation")
            set_tests_properties(${EXAMPLE_TARGET} PROPERTIES SKIP_RETURN_CODE 77)
        endif ()
        return()
    endif ()

    set(_TEST_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/ci/run_smoke_test.sh")
    set(_TEST_COMMAND bash "${_TEST_SCRIPT}" "$<TARGET_FILE:${EXAMPLE_TARGET}>")

    if ("${EXAMPLE_TARGET}" STREQUAL "igvDump")
        # igvDump 是工具：不带参数时打印用法并返回 2，所以 CTest 走它自己的自检模式。
        set(_TEST_COMMAND bash "${_TEST_SCRIPT}" "$<TARGET_FILE:${EXAMPLE_TARGET}>" 120 -- --self-check)
    endif ()

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


# Elevation
igame_add_example(testElevation Filter/Elevation/TestElevation.cpp)
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
igame_add_example(testSpectralReaderCPU ../iGameCore/IO/Spectral/tests/SpectralReaderValidation.cxx)
igame_add_example(testVTKXMLDataArrayDecoderValidation IO/VTKXMLDataArrayDecoderValidation.cpp)
igame_add_example(testVertexReaderValidation IO/VertexReaderValidation.cpp)
igame_add_example(testZeroCellPointCloudValidation Rendering/ZeroCellPointCloudValidation.cpp)
igame_add_example(testPolylineWireframeValidation Rendering/PolylineWireframeValidation.cpp)
igame_add_example(testUtf8FilePathValidation IO/Utf8FilePathValidation.cpp)

# IGV topology probe (answers Q1/Q2/Q3 before implementing the writer).
#
# Not built: it no longer compiles against the current APIs (Points::GetDimension,
# Points::GetArrayTypedSize, IdArray::GetNumberOfValues, SurfaceMesh::GetCells, etc.).
# It is a print-only diagnostic with no product assertions; its three questions are
# already answered. The source is kept as a historical artifact.
# igame_add_example(probeIGVTopologyLayers IO/ProbeIGVTopologyLayers.cpp)

# ---------------------------------------------------------------------------
# IGV round-trip test.
#
# 为什么包在 ENABLE_IGV_MODULE 里：IGV 的读写实现依赖 HDF5（IGV_ENABLE），
# 模块关闭时 iGameCore 里根本不会编译 iGameIGVWriter.cpp / iGameIGVReader.cpp，
# 注册这个目标会直接链接失败。这与 ENABLE_CGNS_MODULE 的写法同构。
#
# 需要显式链接 HDF5：本程序直接用 HDF5 C API 做结构断言
# （dcpl 的 DEFLATE filter、dataspace 固定性、chunk 不切断记录），
# 不把验收挂在外部 h5dump 可执行文件是否可启动上。
# ---------------------------------------------------------------------------
if (ENABLE_IGV_MODULE AND NOT EMSCRIPTEN)
    igame_add_example(testIGVRoundTrip IO/TestIGVRoundTrip.cpp)
    target_link_libraries(testIGVRoundTrip PUBLIC HDF5::HDF5)
    target_compile_definitions(testIGVRoundTrip PUBLIC IGV_ENABLE)
    # Inject the source directory so the test can find Models/ and IGV golden samples
    # without depending on the current working directory.
    target_compile_definitions(testIGVRoundTrip PRIVATE
        IGAME_IGV_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif ()

# ---------------------------------------------------------------------------
# IGV metadata fast-path test.
#
# 与上面两个 IGV 程序的差别：
#   * 只走 FileIO::ReadIGVMetadata + FileIO::ReadFile 两条**公开路径**，自己**不**直接调
#     HDF5 C API（"零解压"的计数断言读的是 iGameIGVStats.h 的原子计数器），
#     因此不需要像 testIGVRoundTrip 那样显式 link HDF5::HDF5；
#   * 仍需 IGV_ENABLE 才能看到 ReadIGVMetadata 的签名，故同样挂在模块门控内。
# ---------------------------------------------------------------------------
if (ENABLE_IGV_MODULE AND NOT EMSCRIPTEN)
    igame_add_example(testIGVMetadata IO/TestIGVMetadata.cpp)
    target_compile_definitions(testIGVMetadata PUBLIC IGV_ENABLE)
    # 与上面两个程序同一套资源定位：注入源码目录，使测试不依赖"当前工作目录"。
    target_compile_definitions(testIGVMetadata PRIVATE
        IGAME_IGV_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif ()

# ---------------------------------------------------------------------------
# IGC <-> IGV conversion test.
#
# 与前面三个 IGV 程序的差别：本程序**直接**用 IGCWriter / IGCReader（而不只是
# FileIO），因为「保真边界报告」需要两样 FileIO 给不了的东西：
#   1. 编码器产生的重排映射（`SetEncodeTrace`）—— IGC 会做空间重排，没有映射就
#      无法把解码结果对回源对象，"逐值 bitwise"只能靠"顺序恰好相同"这个偶然前提；
#   2. 可调的编码档位（`SetCodecControlParams`）—— 有损档必须能真的打开量化，
#      否则 P4-3「必须诚实地说出不能保真」没有可验证的载体。
# 它仍然只是「链接 + 使用」，不改 iGameCore 的任何既有行为。
#
# 已确认过的编译成本：这个 TU 只**声明**用到 IGCWriter，真正昂贵的模板实例化
# （MeshEncoderFilter<>::WriteNumpyExports + cnpy）发生在已编好的
# iGameIGCWriter.cpp 里，因此本文件是秒级编译，不是 20 分钟的重型 TU。
# ---------------------------------------------------------------------------
if (ENABLE_IGV_MODULE AND NOT EMSCRIPTEN)
    igame_add_example(testIGVIGCConvert IO/TestIGVIGCConvert.cpp)
    target_compile_definitions(testIGVIGCConvert PUBLIC IGV_ENABLE)
    # 与其它 IGV 验收程序同一套资源定位：注入源码目录，供它找 Examples/Models。
    target_compile_definitions(testIGVIGCConvert PRIVATE
        IGAME_IGV_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif ()

# ---------------------------------------------------------------------------
# IGV robustness and large-file tests.
#
# 两个目标，性质不同：
#   * testIGVRobustness：畸形文件必须被拒绝（P5-1…P5-11、P5-14）。**默认构建**，
#     它自己造种子文件，不依赖任何外部模型，跑一次是秒级。
#   * testIGVLargeFile：>4 GiB 与亿级单元（P5-12 / P5-13）。**默认不构建**，
#     由 IGAME_ENABLE_IGV_LARGE_TESTS 打开 —— 一次要几 GB 内存与磁盘、耗时分钟级，
#     不能让每个开发者/CI 都无条件承担。
# 两者都需要直接调 HDF5 C API（造/改畸形文件、读回校验），故显式链接 HDF5::HDF5。
# ---------------------------------------------------------------------------
if (ENABLE_IGV_MODULE AND NOT EMSCRIPTEN)
    igame_add_example(testIGVRobustness IO/TestIGVRobustness.cpp)
    target_link_libraries(testIGVRobustness PUBLIC HDF5::HDF5)
    target_compile_definitions(testIGVRobustness PUBLIC IGV_ENABLE)

    if (IGAME_ENABLE_IGV_LARGE_TESTS)
        igame_add_example(testIGVLargeFile IO/TestIGVLargeFile.cpp)
        target_link_libraries(testIGVLargeFile PUBLIC HDF5::HDF5)
        target_compile_definitions(testIGVLargeFile PUBLIC IGV_ENABLE)
    endif ()

    # 结构 dump 工具（等价 h5dump -H -A 的可读版）：既是人工排查工具，
    # 也带 `--self-check`（造样例 → dump → 断言关键行），因此能在 CTest 里跑。
    igame_add_example(igvDump IO/igvDump.cpp)
    target_link_libraries(igvDump PUBLIC HDF5::HDF5)
    target_compile_definitions(igvDump PUBLIC IGV_ENABLE)

    # ------------------------------------------------------------------
    # igvDump 的 GUI 模式（`--gui`）：一个 ParaView Information 风格的面板。
    #
    # 为什么和命令行做在同一个 exe 里：两者看的是同一份元数据，只是"读结构"与
    # "看数值"两种习惯；拆成两个 exe 会让 `--self-check`（CTest 用）与 GUI
    # 各存一份入口。IGV_DUMP_QT 只影响 IGVDumpInfoWindow.cpp 是否参与编译，
    # 命令行分支在两种构建里**完全一致**。
    #
    # 没有 Qt 的构建：`--gui` 打印一行明确说明并返回 3（不静默退化成文本模式）。
    # ------------------------------------------------------------------
    if (ENABLE_QT_MODULE)
        find_package(Qt5 QUIET COMPONENTS Core Gui Widgets)
        if (TARGET Qt5::Widgets)
            target_sources(igvDump PRIVATE IO/IGVDumpInfoWindow.cpp)
            target_link_libraries(igvDump PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)
            target_compile_definitions(igvDump PRIVATE IGV_DUMP_QT)
            target_include_directories(igvDump PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/IO")

            # 让 exe 能在资源管理器里双击 / 右键直接启动：Qt 的 DLL 与
            # platforms 插件必须和 exe 同目录，否则一启动就是 0xC0000135
            # （窗口一闪而过，看起来像"程序什么都没干"）。
            if (WIN32 AND DEPLOYQT_EXECUTABLE)
                add_custom_command(TARGET igvDump POST_BUILD
                        COMMAND "${DEPLOYQT_EXECUTABLE}"
                        --no-translations --no-opengl-sw --no-compiler-runtime
                        --no-system-d3d-compiler
                        "$<TARGET_FILE:igvDump>"
                        COMMENT "Deploying Qt runtime for igvDump (GUI mode)")
            endif ()
        else ()
            message(STATUS "igvDump: Qt5 Widgets not found - GUI mode (--gui) will report as unavailable")
        endif ()
    endif ()
endif ()

# ---------------------------------------------------------------------------
# IGV FileIO integration test.
#
# 与 testIGVRoundTrip 不同，本程序**不**链接 HDF5、只经 FileIO 分派，因此刻意
# 注册在 ENABLE_IGV_MODULE 门控**之外**（只排除 Emscripten）：
#   * 模块 ON ：验证 P2-3/4/5（主路径写/读、内存读）与 P2-8（既有格式零回归）；
#   * 模块 OFF：同一份源码编译出的程序转而验证 P2-7（后缀仍被识别，I/O 明确失败）。
# 若包在门控里，模块关闭时的降级路径就没有可运行的测试载体。
#
# 不链接 HDF5 的直接好处：该 TU 不引入 IGV/HDF5 头，增量编译显著快于
# testIGVRoundTrip（后者经 iGameIGVWriter.h 间接拉入 MeshEncoderFilter）。
# ---------------------------------------------------------------------------
if (NOT EMSCRIPTEN)
    igame_add_example(testIGVFileIO IO/TestIGVFileIO.cpp)
    # 与 testIGVRoundTrip 同一套资源定位：注入源码目录，供 P2-8 找 Examples/Models。
    target_compile_definitions(testIGVFileIO PRIVATE
        IGAME_IGV_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif ()

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
igame_add_example(testCCMReader IO/CCMReader.cpp)
igame_add_example(testAnsysRSTReader IO/AnsysRSTReader.cpp)
igame_add_example(testAnsysRTHReader IO/AnsysRTHReader.cpp)
igame_add_example(testLsDynaReader IO/LsDynaReader.cpp)

# Animation / Filters
igame_add_example(testAnimation Animation/TestAnimation.cpp)

if (TARGET Qt_module AND TARGET iGameVis)
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets OpenGL Charts Svg)
    igame_add_example(testAnimationPipelineValidation
        Animation/AnimationPipelineValidation.cpp)
    target_link_libraries(testAnimationPipelineValidation PRIVATE Qt_module)
    if (NOT IGAME_BUILD_SHARED)
        target_compile_definitions(testAnimationPipelineValidation PRIVATE IG_QT_COMPILE_EXPORT)
    endif ()
    set_target_properties(testAnimationPipelineValidation PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

    igame_add_example(testModelTreeViewStyleValidation Rendering/ModelTreeViewStyleValidation.cpp)
    target_link_libraries(testModelTreeViewStyleValidation PRIVATE Qt_module)
    if (NOT IGAME_BUILD_SHARED)
        target_compile_definitions(testModelTreeViewStyleValidation PRIVATE IG_QT_COMPILE_EXPORT)
    endif ()
    # Reuse the desktop application's deployed Qt/runtime DLLs.
    set_target_properties(testModelTreeViewStyleValidation PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:iGameVis>")
    add_dependencies(testModelTreeViewStyleValidation iGameVis)

    igame_add_example(testAnimationVertexValidation Animation/AnimationVertexValidation.cpp)
    target_link_libraries(testAnimationVertexValidation PRIVATE Qt_module)
    if (NOT IGAME_BUILD_SHARED)
        target_compile_definitions(testAnimationVertexValidation PRIVATE IG_QT_COMPILE_EXPORT)
    endif ()
    set_target_properties(testAnimationVertexValidation PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/vertex-animation-validation/$<CONFIG>")
    add_dependencies(testAnimationVertexValidation iGameVis)
endif ()

if (FFMPEG_FOUND)
    igame_add_example(testSaveAnimation Animation/SaveAnimation.cpp)
    target_compile_definitions(testSaveAnimation PUBLIC FFMPEG_ENABLE)
    igame_add_example(testAnimationExportRangeValidation Animation/AnimationExportRangeValidation.cpp)
    target_sources(testAnimationExportRangeValidation PRIVATE Animation/SaveAnimation.cpp)
    target_compile_definitions(testAnimationExportRangeValidation PRIVATE FFMPEG_ENABLE IGAME_ANIMATION_EXPORT_LIBRARY)
endif ()

igame_add_example(testClip Filter/TestClip.cpp)
igame_add_example(testSlice Filter/TestSlice.cpp)
igame_add_example(testIsoVolume Filter/TestIsoVolume.cpp)
igame_add_example(testDeformation Filter/Deformation/TestStressDeformationFilter.cpp)
igame_add_example(testDeformationCode Filter/Deformation/TestStressDeformationFilterCode.cpp)
igame_add_example(testContourLine Filter/TestContourLine.cpp)
igame_add_example(testContourExtraction Filter/TestContourExtraction.cpp)
igame_add_example(testModelGeometryParallelValidation Filter/ModelGeometryParallelValidation.cpp)
add_custom_command(TARGET testModelGeometryParallelValidation POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/test/Animation Test/result-case1/VTKfile-0.vtk"
        "$<TARGET_FILE_DIR:testModelGeometryParallelValidation>/VTKfile-0.vtk"
)

# Color bar / interaction
igame_add_example(testChangeColorRange ColorBar/ChangeColorRange.cpp)
igame_add_example(testChangeColorBar ColorBar/ChangeColorBar.cpp)
igame_add_example(testMultiscaleInteraction MultiscaleInteraction/TestMultiscaleInteraction.cpp)

# Feature extraction
igame_add_example(testVortexExtraction Filter/FeatureExtraction/VortexExtraction.cpp)
igame_add_example(testCurvatureExtraction Filter/FeatureExtraction/CurvatureExtraction.cpp)
igame_add_example(testGradientExtraction Filter/FeatureExtraction/GradientExtraction.cpp)
igame_add_example(testAdvancedGradientFilter Filter/FeatureExtraction/AdvancedGradientFilterTest.cpp)
igame_add_example(testLaplacianExtraction Filter/FeatureExtraction/LaplacianExtraction.cpp)

if (ENABLE_LIBTORCH_MODULE)
    igame_add_example(testVortexDetection Filter/FeatureExtraction/VortexDetection.cpp)
    target_compile_definitions(testVortexDetection PUBLIC LibTorch_ENABLE)
    target_link_libraries(testVortexDetection PRIVATE ${TORCH_LIBRARIES})
    if (OpenMP_CXX_FOUND)
        target_link_libraries(testVortexDetection PRIVATE OpenMP::OpenMP_CXX)
    endif ()
endif ()


# Batch 2 filters: algorithms, regression checks, and visual examples.
igame_add_example(testBatch2GeometryValidation Filter/Batch2GeometryValidation.cpp)
igame_add_example(testCellSizeExtraction Filter/FeatureExtraction/CellSizeExtraction.cpp)
igame_add_example(testCountCellVertices Filter/TestCountCellVertices.cpp)
igame_add_example(testExtractEdges Filter/TestExtractEdges.cpp)
igame_add_example(testGenerateGlobalIds Filter/GlobalIds/TestGenerateGlobalIds.cpp)
igame_add_example(testPointAndCellIds Filter/PointAndCellIds/TestPointAndCellIds.cpp)
igame_add_example(testGenerateProcessIds Filter/TestGenerateProcessIds.cpp)
igame_add_example(testAxisAlignedReflection Filter/AxisAlignedReflection/TestAxisAlignedReflection.cpp)
igame_add_example(testExtractComponent Filter/TestExtractComponent.cpp)
igame_add_example(testMergeVectorComponents Filter/TestMergeVectorComponents.cpp)
igame_add_example(testResampleToImage Filter/Convert/TestResampleToImage.cpp)
igame_add_example(testResampleToLine Filter/TestResampleToLine.cpp)
igame_add_example(testTriangleStrip Filter/TriangleStrip/TestTriangleStrip.cpp)
igame_add_example(testResampleImageChecks Filter/Convert/TestResampleImageChecks.cpp)
if (ENABLE_QT_MODULE)
    igame_add_example(testThirdBatchMenu Filter/ThirdBatchMenuValidation.cpp)
    target_link_libraries(testThirdBatchMenu PRIVATE Qt_module)
    target_include_directories(testThirdBatchMenu PRIVATE "${CMAKE_SOURCE_DIR}/Qt")
    if (NOT IGAME_BUILD_SHARED)
        target_compile_definitions(testThirdBatchMenu PRIVATE IG_QT_COMPILE_EXPORT)
    endif ()
    igame_add_example(testTriangleStripWidget Filter/TriangleStrip/TestTriangleStripWidget.cpp)
    target_link_libraries(testTriangleStripWidget PRIVATE Qt_module)
    igame_add_example(testStandardFiltersMenu Filter/StandardFiltersMenuValidation.cpp)
    target_link_libraries(testStandardFiltersMenu PRIVATE Qt_module)
    target_include_directories(testStandardFiltersMenu PRIVATE "${CMAKE_SOURCE_DIR}/Qt")
    if (NOT IGAME_BUILD_SHARED)
        target_compile_definitions(testTriangleStripWidget PRIVATE IG_QT_COMPILE_EXPORT)
        target_compile_definitions(testStandardFiltersMenu PRIVATE IG_QT_COMPILE_EXPORT)
    endif ()
endif ()

# Third-batch filter examples and numerical acceptance checks.
igame_add_example(testThirdBatchData Filter/ThirdBatchDataValidation.cpp)
igame_add_example(testAngularPeriodic Filter/Periodic/TestAngularPeriodic.cpp)
igame_add_example(testAngularPeriodicSelfCheck Filter/Periodic/TestAngularPeriodicSelfCheck.cpp)
igame_add_example(testAppendLocationAttributeFilter Filter/TestAppendLocationAttribute.cpp)
igame_add_example(testBoundaryMeshQuality Filter/TestBoundaryMeshQuality.cpp)
igame_add_example(testCleanToGridFilter Filter/TestCleanToGridFilter.cpp)
igame_add_example(testCountCellFaces Filter/FeatureExtraction/CountCellFaces.cpp)
igame_add_example(testCountCellFacesOutput Filter/FeatureExtraction/CountCellFacesOutput.cpp)
igame_add_example(testFeatureEdgeRegion Filter/FeatureExtraction/FeatureEdgeRegion.cpp)
igame_add_example(testThresholdAndGenerateIds Filter/TestThresholdAndGenerateIds.cpp)
igame_add_example(testMeshQuality Filter/MeshQuality/TestMeshQuality.cpp)
igame_add_example(testPointLineInterpolator Filter/PointLineInterpolator/TestPointLineInterpolator.cpp)
igame_add_example(testPointSetToOctree Filter/Convert/TestPointSetToOctree.cpp)
igame_add_example(testResampleOctreeChecks Filter/Convert/TestResampleOctreeChecks.cpp)
igame_add_example(testPointVolumeInterpolator Filter/Interpolation/TestPointVolumeInterpolator.cpp)
igame_add_example(testPointVolumeInterpolatorSelfCheck Filter/Interpolation/TestPointVolumeInterpolatorSelfCheck.cpp)
igame_add_example(testShrink Filter/Shrink/TestShrink.cpp)
igame_add_example(testShrinkModel Filter/Shrink/TestShrinkModel.cpp)
igame_add_example(testSurfaceNormalsFilter Filter/SurfaceNormals/TestSurfaceNormalsFilter.cpp)
igame_add_example(testVolumeMeshSimplification Filter/TestVolumeMeshSimplification.cpp)
igame_add_example(testMeshTetrahedralize Filter/TestMeshTetrahedralize.cpp)

# First-batch standard filter examples (Elevation is registered above).
igame_add_example(testPointCoordinates Filter/PointCoordinates/TestPointCoordinates.cpp)
igame_add_example(testCellCenterFilter Filter/CellCenter/TestCellCenterFilter.cpp)
igame_add_example(testRandomVectors Filter/AttributeManipulation/TestRandomVectors.cpp)
igame_add_example(testRemoveGhostInformation Filter/RemoveGhostInformation/TestRemoveGhostInformation.cpp)
igame_add_example(testMaskPoints Filter/MaskPoints/TestMaskPoints.cpp)
igame_add_example(testFeatureEdges Filter/FeatureExtraction/FeatureEdges.cpp)
igame_add_example(testFeatureEdgesVisualization Filter/FeatureExtraction/FeatureEdgesVisualization.cpp)
igame_add_example(testOutlineCorners Filter/FeatureExtraction/OutlineCorners.cpp)
igame_add_example(testProbe Filter/Probe/TestProbe.cpp)
igame_add_example(testConvertToVertex Filter/Convert/TestConvertToVertex.cpp)
igame_add_example(testExtractSubset Filter/TestExtractSubset.cpp)


# Standard filters migrated from the local igamevis repository.
igame_add_example(TestValidateCellsFilter Filter/ValidateCells/TestValidateCellsFilter.cpp)
igame_add_example(testExtractCellsByType Filter/ExtractCellsByType/TestExtractCellsByType.cpp)
igame_add_example(testExtractLocation Filter/DataProcessing/ExtractLocation/TestExtractLocation.cpp)
igame_add_example(testExtractLocationDrag Filter/DataProcessing/ExtractLocation/TestExtractLocationDrag.cpp)
igame_add_example(testOutline Filter/Outline/TestOutline.cpp)
igame_add_example(testReverseSense Filter/ReverseSense/TestReverseSense.cpp)
igame_add_example(testReverseSenseSelfCheck Filter/ReverseSense/TestReverseSenseSelfCheck.cpp)
igame_add_example(testWarpByScalar Filter/WarpByScalar/TestWarpByScalar.cpp)
igame_add_example(testAppendAttributes Filter/AppendAttributes/TestAppendAttributes.cpp)
igame_add_example(TestTubeFilter Filter/Tube/TestTubeFilter.cpp)
igame_add_example(TestSubdivideFilter Filter/Subdivide/TestSubdivideFilter.cpp)
igame_add_example(testLinearExtrusion Filter/TestLinearExtrusion.cpp)
igame_add_example(testMigratedStandardFilters Filter/TestMigratedStandardFilters.cpp)
igame_add_example(testWarpScalarAttributeRanges Filter/WarpByScalar/TestWarpScalarAttributeRanges.cpp)

add_custom_target(iGameCopyMigratedFilterAssets
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../test_models/subdivide_cell_data"
        "${CMAKE_CURRENT_BINARY_DIR}/SubdivideModels")
add_dependencies(testMigratedStandardFilters iGameCopyMigratedFilterAssets)


# Additional standard filters: complete upstream algorithms and numerical checks.
igame_add_example(testGenerateSurfaceTangents Filter/GenerateSurfaceTangents/TestGenerateSurfaceTangents.cpp)
igame_add_example(testGroupDatasets Filter/TestGroupDatasets.cpp)
igame_add_example(testMedianFilter Filter/Median/TestMedianFilter.cpp)
igame_add_example(testRenameArrays Filter/TestRenameArrays.cpp)
igame_add_example(testResampleWithDataSet Filter/TestResampleWithDataSet.cpp)
igame_add_example(testTextureMapToCylinder Filter/TestTextureMapToCylinder.cpp)
igame_add_example(testPerlinNoise Filter/AttributeManipulation/TestPerlinNoise.cpp)
igame_add_example(testTextureMapToPlane Filter/DataProcessing/TextureMapToPlane/TestTextureMapToPlane.cpp)
igame_add_example(testTextureMapToSphere Filter/TextureMapToSphere/TestTextureMapToSphere.cpp)


# October 19 standard filter migration: upstream examples and checks.
igame_add_example(testAppendArcLength Filter/AppendArcLength/TestAppendArcLength.cpp)
igame_add_example(testAppendArcLengthChecks Filter/AppendArcLength/TestAppendArcLengthChecks.cpp)
igame_add_example(testCellMeshMetrics Filter/TestCellMeshMetrics.cpp)
igame_add_example(testConnectivity Filter/Connectivity/TestConnectivity.cpp)
igame_add_example(testConnectivitySelfCheck Filter/Connectivity/TestConnectivitySelfCheck.cpp)
igame_add_example(testDecimatePolyline Filter/DataProcessing/DecimatePolyline/TestDecimatePolyline.cpp)
igame_add_example(testIntegrateVariables Filter/FeatureExtraction/TestIntegrateVariables.cpp)
igame_add_example(testMultiBlockGeometry Filter/TestMultiBlockGeometry.cpp)
igame_add_example(testRibbon Filter/TestRibbon.cpp)
igame_add_example(testSmooth Filter/Smooth/TestSmooth.cpp)
igame_add_example(testTensorPrincipalInvariants Filter/TestTensorPrincipalInvariants.cpp)
igame_add_example(testTransform Filter/Transformation/TestTransform.cpp)
igame_add_example(testWarpByVector Filter/TestWarpByVector.cpp)

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
add_test(NAME oct19_testAppendArcLengthChecks COMMAND testAppendArcLengthChecks --no-pause)
set_tests_properties(oct19_testAppendArcLengthChecks PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "oct19-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME oct19_testCellMeshMetrics COMMAND testCellMeshMetrics --selfcheck)
set_tests_properties(oct19_testCellMeshMetrics PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "oct19-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME oct19_testConnectivitySelfCheck COMMAND testConnectivitySelfCheck )
set_tests_properties(oct19_testConnectivitySelfCheck PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "oct19-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME oct19_testDecimatePolyline COMMAND testDecimatePolyline )
set_tests_properties(oct19_testDecimatePolyline PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "oct19-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME oct19_testIntegrateVariables COMMAND testIntegrateVariables )
set_tests_properties(oct19_testIntegrateVariables PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "oct19-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME oct19_testMultiBlockGeometry COMMAND testMultiBlockGeometry --selfcheck)
set_tests_properties(oct19_testMultiBlockGeometry PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "oct19-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME oct19_testRibbon COMMAND testRibbon )
set_tests_properties(oct19_testRibbon PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "oct19-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME oct19_testSmooth COMMAND testSmooth --check)
set_tests_properties(oct19_testSmooth PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "oct19-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME oct19_testTensorPrincipalInvariants COMMAND testTensorPrincipalInvariants )
set_tests_properties(oct19_testTensorPrincipalInvariants PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "oct19-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME oct19_testTransform COMMAND testTransform --check)
set_tests_properties(oct19_testTransform PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "oct19-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME oct19_testWarpByVector COMMAND testWarpByVector )
set_tests_properties(oct19_testWarpByVector PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "oct19-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")

add_test(NAME additional_testGenerateSurfaceTangents COMMAND testGenerateSurfaceTangents)
set_tests_properties(additional_testGenerateSurfaceTangents PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "additional-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME additional_testGroupDatasets COMMAND testGroupDatasets)
set_tests_properties(additional_testGroupDatasets PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "additional-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME additional_testMedianFilter COMMAND testMedianFilter)
set_tests_properties(additional_testMedianFilter PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "additional-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME additional_testRenameArrays COMMAND testRenameArrays)
set_tests_properties(additional_testRenameArrays PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "additional-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME additional_testResampleWithDataSet COMMAND testResampleWithDataSet)
set_tests_properties(additional_testResampleWithDataSet PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "additional-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME additional_testTextureMapToCylinder COMMAND testTextureMapToCylinder)
set_tests_properties(additional_testTextureMapToCylinder PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "additional-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME additional_testPerlinNoise COMMAND testPerlinNoise)
set_tests_properties(additional_testPerlinNoise PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "additional-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME additional_testTextureMapToPlane COMMAND testTextureMapToPlane)
set_tests_properties(additional_testTextureMapToPlane PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "additional-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")
add_test(NAME additional_testTextureMapToSphere COMMAND testTextureMapToSphere)
set_tests_properties(additional_testTextureMapToSphere PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "additional-standard-filters" ENVIRONMENT "IGV_TEST_NO_VIEW=1")

add_test(NAME testWarpScalarLazyRanges COMMAND testWarpScalarAttributeRanges)
set_tests_properties(testWarpScalarLazyRanges PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 60)
add_test(NAME testMigratedAlgorithms COMMAND testMigratedStandardFilters)
add_test(NAME testMigratedOutline COMMAND testOutline --check)
add_test(NAME testMigratedExtractCellsByType COMMAND testExtractCellsByType --no-render)
add_test(NAME testMigratedExtractLocation COMMAND testExtractLocation)
add_test(NAME testMigratedExtractLocationDrag COMMAND testExtractLocationDrag)
add_test(NAME testMigratedReverseSense COMMAND testReverseSenseSelfCheck)
add_test(NAME testMigratedLinearExtrusion COMMAND testLinearExtrusion)
set_tests_properties(testMigratedAlgorithms testMigratedOutline
    testMigratedExtractCellsByType testMigratedExtractLocation
    testMigratedExtractLocationDrag testMigratedReverseSense
    testMigratedLinearExtrusion PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 60 LABELS "migrated-standard-filters")
add_executable(testDrawObjectResourcesValidation IO/DrawObjectResourcesValidation.cpp)
igame_link_standard_example(testDrawObjectResourcesValidation)
add_test(NAME testDrawObjectResourcesValidation COMMAND testDrawObjectResourcesValidation)
set_tests_properties(testDrawObjectResourcesValidation PROPERTIES TIMEOUT 30)
if (TARGET Qt_module)
find_package(Qt5 REQUIRED COMPONENTS Core Gui)
add_executable(testPreparedCpuGpuValidation IO/PreparedCpuGpuValidation.cpp)
target_link_libraries(testPreparedCpuGpuValidation PRIVATE Qt5::Gui)
igame_link_standard_example(testPreparedCpuGpuValidation)
add_test(NAME testPreparedCpuGpuValidation COMMAND testPreparedCpuGpuValidation)
set_tests_properties(testPreparedCpuGpuValidation PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
get_target_property(_IGAME_TEST_QMAKE Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_IGAME_TEST_QT_BIN "${_IGAME_TEST_QMAKE}" DIRECTORY)
set_tests_properties(testPreparedCpuGpuValidation PROPERTIES ENVIRONMENT
    "QT_QPA_PLATFORM_PLUGIN_PATH=${_IGAME_TEST_QT_BIN}/../plugins/platforms")
add_executable(testResidentModelCacheValidation
    IO/ResidentModelCacheValidation.cpp
    "${CMAKE_SOURCE_DIR}/Qt/src/IQCore/igQtResidentModelCache.cpp")
target_include_directories(testResidentModelCacheValidation PRIVATE "${CMAKE_SOURCE_DIR}/Qt/include")
target_compile_definitions(testResidentModelCacheValidation PRIVATE IG_QT_COMPILE_EXPORT)
target_link_libraries(testResidentModelCacheValidation PRIVATE Qt5::Core)
igame_link_standard_example(testResidentModelCacheValidation)
add_test(NAME testResidentModelCacheValidation COMMAND testResidentModelCacheValidation)
set_tests_properties(testResidentModelCacheValidation PROPERTIES TIMEOUT 30)
endif ()
foreach(EXAMPLE_TARGET IN LISTS IGAME_EXAMPLE_TARGETS)
    igame_register_example_test(${EXAMPLE_TARGET})
endforeach()

add_test(NAME testSpectralReaderCPUSamples
    COMMAND "$<TARGET_FILE:testSpectralReaderCPU>" --samples
        "${CMAKE_CURRENT_SOURCE_DIR}/../test/Spectral Test"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:testSpectralReaderCPU>")
set_tests_properties(testSpectralReaderCPUSamples PROPERTIES TIMEOUT 180)

igame_add_example(testAppendDatasets Filter/TestAppendDatasets.cpp)

igame_add_example(testDeflectNormals Filter/TestDeflectNormals.cpp)

igame_add_example(testPointPlaneInterpolator Filter/TestPointPlaneInterpolator.cpp)

igame_add_example(testRandomAttributes Filter/TestRandomAttributes.cpp)

igame_add_example(testSurfaceVectors Filter/TestSurfaceVectors.cpp)

igame_add_example(testHistogram Filter/Histogram/TestHistogram.cpp)

if (TARGET Qt_module)
igame_add_example(testHistogramChartChecks Filter/Histogram/TestHistogramChartChecks.cpp)
target_link_libraries(testHistogramChartChecks PRIVATE Qt_module Qt5::Widgets Qt5::Charts)
endif ()

igame_add_example(testHistogramChecks Filter/Histogram/TestHistogramChecks.cpp)

igame_add_example(testExtractCellsByRegion Filter/Selection/TestExtractCellsByRegion.cpp)

if (TARGET Qt_module)
igame_add_example(testExtractCellsByRegionDialog Filter/Selection/TestExtractCellsByRegionDialog.cpp)
target_link_libraries(testExtractCellsByRegionDialog PRIVATE Qt_module Qt5::Widgets Qt5::Charts)
endif ()

if (TARGET Qt_module)
igame_add_example(testExtractCellsByRegionUi Filter/Selection/TestExtractCellsByRegionUi.cpp)
target_link_libraries(testExtractCellsByRegionUi PRIVATE Qt_module Qt5::Widgets Qt5::Charts)
endif ()

igame_add_example(testYieldCriteria Filter/Tensor/TestYieldCriteria.cpp)

igame_add_example(testExtractTimeSteps Filter/TimeSeries/TestExtractTimeSteps.cpp)

igame_add_example(testTemporalShiftScale Filter/TimeSeries/TestTemporalShiftScale.cpp)

igame_add_example(testVolumeOfRevolution Filter/VolumeOfRevolution/TestVolumeOfRevolution.cpp)

igame_add_example(testConnectedSurfaceProperties Filter/DataProcessing/ConnectedSurfaceProperties/TestConnectedSurfaceProperties.cpp)

if (TARGET Qt_module)
igame_add_example(testConnectedSurfacePropertiesWidget Filter/DataProcessing/ConnectedSurfaceProperties/TestConnectedSurfacePropertiesWidget.cpp)
target_link_libraries(testConnectedSurfacePropertiesWidget PRIVATE Qt_module Qt5::Widgets Qt5::Charts)
endif ()

igame_add_example(testOverlappingCellsDetector Filter/DataProcessing/OverlappingCellsDetector/TestOverlappingCellsDetector.cpp)

add_test(NAME migrated_testAppendDatasets COMMAND testAppendDatasets)
set_tests_properties(migrated_testAppendDatasets PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

add_test(NAME migrated_testDeflectNormals COMMAND testDeflectNormals --no-render)
set_tests_properties(migrated_testDeflectNormals PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

add_test(NAME migrated_testPointPlaneInterpolator COMMAND testPointPlaneInterpolator)
set_tests_properties(migrated_testPointPlaneInterpolator PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

add_test(NAME migrated_testRandomAttributes COMMAND testRandomAttributes --no-render)
set_tests_properties(migrated_testRandomAttributes PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

add_test(NAME migrated_testSurfaceVectors COMMAND testSurfaceVectors)
set_tests_properties(migrated_testSurfaceVectors PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

if (TARGET testHistogramChartChecks)
add_test(NAME migrated_testHistogramChartChecks COMMAND testHistogramChartChecks)
set_tests_properties(migrated_testHistogramChartChecks PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")
endif ()

add_test(NAME migrated_testHistogramChecks COMMAND testHistogramChecks --no-pause)
set_tests_properties(migrated_testHistogramChecks PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

add_test(NAME migrated_testExtractCellsByRegion COMMAND testExtractCellsByRegion)
set_tests_properties(migrated_testExtractCellsByRegion PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

if (TARGET testExtractCellsByRegionDialog)
add_test(NAME migrated_testExtractCellsByRegionDialog COMMAND testExtractCellsByRegionDialog)
set_tests_properties(migrated_testExtractCellsByRegionDialog PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")
endif ()

if (TARGET testExtractCellsByRegionUi)
add_test(NAME migrated_testExtractCellsByRegionUi COMMAND testExtractCellsByRegionUi Models/ExtractCellsByRegion_BoxCases.vtk)
set_tests_properties(migrated_testExtractCellsByRegionUi PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")
endif ()

add_test(NAME migrated_testYieldCriteria COMMAND testYieldCriteria)
set_tests_properties(migrated_testYieldCriteria PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

add_test(NAME migrated_testExtractTimeSteps COMMAND testExtractTimeSteps)
set_tests_properties(migrated_testExtractTimeSteps PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

add_test(NAME migrated_testTemporalShiftScale COMMAND testTemporalShiftScale)
set_tests_properties(migrated_testTemporalShiftScale PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

add_test(NAME migrated_testVolumeOfRevolution COMMAND testVolumeOfRevolution)
set_tests_properties(migrated_testVolumeOfRevolution PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

add_test(NAME migrated_testConnectedSurfaceProperties COMMAND testConnectedSurfaceProperties)
set_tests_properties(migrated_testConnectedSurfaceProperties PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

if (TARGET testConnectedSurfacePropertiesWidget)
add_test(NAME migrated_testConnectedSurfacePropertiesWidget COMMAND testConnectedSurfacePropertiesWidget)
set_tests_properties(migrated_testConnectedSurfacePropertiesWidget PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")
endif ()

add_test(NAME migrated_testOverlappingCellsDetector COMMAND testOverlappingCellsDetector)
set_tests_properties(migrated_testOverlappingCellsDetector PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" TIMEOUT 120 LABELS "standard-filter-migration" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;IGAME_EXAMPLE_NO_RENDER=1;IGV_TEST_NO_VIEW=1")

# These integration tests exercise GL-backed interactive widgets.
if (WIN32 AND TARGET Qt_module)
set_tests_properties(migrated_testExtractCellsByRegionUi migrated_testConnectedSurfacePropertiesWidget PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows;IGAME_EXAMPLE_NO_RENDER=1")
endif ()

foreach(_MIGRATED_FILTER_TARGET IN ITEMS testAppendDatasets testDeflectNormals testPointPlaneInterpolator testRandomAttributes testSurfaceVectors testHistogram testHistogramChartChecks testHistogramChecks testExtractCellsByRegion testExtractCellsByRegionDialog testExtractCellsByRegionUi testYieldCriteria testExtractTimeSteps testTemporalShiftScale testVolumeOfRevolution testConnectedSurfaceProperties testConnectedSurfacePropertiesWidget testOverlappingCellsDetector)
    if (TARGET ${_MIGRATED_FILTER_TARGET})
    igame_link_standard_example(${_MIGRATED_FILTER_TARGET})
    target_include_directories(${_MIGRATED_FILTER_TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/Qt")
    endif ()
endforeach()

# Match the application's Qt runtime without copying DLLs into the source tree.
# Core-only CI does not create Qt targets.
if (WIN32 AND TARGET Qt_module)
get_target_property(_MIGRATED_QMAKE Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_MIGRATED_QT_BIN "${_MIGRATED_QMAKE}" DIRECTORY)
if (WIN32)
    string(REPLACE ";" "\\;" _MIGRATED_TEST_PATH "$<TARGET_FILE_DIR:iGameVis>;${_MIGRATED_QT_BIN};$ENV{PATH}")
    foreach(_MIGRATED_QT_TEST IN ITEMS testHistogramChartChecks testExtractCellsByRegionDialog testExtractCellsByRegionUi testConnectedSurfacePropertiesWidget)
        set_property(TEST migrated_${_MIGRATED_QT_TEST} APPEND PROPERTY ENVIRONMENT
            "PATH=${_MIGRATED_TEST_PATH}"
            "QT_QPA_PLATFORM_PLUGIN_PATH=${_MIGRATED_QT_BIN}/../plugins/platforms")
    endforeach()
endif()

endif ()
