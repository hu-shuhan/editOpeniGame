if (CORE_MODULE_INSTALL)
    # 安装模块库
    install(TARGETS ${MODULE_NAME}
            EXPORT ${MODULE_NAME}Targets           # 导出名称为 ModuleTargets.cmake
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin
            INCLUDES DESTINATION include)
    install(TARGETS ${ThirdParty_lib_dependency}
            EXPORT ${MODULE_NAME}Targets           # 导出名称为 ModuleTargets.cmake
            LIBRARY DESTINATION lib/ThirdParty
            INCLUDES DESTINATION include)
    if(${AbqSDK_FOUND})
        install(DIRECTORY ${AbqSDK_PRIVATE_INCLUDE_DIR} DESTINATION include/ThirdParty/AbaqusSDK FILES_MATCHING PATTERN "*.h")
        file(GLOB DLL_FILES "${AbqSDK_DLL_DIR}/*.dll")
        file(COPY ${DLL_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/AbaqusSDK)
        list(APPEND ThirdParty_lib_dependency ${ABQ_LIB_LIST})

        foreach (LIB ${ABQ_LIB_LIST})
#            install(FILES ${temp${LIB}} LIBRARY DESTINATION lib/ThirdParty/AbaqusSDK)
            file(COPY ${temp${LIB}} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/ThirdParty/AbaqusSDK)
        endforeach ()
    endif ()
    if(${FFMPEG_FOUND})
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