find_package(VTK 9 CONFIG REQUIRED COMPONENTS
        CommonCore
        CommonDataModel
        IOLegacy
        IOXML
)

set(IGAME_DATACODEC_VTK_MODULES
        VTK::CommonCore
        VTK::CommonDataModel
        VTK::IOLegacy
        VTK::IOXML
)

add_library(dataCodecVtkAdapter STATIC
        VtkDataCodecAdapters.cpp
        VtkDataCodecAdapters.h
        VtkUnstructuredGridIO.cpp
        VtkUnstructuredGridIO.h
)
target_include_directories(dataCodecVtkAdapter PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(dataCodecVtkAdapter PUBLIC
        iGameCore
        VTK::CommonCore
        VTK::CommonDataModel
        VTK::IOLegacy
        VTK::IOXML
)

vtk_module_autoinit(
        TARGETS dataCodecVtkAdapter
        MODULES ${IGAME_DATACODEC_VTK_MODULES}
)