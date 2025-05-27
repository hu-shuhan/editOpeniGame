#target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(GLAD_VERSION glad460)
#set(GLAD_VERSION glad330)

file(GLOB_RECURSE srcs CONFIGURE_DEPENDS ${GLAD_VERSION}/src/*.c ${GLAD_VERSION}/include/glad/*.h)


# Currently, only shared libraries for the Linux system are supported
if(IGAME_BUILD_SHARED AND UNIX AND NOT APPLE)
    message(WARNING "Building Shared Lib.")
    add_library(glad SHARED ${srcs})
else ()
    add_library(glad STATIC ${srcs})
endif ()

# 添加位置无关代码设置
set_property(TARGET glad PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(glad PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${GLAD_VERSION}/include>
        $<INSTALL_INTERFACE:include/glad/${GLAD_VERSION}/include>

)