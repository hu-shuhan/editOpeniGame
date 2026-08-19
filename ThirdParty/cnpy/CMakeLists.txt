add_library(cnpy STATIC cnpy.cpp)

target_include_directories(cnpy PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(cnpy PUBLIC zlibstatic)

target_compile_features(cnpy PUBLIC cxx_std_11)
