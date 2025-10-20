# ============================================================================
# Nastran Reader with Python/Pybind11 Integration
# ============================================================================

# 确保Python已经配置
if(NOT Python3_FOUND)
    message(FATAL_ERROR "NastranReader需要Python3，请先配置Python环境")
endif()

if(NOT pybind11_FOUND)
    message(FATAL_ERROR "NastranReader需要pybind11")
endif()

# 源文件
set(NASTRAN_SOURCES
    iGameNastranReader.cpp
)

set(NASTRAN_HEADERS
    iGameNastranReader.h
)

# 添加到父级的源文件列表
list(APPEND IO_SRC ${NASTRAN_SOURCES})
list(APPEND IO_HEADER ${NASTRAN_HEADERS})

# 传递给父级作用域
set(IO_SRC ${IO_SRC} PARENT_SCOPE)
set(IO_HEADER ${IO_HEADER} PARENT_SCOPE)

# 定义Python脚本路径宏
# 这个路径会在运行时用于导入Python模块
add_compile_definitions(PYNASTRAN_SCRIPT_PATH="${PYNASTRAN_SCRIPT_DIR}")

message(STATUS "NastranReader配置完成，Python脚本路径: ${PYNASTRAN_SCRIPT_DIR}")


