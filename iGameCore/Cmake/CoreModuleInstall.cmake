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
    if (ENABLE_CGNS_MODULE)
        find_package(HDF5)
        install(FILES
                ${HDF5_LIBRARIES}/../libhdf5.lib
                ${HDF5_LIBRARIES}/../libhdf5_hl.lib
                ${HDF5_LIBRARIES}/../libhdf5_tools.lib
                ${HDF5_LIBRARIES}/../libzlib.lib
                ${HDF5_LIBRARIES}/../libsz.lib
                ${HDF5_LIBRARIES}/../libaec.lib
                DESTINATION lib/ThirdParty)

        #        install(FILES
        #                ${HDF5_DIR}/../../lib/libhdf5.lib
        #                ${HDF5_DIR}/../../lib/libhdf5_hl.lib
        #                ${HDF5_DIR}/../../lib/libhdf5_tools.lib
        #                ${HDF5_DIR}/../../lib/libzlib.lib
        #                ${HDF5_DIR}/../../lib/libsz.lib
        #                ${HDF5_DIR}/../../lib/libaec.lib
        #                DESTINATION lib/ThirdParty)

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
            #            install(FILES ${temp${LIB}} LIBRARY DESTINATION lib)
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

if(ENABLE_NASTRAN_MODULE)
    # Install Nastran converter executable
    set(NASTRAN_CONVERTER_EXE "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Python/pyNastranLib/nastran_to_vtk_cli.exe")
    if(EXISTS "${NASTRAN_CONVERTER_EXE}")
        install(FILES "${NASTRAN_CONVERTER_EXE}"
                DESTINATION bin/NastranConverter)
        message(STATUS "将安装Nastran转换器到 bin/NastranConverter/nastran_to_vtk_cli.exe")
    else()
        message(WARNING "未找到Nastran转换器: ${NASTRAN_CONVERTER_EXE}")
    endif()
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

    # 生成和安装 ${MODULE_NAME}Config.cmake 文件
    configure_file(Cmake/iGameCoreModuleConfig.cmake.in
            "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}Config.cmake" @ONLY)
    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}Config.cmake"
            DESTINATION lib/cmake/${MODULE_NAME})
endif ()