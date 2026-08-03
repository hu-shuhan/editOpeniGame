set(LIBRARY_NAME GPHelperIO)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS "*.cpp")
file(GLOB INCLUDE_FILES CONFIGURE_DEPENDS "*.hpp")

add_library(${LIBRARY_NAME}
        STATIC
        ${INCLUDE_FILES}
        ${SOURCE_FILES}
)

target_link_libraries(${LIBRARY_NAME}
        PRIVATE
        CommonLib
        Geom
        $<BUILD_INTERFACE:glm>
)