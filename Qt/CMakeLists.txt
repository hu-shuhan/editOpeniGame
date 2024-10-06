# 在系统环境变量里设置QT_HOME，值为Qt路径
if (DEFINED ENV{QT_HOME})
    set(Qt5_DIR "$ENV{QT_HOME}/5.14.2/msvc2017_64/lib/cmake/Qt5")
else ()
    # if complied in Linux,   set Qt5_DIR to e.g.(/usr/lib/Qt/5.14.2/gcc_64/lib/cmake/Qt5)
    # if complied in Windows, set Qt5_DIR to e.g.(D:/Qt/5.14.2/msvc2017_64/lib/cmake/Qt5)
    #    set(Qt5_DIR "D:/Software/Qt/5.14.2/msvc2017_64/lib/cmake/Qt5")
    #set(Qt5_DIR "F:/Qt/Qt5.14.2/5.14.2/msvc2017_64/lib/cmake/Qt5")
    set(Qt5_DIR "D:/Qt/5.14.2/msvc2017_64/lib/cmake/Qt5")
    #set(Qt5_DIR "E:/Software/Visualsoft/Qt/Qt5.14.2/5.14.2/msvc2017_64/lib/cmake/Qt5")
endif ()

set(MODULE_NAME Qt_module)

set(CMAKE_INCLUDE_CURRENT_DIR ON) #将编译目录包含到Include目录，否则编译器会找不到qt编译的这些头文件
set(CMAKE_AUTOMOC ON) # for meta object compiler
set(CMAKE_AUTORCC ON) # resource files
set(CMAKE_AUTOUIC ON) # UI files
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

# 查找 Qt
#find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
#qt_standard_project_setup()
Qt5_add_resources(QRC_FILES "Resources/iGameQtMainWindow.qrc")
#set(QRC_FILES ${QRC_FILES} PARENT_SCOPE)

file(GLOB_RECURSE UI_FILES Resources/UI/*.ui)
Qt5_wrap_ui(UI_HEAD_FILES ${UI_FILES})

get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(DEPLOYQT_EXECUTABLE NAMES windeployqt macdeployqt HINTS "${_qt_bin_dir}")


file(GLOB_RECURSE QT_SRCS CONFIGURE_DEPENDS src/*.cpp include/*.h Plugin/*)


add_library(${MODULE_NAME} SHARED
        ${QRC_FILES}
        ${UI_FILES}
        ${UI_HEAD_FILES}
        ${QT_SRCS}
)


set_source_files_properties(${QRC_FILES} PROPERTIES GENERATED ON)

target_include_directories(${MODULE_NAME} PUBLIC
        include
        Plugin
        "${CMAKE_BINARY_DIR}/Qt"
)
set(LINK_LIBS)


list(APPEND LINK_LIBS
        Qt5::OpenGL
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        iGameCore_module
        #iGameRendering_module
)

if (EMSCRIPTEN)
    set(LINK_OPTS)
    list(APPEND LINK_LIBS Qt5::Platform)
    list(APPEND LINK_OPTS "SHELL:-s FULL_ES2=1")
    target_link_options(${MODULE_NAME} PUBLIC ${LINK_OPTS})
endif ()

target_link_libraries(${MODULE_NAME} PUBLIC ${LINK_LIBS})
target_compile_definitions(${MODULE_NAME} PRIVATE IG_QT_COMPILE_EXPORT)
if (WIN32)
    #  内部库在cmake构建时定义IG_QT_COMPILE_EXPORT，使得模式切换为dllexport, 否则外部库在使用编译这个动态库代码时将其模式切换为dllimport
    target_compile_definitions(${MODULE_NAME} PRIVATE IG_QT_COMPILE_EXPORT)
    #  由于Windows不会搜索其他目录下的dll文件因此要使用cmake指令将dll移至exe目录
    #  将QT模块编译成的可执行文件(.exe)、静态库(.lib)、动态库(.dll)的输出移至放入主程序源码目录
    set_property(TARGET ${MODULE_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
    set_property(TARGET ${MODULE_NAME} PROPERTY LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
    #   get_target_property(Qt5Core_LOCATION Qt5::Core LOCATION)
    #   get_filename_component(Qt5_INSTALL_DIR ${Qt5Core_LOCATION} DIRECTORY)
    #   # 构建 Qt 的 DLL 文件路径
    #   set(Qt5_DLL_DIR "${Qt5_INSTALL_DIR}")
    #   if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    #       set(Qt5_DLLS
    #               ${Qt5_DLL_DIR}/Qt5Cored.dll
    #               ${Qt5_DLL_DIR}/Qt5OpenGLd.dll
    #               #                ${Qt5_DLL_DIR}/Qt5OpenGLWidgetsd.dll
    #               ${Qt5_DLL_DIR}/Qt5Widgetsd.dll
    #               ${Qt5_DLL_DIR}/Qt5Guid.dll
    #       )
    #   elseif (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    #       set(Qt5_DLLS
    #               ${Qt5_DLL_DIR}/Qt5Core.dll
    #               ${Qt5_DLL_DIR}/Qt5OpenGL.dll
    #               #                ${Qt5_DLL_DIR}/Qt5OpenGLWidgets.dll
    #               ${Qt5_DLL_DIR}/Qt5Widgets.dll
    #               ${Qt5_DLL_DIR}/Qt5Gui.dll
    #
    #       )
    #   endif ()
    #   # 添加一个自定义命令来复制 Qt DLL 到构建目标的目录
    #   add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
    #           COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #           ${Qt5_DLLS}
    #           $<TARGET_FILE_DIR:${PROJECT_NAME}>
    #   )
endif ()
target_link_libraries(${PROJECT_NAME} PUBLIC ${MODULE_NAME})