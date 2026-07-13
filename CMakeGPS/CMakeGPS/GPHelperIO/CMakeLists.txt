set(LIBRARY_NAME GPHelperIO)

file(GLOB SOURCE_FILES *.cpp)
file(GLOB INCLUDE_FILES *.hpp)
#message(${SOURCE_FILES})
add_library(${LIBRARY_NAME} STATIC ${INCLUDE_FILES} ${SOURCE_FILES})

target_link_libraries(${LIBRARY_NAME}
        #CommonLib helper_opencascade Geom spdlog::spdlog glm
        #CommonLib helper_opencascade Geom glm
        CommonLib Geom glm
)