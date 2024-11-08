add_library(vtkkwiml INTERFACE)
target_include_directories(vtkkwiml INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>

)

