if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_subdirectory(Debug)
    set(LIBTORCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Debug")
else()
    add_subdirectory(Release)
    set(LIBTORCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Release")
endif()

#include_directories(
#        "${LIBTORCH_DIR}/include"
#        "${LIBTORCH_DIR}/include/torch/csrc/api/include"
#)
#
#if (WIN32)
#    file(GLOB TORCH_DLLS "${LIBTORCH_DIR}/lib/*.dll")
#    add_custom_target(libtorchDlls ALL
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different
#            ${TORCH_DLLS}
#            ${CMAKE_BINARY_DIR}/$<CONFIG>
#            DEPENDS ${TORCH_DLLS}
#            COMMENT "Copying libtorch DLLs to output folder"
#    )
#    add_dependencies(${PROJECT_NAME} libtorchDlls)
#else ()
#    message(FATAL_ERROR "Only Windows platform is supported for this project! LibTorch DLL copy is defined for Windows only.")
#endif()