set(LIBRARY_NAME Geom)

file(GLOB SOURCE_FILES *.*)
file(GLOB SUB_SOURCE_FILES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
#message("hello")
#message(${SUB_SOURCE_FILES})
add_library(${LIBRARY_NAME} STATIC ${SUB_SOURCE_FILES})

target_link_libraries(
        ${LIBRARY_NAME}
#        PUBLIC helper_opencascade
)