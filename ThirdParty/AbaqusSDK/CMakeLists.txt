set(MODULE_NAME ABAQUSSDK)

add_library(${MODULE_NAME}
        INTERFACE)
target_include_directories(${MODULE_NAME} INTERFACE
        include
        2024
)
file(GLOB libs lib/*.lib)
message(${libs})
target_link_libraries(${MODULE_NAME} INTERFACE
        ${libs}
)