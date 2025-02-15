#target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(GLAD_VERSION glad460)
#set(GLAD_VERSION glad330)

file(GLOB_RECURSE srcs CONFIGURE_DEPENDS ${GLAD_VERSION}/src/*.c ${GLAD_VERSION}/include/glad/*.h)
add_library(glad STATIC ${srcs})

target_include_directories(glad PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${GLAD_VERSION}/include>
        $<INSTALL_INTERFACE:include/glad/${GLAD_VERSION}/include>

)