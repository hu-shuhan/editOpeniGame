target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(GLAD_VERSION glad460)

file(GLOB_RECURSE srcs CONFIGURE_DEPENDS ${GLAD_VERSION}/src/*.c ${GLAD_VERSION}/include/glad/*.h)
add_library(glad STATIC ${srcs})

target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${GLAD_VERSION}/include")