add_library(eigen INTERFACE)
set(Eigen_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/Eigen/ CACHE INTERNAL "ThirdParty variable")

target_include_directories(eigen INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/ThirdParty/eigen-3.4.0>
)