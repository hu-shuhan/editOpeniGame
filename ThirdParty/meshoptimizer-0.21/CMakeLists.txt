set(SOURCES
        src/meshoptimizer.h
        src/allocator.cpp
        src/clusterizer.cpp
        src/indexcodec.cpp
        src/indexgenerator.cpp
        src/overdrawanalyzer.cpp
        src/overdrawoptimizer.cpp
        src/quantization.cpp
        src/simplifier.cpp
        src/spatialorder.cpp
        src/stripifier.cpp
        src/vcacheanalyzer.cpp
        src/vcacheoptimizer.cpp
        src/vertexcodec.cpp
        src/vertexfilter.cpp
        src/vfetchanalyzer.cpp
        src/vfetchoptimizer.cpp
)

add_library(meshoptimizer STATIC ${SOURCES})

target_include_directories(meshoptimizer PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include/meshoptimizer-0.21/src>
)