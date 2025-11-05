if (CORE_MODULE_INSTALL AND CMAKE_BUILD_TYPE STREQUAL "Release")
    install(TARGETS ${MODULE_NAME}
            EXPORT ${MODULE_NAME}Targets
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin
            INCLUDES DESTINATION include)
    install(TARGETS ${ThirdParty_lib_dependency}
            EXPORT ${MODULE_NAME}Targets
            ARCHIVE DESTINATION lib/ThirdParty
            LIBRARY DESTINATION lib/ThirdParty
            INCLUDES DESTINATION include)

    install(CODE "
        file(GLOB ZSTD_ROOT_FILES \"\${CMAKE_INSTALL_PREFIX}/lib/libzstd.*\" \"\${CMAKE_INSTALL_PREFIX}/lib/zstd_static.*\")
        foreach(ZSTD_ROOT_FILE IN LISTS ZSTD_ROOT_FILES)
            file(REMOVE \"\${ZSTD_ROOT_FILE}\")
        endforeach()
        
        file(REMOVE 
            \"\${CMAKE_INSTALL_PREFIX}/include/zstd.h\"
            \"\${CMAKE_INSTALL_PREFIX}/include/zstd_errors.h\"
            \"\${CMAKE_INSTALL_PREFIX}/include/zdict.h\"
        )
        
        if(EXISTS \"\${CMAKE_INSTALL_PREFIX}/lib/ThirdParty/zstd_static.lib\")
            file(RENAME 
                \"\${CMAKE_INSTALL_PREFIX}/lib/ThirdParty/zstd_static.lib\"
                \"\${CMAKE_INSTALL_PREFIX}/lib/ThirdParty/libzstd_static.lib\"
            )
        endif()
        
        if(EXISTS \"\${CMAKE_INSTALL_PREFIX}/lib/ThirdParty/libzstd.a\")
            file(RENAME 
                \"\${CMAKE_INSTALL_PREFIX}/lib/ThirdParty/libzstd.a\"
                \"\${CMAKE_INSTALL_PREFIX}/lib/ThirdParty/liblibzstd_static.a\"
            )
        endif()
    ")
    


    if (ENABLE_CGNS_MODULE)
        set(HDF5_DIR "C:/Program Files/HDF_Group/HDF5/1.13.0/share/cmake")
        find_package(HDF5)
        #        install(FILES
        #                ${HDF5_LIBRARIES}/../libhdf5.lib
        #                ${HDF5_LIBRARIES}/../libhdf5_hl.lib
        #                ${HDF5_LIBRARIES}/../libhdf5_tools.lib
        #                ${HDF5_LIBRARIES}/../libzlib.lib
        #                ${HDF5_LIBRARIES}/../libsz.lib
        #                ${HDF5_LIBRARIES}/../libaec.lib
        #                DESTINATION lib/ThirdParty)

        install(FILES
                ${HDF5_DIR}/../../lib/libhdf5.lib
                ${HDF5_DIR}/../../lib/libhdf5_hl.lib
                ${HDF5_DIR}/../../lib/libhdf5_tools.lib
                ${HDF5_DIR}/../../lib/libzlib.lib
                ${HDF5_DIR}/../../lib/libsz.lib
                ${HDF5_DIR}/../../lib/libaec.lib
                DESTINATION lib/ThirdParty)

        #        file(GLOB )
    endif ()


    install(DIRECTORY ${Eigen_INCLUDE_DIRS} DESTINATION include/ThirdParty/eigen-3.4.0/Eigen)

    if (${AbqSDK_FOUND})
        install(DIRECTORY ${AbqSDK_PRIVATE_INCLUDE_DIRS} DESTINATION include/ThirdParty/AbaqusSDK
                FILES_MATCHING PATTERN "*.h"
                PATTERN "*/" EXCLUDE
                PATTERN "2024/win_b64" EXCLUDE
        )
        file(GLOB DLL_FILES "${AbqSDK_DLL_DIR}/*.dll")
        #        message(WARNING ${AbqSDK_DLL_DIR})
        file(COPY ${DLL_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/AbaqusSDK)
        list(APPEND ThirdParty_lib_dependency ${ABQ_LIB_LIST})

        foreach (LIB ${ABQ_LIB_LIST})
            #            install(FILES ${temp${LIB}} LIBRARY DESTINATION lib/ThirdParty/AbaqusSDK)
            file(COPY ${temp${LIB}} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/ThirdParty/AbaqusSDK)
        endforeach ()
    endif ()
    if (${FFMPEG_FOUND})

        install(DIRECTORY ${FFMPEG_PRIVATE_INCLUDE_DIR} DESTINATION include/ThirdParty/FFMPEG FILES_MATCHING PATTERN "*.h")
        file(GLOB DLL_FILES "${FFMPEG_DLL_DIR}/*.dll")

        file(COPY ${DLL_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/FFMPEG)
        list(APPEND ThirdParty_lib_dependency ${FFMPEG_LIB_LIST})

        foreach (LIB ${FFMPEG_LIB_LIST})
            #            message(WARNING ${temp${LIB}})
            #            message(WARNING ${CMAKE_INSTALL_PREFIX}/lib/ThirdParty/FFMPEG)
            #                install(FILES ${temp${LIB}} LIBRARY DESTINATION lib)
            file(COPY ${temp${LIB}} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/ThirdParty/FFMPEG)
        endforeach ()
    endif ()

    # 安装每个子目录下的头文件
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Core" DESTINATION include FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty" DESTINATION include FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Filters" DESTINATION include FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/IO" DESTINATION include FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Rendering" DESTINATION include FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Rendering" DESTINATION include FILES_MATCHING PATTERN "*.inl")

    # Transform shader code to install directories
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Assests" DESTINATION Resources)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Shaders/GLSL" DESTINATION Resources/Shaders)

    # Install Python scripts for NastranReader

    #        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Python/pyNastran")
    #            install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Python/pyNastran"
    #                    DESTINATION Resources/Python
    #                    FILES_MATCHING PATTERN "*.py"
    #                    PATTERN "__pycache__" EXCLUDE
    #                    PATTERN "*.pyc" EXCLUDE)
    #            message(STATUS "将安装Python脚本到 Resources/Python/pyNastran")
    #        endif()

    # 新增：当启用 GPSCUDA 模块时，一并安装其头文件和运行时库
    if (ENABLE_GPSCUDA_MODULE)
        # 首先尝试安装 CMakeGPS 源代码内的头文件（项目内放置的第三方）
        if (EXISTS "${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS")
            # 安装 extern 下的头文件
            install(DIRECTORY "${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/extern"
                    DESTINATION include/ThirdParty/CMakeGPS
                    FILES_MATCHING PATTERN "*.h")
            # 如果存在顶层 include 目录也安装
            if (EXISTS "${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/include")
                install(DIRECTORY "${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/include"
                        DESTINATION include/ThirdParty/CMakeGPS
                        FILES_MATCHING PATTERN "*.h")
            endif ()
        else ()
            message(WARNING "CMakeGPS source directory not found: ${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS")
        endif ()

        # 拷贝 ReferenceDLL 中的运行时 DLL 到安装的 bin 目录
        file(GLOB GPSCUDA_DLLS "${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/ReferenceDLL/*.dll")
        if (GPSCUDA_DLLS)
            file(COPY ${GPSCUDA_DLLS} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/GPSCUDA)
        else ()
            # 如果二进制在构建目录也尝试复制
            file(GLOB GPSCUDA_DLLS_BUILD "${CMAKE_BINARY_DIR}/CMakeGPS/CMakeGPS/ReferenceDLL/*.dll")
            if (GPSCUDA_DLLS_BUILD)
                file(COPY ${GPSCUDA_DLLS_BUILD} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/GPSCUDA)
            else ()
                message(STATUS "No GPSCUDA DLLs found to install")
            endif ()
        endif ()

        # 拷贝可能的静态库或 import lib 到安装的 lib 目录
        file(GLOB GPSCUDA_LIBS "${CMAKE_SOURCE_DIR}/CMakeGPS/CMakeGPS/ReferenceDLL/*.lib")
        if (GPSCUDA_LIBS)
            file(COPY ${GPSCUDA_LIBS} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/ThirdParty/GPSCUDA)
        else ()
            file(GLOB GPSCUDA_LIBS_BUILD "${CMAKE_BINARY_DIR}/CMakeGPS/CMakeGPS/ReferenceDLL/*.lib")
            if (GPSCUDA_LIBS_BUILD)
                file(COPY ${GPSCUDA_LIBS_BUILD} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/ThirdParty/GPSCUDA)
            else ()
                message(STATUS "No GPSCUDA .lib files found to install")
            endif ()
        endif ()

        # 如有需要，可将 GPSCUDA 依赖的库名加入 ThirdParty_lib_dependency 列表，例如：
        # list(APPEND ThirdParty_lib_dependency GPHelperIO GPSplineLib Geom CommonLib)
    endif ()

    if (ENABLE_NASTRAN_MODULE)
        # Install Nastran converter executable
        set(NASTRAN_CONVERTER_EXE "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Python/pyNastranLib/nastran_to_vtk_cli.exe")
        if (EXISTS "${NASTRAN_CONVERTER_EXE}")
            install(FILES "${NASTRAN_CONVERTER_EXE}"
                    DESTINATION bin/NastranConverter)
            message(STATUS "Installing NastranTransfer to bin/NastranConverter/nastran_to_vtk_cli.exe")
        else ()
            message(WARNING "Can't Find NastranTransfer: ${NASTRAN_CONVERTER_EXE}")
        endif ()
    endif ()

    # 导出模块的 CMake 配置文件，供其他项目查找使用
    install(EXPORT ${MODULE_NAME}Targets
            FILE ${MODULE_NAME}Targets.cmake
            NAMESPACE ${MODULE_NAME}::               # 设置命名空间，便于 find_package
            DESTINATION lib/cmake/${MODULE_NAME})

    # 创建配置文件以帮助 find_package 使用
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}ConfigVersion.cmake"
            VERSION 0.0.1
            COMPATIBILITY AnyNewerVersion
    )
    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}ConfigVersion.cmake"
            DESTINATION lib/cmake/${MODULE_NAME}
    )

    # 如果 CMakeGPS 提供的目标存在，把它们也加入导出集合，避免 install(EXPORT ...) 报错
    set(_IGAMEGPS_DEPS)
    foreach (_t IN ITEMS CommonLib Geom GPHelperIO GPSplineLib)
        if (TARGET ${_t})
            list(APPEND _IGAMEGPS_DEPS ${_t})
        endif ()
    endforeach ()
    if (_IGAMEGPS_DEPS)
        install(TARGETS ${_IGAMEGPS_DEPS}
                EXPORT ${MODULE_NAME}Targets
                ARCHIVE DESTINATION lib/ThirdParty/CMakeGPS
                LIBRARY DESTINATION lib/ThirdParty/CMakeGPS
                RUNTIME DESTINATION bin/ThirdParty/CMakeGPS
                INCLUDES DESTINATION include/ThirdParty/CMakeGPS)
    endif ()

    # 生成和安装 ${MODULE_NAME}Config.cmake 文件
    configure_file(Cmake/iGameCoreModuleConfig.cmake.in
            "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}Config.cmake" @ONLY)
    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}Config.cmake"
            DESTINATION lib/cmake/${MODULE_NAME})
endif ()