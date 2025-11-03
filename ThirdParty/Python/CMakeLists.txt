## ============================================================================
## Python and PyNastran Configuration
## ============================================================================
#cmake_minimum_required(VERSION 3.15)
#
## 查找Python解释器
## 用户可以通过 -DPython3_ROOT_DIR=<path> 指定Python路径
#find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
#
#message(STATUS "Python3 Executable: ${Python3_EXECUTABLE}")
#message(STATUS "Python3 Version: ${Python3_VERSION}")
#message(STATUS "Python3 Include Dirs: ${Python3_INCLUDE_DIRS}")
#message(STATUS "Python3 Libraries: ${Python3_LIBRARIES}")
#
## ============================================================================
## 检查并安装Python依赖包
## ============================================================================
#set(PYNASTRAN_REQUIREMENTS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/pyNastran/requirements.txt")
#
## 定义需要检查的Python模块列表
#set(REQUIRED_PYTHON_MODULES
#    numpy
#    scipy
#    cpylog
#    pyNastran
#)
#
## 函数：检查Python模块是否已安装
#function(check_python_module MODULE_NAME RESULT_VAR)
#    execute_process(
#        COMMAND ${Python3_EXECUTABLE} -c "import ${MODULE_NAME}"
#        RESULT_VARIABLE MODULE_CHECK_RESULT
#        OUTPUT_QUIET
#        ERROR_QUIET
#    )
#    if(MODULE_CHECK_RESULT EQUAL 0)
#        set(${RESULT_VAR} TRUE PARENT_SCOPE)
#        message(STATUS "Python模块 '${MODULE_NAME}' 已安装")
#    else()
#        set(${RESULT_VAR} FALSE PARENT_SCOPE)
#        message(WARNING "Python模块 '${MODULE_NAME}' 未安装")
#    endif()
#endfunction()
#
## 检查所有必需的模块
#set(MISSING_MODULES "")
#foreach(MODULE ${REQUIRED_PYTHON_MODULES})
#    check_python_module(${MODULE} MODULE_FOUND)
#    if(NOT MODULE_FOUND)
#        list(APPEND MISSING_MODULES ${MODULE})
#    endif()
#endforeach()
#
## 如果有缺失的模块，安装requirements.txt中的依赖
#if(MISSING_MODULES)
#    message(STATUS "检测到缺失的Python模块: ${MISSING_MODULES}")
#    message(STATUS "正在安装Python依赖...")
#
#    # 首先安装requirements.txt中的基础依赖
#    execute_process(
#        COMMAND ${Python3_EXECUTABLE} -m pip install -r ${PYNASTRAN_REQUIREMENTS_FILE}
#        RESULT_VARIABLE PIP_INSTALL_RESULT
#        OUTPUT_VARIABLE PIP_INSTALL_OUTPUT
#        ERROR_VARIABLE PIP_INSTALL_ERROR
#    )
#
#    if(NOT PIP_INSTALL_RESULT EQUAL 0)
#        message(WARNING "安装requirements.txt失败: ${PIP_INSTALL_ERROR}")
#    else()
#        message(STATUS "requirements.txt安装成功")
#    endif()
#
#    # 安装pyNastran（如果不在requirements.txt中）
#    check_python_module(pyNastran PYNASTRAN_FOUND)
#    if(NOT PYNASTRAN_FOUND)
#        message(STATUS "正在安装pyNastran...")
#        execute_process(
#            COMMAND ${Python3_EXECUTABLE} -m pip install pyNastran
#            RESULT_VARIABLE PYNASTRAN_INSTALL_RESULT
#            OUTPUT_QUIET
#            ERROR_QUIET
#        )
#        if(PYNASTRAN_INSTALL_RESULT EQUAL 0)
#            message(STATUS "pyNastran安装成功")
#        else()
#            message(FATAL_ERROR "pyNastran安装失败，请手动安装: pip install pyNastran")
#        endif()
#    endif()
#endif()
#
## 最终验证所有模块是否可用
#message(STATUS "验证Python依赖...")
#foreach(MODULE ${REQUIRED_PYTHON_MODULES})
#    check_python_module(${MODULE} MODULE_FOUND)
#    if(NOT MODULE_FOUND)
#        message(FATAL_ERROR "Python模块 '${MODULE}' 仍然不可用，请手动安装")
#    endif()
#endforeach()
#
## ============================================================================
## Pybind11配置
## ============================================================================
## 优先查找系统安装的pybind11，如果没有则从Python包中获取
#find_package(pybind11 CONFIG QUIET)
#
#if(NOT pybind11_FOUND)
#    message(STATUS "未找到系统pybind11，尝试使用Python中的pybind11...")
#
#    # 尝试通过pip安装pybind11
#    execute_process(
#        COMMAND ${Python3_EXECUTABLE} -m pip install pybind11
#        RESULT_VARIABLE PYBIND11_INSTALL_RESULT
#        OUTPUT_QUIET
#        ERROR_QUIET
#    )
#
#    # 获取pybind11的CMake目录
#    execute_process(
#        COMMAND ${Python3_EXECUTABLE} -m pybind11 --cmakedir
#        OUTPUT_VARIABLE PYBIND11_CMAKE_DIR
#        OUTPUT_STRIP_TRAILING_WHITESPACE
#        RESULT_VARIABLE PYBIND11_CMAKE_RESULT
#    )
#
#    if(PYBIND11_CMAKE_RESULT EQUAL 0 AND EXISTS ${PYBIND11_CMAKE_DIR})
#        message(STATUS "找到pybind11 CMake配置: ${PYBIND11_CMAKE_DIR}")
#        set(pybind11_DIR ${PYBIND11_CMAKE_DIR})
#        find_package(pybind11 CONFIG REQUIRED)
#    else()
#        message(FATAL_ERROR "无法找到pybind11，请手动安装: pip install pybind11")
#    endif()
#endif()
#
#message(STATUS "pybind11版本: ${pybind11_VERSION}")
#message(STATUS "pybind11 Include: ${pybind11_INCLUDE_DIRS}")
#
## ============================================================================
## 导出变量供父级CMakeLists使用
## ============================================================================
#set(Python3_EXECUTABLE ${Python3_EXECUTABLE} PARENT_SCOPE)
#set(Python3_INCLUDE_DIRS ${Python3_INCLUDE_DIRS} PARENT_SCOPE)
#set(Python3_LIBRARIES ${Python3_LIBRARIES} PARENT_SCOPE)
#set(Python3_VERSION ${Python3_VERSION} PARENT_SCOPE)
#set(pybind11_INCLUDE_DIRS ${pybind11_INCLUDE_DIRS} PARENT_SCOPE)
#set(PYNASTRAN_SCRIPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pyNastran PARENT_SCOPE)
#
## 将pyNastran脚本目录添加到Python路径
#set(PYNASTRAN_PYTHON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pyNastran PARENT_SCOPE)
#
#message(STATUS "==========================================================")
#message(STATUS "Python环境配置完成")
#message(STATUS "==========================================================")
#

if (ENABLE_NASTRAN_MODULE)
    set(PYNASTRAN_LIB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pyNastranLib)
    set(PYTHON_RESOURCES_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Resources)

    file(MAKE_DIRECTORY ${PYTHON_RESOURCES_OUTPUT_DIR})

    file(COPY ${PYNASTRAN_LIB_SOURCE_DIR}/ DESTINATION ${PYTHON_RESOURCES_OUTPUT_DIR}/pyNastranLib)
    #message(WARNING "${PYTHON_RESOURCES_OUTPUT_DIR}  SOURCE ${PYNASTRAN_LIB_SOURCE_DIR}" )
endif ()
